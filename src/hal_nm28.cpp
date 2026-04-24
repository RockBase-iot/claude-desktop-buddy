// hal_nm28.cpp — NM 2.8" (ESP32-S3) HAL implementation
// Only compiled when NM28_BOARD is defined in build_flags.
#ifdef NM28_BOARD

#include "hal.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/ledc.h>

// ---------------------------------------------------------------------------
// XPowersLib — AXP2101 with non-standard chip ID (0x47 instead of 0x4A)
// Must patch the constant before including XPowersLib.h.
// ---------------------------------------------------------------------------
#define XPOWERS_CHIP_AXP2101
#include <REG/AXP2101Constants.h>
#undef  XPOWERS_AXP2101_CHIP_ID
#define XPOWERS_AXP2101_CHIP_ID (0x47)
#include <XPowersLib.h>

// ---------------------------------------------------------------------------
// QMI8658 — from lewisxhe/SensorLib
// ---------------------------------------------------------------------------
#include <SensorQMI8658.hpp>

// ---------------------------------------------------------------------------
// Pin mapping (from NM-Display-28inch project)
// ---------------------------------------------------------------------------
#define NM28_PIN_I2C_SDA  8
#define NM28_PIN_I2C_SCL  7
#define NM28_PIN_BTN_A    0    // sole physical button, active-low
#define NM28_PIN_LCD_BL   6   // backlight, driven by LEDC PWM

// ---------------------------------------------------------------------------
// TCA9554 I2C IO expander — drives LCD RST (and other board signals)
// Address 0x20 (A2=A1=A0=GND).  LCD RST = PIN_NUM_1 (bit 1).
// Registers: 0x01=Output Port, 0x03=Configuration (0=output,1=input)
// ---------------------------------------------------------------------------
#define TCA9554_ADDR       0x20
#define TCA9554_REG_OUT    0x01
#define TCA9554_REG_CFG    0x03
#define TCA9554_LCD_RST_BIT  (1 << 1)   // IO1

static void _tca9554Write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// Configure LCD RST pin as output and toggle it to reset the display.
static void _lcdResetViaExpander() {
  // Set bit1 as output (0), keep others as input (1) = 0b11111101 = 0xFD
  _tca9554Write(TCA9554_REG_CFG, 0xFD);
  // Drive RST LOW
  _tca9554Write(TCA9554_REG_OUT, 0x00);
  delay(100);
  // Drive RST HIGH
  _tca9554Write(TCA9554_REG_OUT, TCA9554_LCD_RST_BIT);
  delay(100);
}

// ---------------------------------------------------------------------------
// Display object (extern declared in hal.h)
// ---------------------------------------------------------------------------
TFT_eSPI nm28_display;

// ---------------------------------------------------------------------------
// Peripherals
// ---------------------------------------------------------------------------
static XPowersPMU    _pmu;
static SensorQMI8658 _qmi;

// ---------------------------------------------------------------------------
// Backlight — LEDC PWM on GPIO6
// ---------------------------------------------------------------------------
#define BL_LEDC_CHAN  0
#define BL_LEDC_FREQ  5000
#define BL_LEDC_RES   10     // 10-bit → duty 0..1023

static uint8_t _brightLevel = 4;

static void _setBl(uint8_t level) {
  // level 0..4 → duty maps 20%..100%
  uint32_t duty = (uint32_t)((20u + level * 20u) * 1023u / 100u);
  ledcWrite(BL_LEDC_CHAN, duty);
}

// ---------------------------------------------------------------------------
// Button debounce — GPIO0 (INPUT_PULLUP, press = LOW)
// ---------------------------------------------------------------------------
static bool     _btnDown    = false;
static bool     _btnRelease = false;   // true for exactly one halUpdate() tick
static uint32_t _btnDownMs  = 0;

static void _btnScan() {
  bool now = (digitalRead(NM28_PIN_BTN_A) == LOW);
  _btnRelease = (!now && _btnDown);
  if (now && !_btnDown) _btnDownMs = millis();
  if (!now)             _btnDownMs = 0;
  _btnDown = now;
}

// ---------------------------------------------------------------------------
// HAL implementation
// ---------------------------------------------------------------------------
void halBegin() {
  Serial.begin(115200);
  delay(1000);  // DBG: wait for serial monitor to connect
  Serial.println("[hal] halBegin start");

  Serial.println("[hal] Wire.begin...");
  Wire.begin(NM28_PIN_I2C_SDA, NM28_PIN_I2C_SCL);
  Serial.println("[hal] Wire.begin OK");

  // I2C scan — DBG: show all devices on bus
  Serial.println("[hal] I2C scan:");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
      Serial.printf("  found 0x%02X\n", addr);
  }

  // Reset display via TCA9554 IO expander (LCD RST = PIN1)
  Serial.println("[hal] TCA9554 LCD reset...");
  _lcdResetViaExpander();
  Serial.println("[hal] TCA9554 reset OK");

  // Display
  Serial.println("[hal] nm28_display.init()...");
  nm28_display.init();
  Serial.println("[hal] display init OK");
  // ST7789_Init.h unconditionally sends INVON (0x21) — override it here
  // by writing raw SPI commands so we don't depend on build-flag caching.
  // INVOFF = 0x20, MADCTL = 0x36
  // Landscape rotation 1: MX(0x40)|MV(0x20) = 0x60, no BGR bit (0x08)
  nm28_display.startWrite();
  nm28_display.writecommand(0x20);   // INVOFF — disable hardware colour inversion
  nm28_display.writecommand(0x36);   // MADCTL
  nm28_display.writedata(0x60);      // MX|MV = landscape, RGB colour order (no BGR)
  nm28_display.endWrite();
  nm28_display.setRotation(1);   // sets _width/_height accounting; re-sends MADCTL
  nm28_display.invertDisplay(true);
  nm28_display.fillScreen(TFT_BLACK);   // should paint true black now
  Serial.println("[hal] display rotation+invert OK");

  // Backlight
  Serial.println("[hal] backlight init...");
  ledcSetup(BL_LEDC_CHAN, BL_LEDC_FREQ, BL_LEDC_RES);
  ledcAttachPin(NM28_PIN_LCD_BL, BL_LEDC_CHAN);
  _setBl(4);
  Serial.println("[hal] backlight ON");

  // Button
  pinMode(NM28_PIN_BTN_A, INPUT_PULLUP);

  // PMU — AXP2101
  Serial.println("[hal] AXP2101 init...");
  if (!_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS,
                  NM28_PIN_I2C_SDA, NM28_PIN_I2C_SCL)) {
    Serial.println("[hal] AXP2101 not found");
  } else {
    Serial.println("[hal] AXP2101 OK");
  }

  // IMU — QMI8658
  Serial.println("[hal] QMI8658 init...");
  if (!_qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS,
                  NM28_PIN_I2C_SDA, NM28_PIN_I2C_SCL)) {
    Serial.println("[hal] QMI8658 not found");
  } else {
    Serial.println("[hal] QMI8658 OK");
    _qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                             SensorQMI8658::ACC_ODR_125Hz,
                             SensorQMI8658::LPF_MODE_0);
    _qmi.enableAccelerometer();
  }

  Serial.println("[hal] halBegin done");
}

void halUpdate() {
  _btnScan();
}

// --- Buttons ---
bool halBtnA_isPressed()             { return _btnDown; }
bool halBtnA_wasReleased()           { return _btnRelease; }
bool halBtnA_pressedFor(uint32_t ms) {
  return _btnDown && _btnDownMs && (millis() - _btnDownMs >= ms);
}
// BtnB not wired — always false
bool halBtnB_isPressed()             { return false; }
bool halBtnB_wasPressed()            { return false; }

// --- Power / Screen ---
void halBrightness(uint8_t level) {
  _brightLevel = level;
  _setBl(level);
}

void halScreenOn(bool on) {
  if (on) {
    _setBl(_brightLevel);
  } else {
    ledcWrite(BL_LEDC_CHAN, 0);
  }
}

void halPowerOff() {
  _pmu.shutdown();
}

bool halOnUsb() {
  return _pmu.isVbusIn();
}

int halPwrBtnEvent() {
  return 0;   // no AXP power button on NM28
}

// --- IMU ---
void halImuGetAccel(float* ax, float* ay, float* az) {
  if (_qmi.getDataReady()) {
    _qmi.getAccelerometer(*ax, *ay, *az);
  }
}

// --- RTC ---
// PCF85063 not yet wired up — return zeros; clock face stays dormant.
void halRtcGet(RTC_TimeTypeDef* t, RTC_DateTypeDef* d) {
  (void)t; (void)d;
}

// --- Sound ---
void halBeep(uint16_t freq, uint16_t dur) {
  Serial.printf("[beep] freq=%u dur=%u\n", freq, dur);
}

#endif // NM28_BOARD
