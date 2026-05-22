// hal_nmtv154.cpp — NM TV 1.54" (ESP32 classic) HAL implementation
// Only compiled when NMTV154_BOARD is defined in build_flags.
#ifdef NMTV154_BOARD

#include "hal.h"
#include <Arduino.h>
#include <driver/ledc.h>

// ---------------------------------------------------------------------------
// Pin mapping
// ---------------------------------------------------------------------------
// Capacitive touchpad — T9 = GPIO32
//   touchRead() raw: ~100 idle, ~78-90 when pressed
#define PIN_TOUCH_A           T9   // GPIO32
#define TOUCH_THRESHOLD_PRESS 90

// Physical BOOT button on GPIO0 (active-low) — used as BtnB fallback
#define PIN_BTN_B    0

#define PIN_LCD_BL   19   // Backlight — active LOW: LOW=ON, HIGH=OFF
#define PIN_LCD_PWR  21   // Screen power rail — active LOW: LOW=ON

// ---------------------------------------------------------------------------
// TFT_eSPI display object (extern declared in hal.h)
// ---------------------------------------------------------------------------
TFT_eSPI nmtv154_display;

// ---------------------------------------------------------------------------
// Backlight — LEDC PWM, inverted duty (active LOW panel)
//   duty = 0    → GPIO always LOW  → backlight fully ON
//   duty = 1023 → GPIO always HIGH → backlight OFF
//   level 0..4  → duty 800..0
// ---------------------------------------------------------------------------
#define BL_LEDC_CHAN  0
#define BL_LEDC_FREQ  5000
#define BL_LEDC_RES   10    // 10-bit → 0..1023

static uint8_t _brightLevel = 4;

static void _setBl(uint8_t level) {
    uint32_t duty = (uint32_t)((4u - level) * 200u);
    ledcWrite(BL_LEDC_CHAN, duty);
}

// ---------------------------------------------------------------------------
// Button A — T9/GPIO32 capacitive touchpad, polled every halUpdate() tick
//   touchRead() returns raw counts; lower = more finger contact.
// Button B — GPIO0 BOOT button (INPUT_PULLUP), active-low
// ---------------------------------------------------------------------------
static bool     _btnDown    = false;
static bool     _btnRelease = false;  // true for exactly one halUpdate() tick
static uint32_t _btnDownMs  = 0;

static bool     _btnBDown    = false;
static bool     _btnBRelease = false;

static void _btnScan() {
    // --- BtnA: capacitive touch ---
    bool now = (touchRead(PIN_TOUCH_A) < TOUCH_THRESHOLD_PRESS);
    _btnRelease = (!now && _btnDown);
    if (now && !_btnDown) _btnDownMs = millis();
    if (!now)             _btnDownMs = 0;
    _btnDown = now;

    // --- BtnB: BOOT button ---
    bool nowB   = (digitalRead(PIN_BTN_B) == LOW);
    _btnBRelease = (!nowB && _btnBDown);
    _btnBDown    = nowB;
}

// ---------------------------------------------------------------------------
// HAL implementation
// ---------------------------------------------------------------------------
void halBegin() {
    Serial.begin(115200);

    // Power on the display panel (active LOW)
    pinMode(PIN_LCD_PWR, OUTPUT);
    digitalWrite(PIN_LCD_PWR, LOW);
    delay(50);

    // Backlight: configure LEDC and keep off during init
    ledcSetup(BL_LEDC_CHAN, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(PIN_LCD_BL, BL_LEDC_CHAN);
    ledcWrite(BL_LEDC_CHAN, 1023);  // off (HIGH → backlight off)

    // Initialise ST7789 240×240 panel
    nmtv154_display.init();
    nmtv154_display.setRotation(0);
    nmtv154_display.invertDisplay(true);  // 240×240 panel requires INVON
    nmtv154_display.fillScreen(TFT_BLACK);

    // Turn on backlight at full brightness
    _setBl(4);

    // BtnB: BOOT button (capacitive touch needs no pinMode)
    pinMode(PIN_BTN_B, INPUT_PULLUP);
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
// BtnB: GPIO0 BOOT button
bool halBtnB_isPressed()             { return _btnBDown; }
bool halBtnB_wasPressed()            { return _btnBRelease; }

// --- Power / Screen ---
void halBrightness(uint8_t level) {
    _brightLevel = level;
    _setBl(level);
}

void halScreenOn(bool on) {
    if (on) _setBl(_brightLevel);
    else    ledcWrite(BL_LEDC_CHAN, 1023);  // fully off
}

// No PMU — soft power-off not possible; restart instead.
void halPowerOff()    { ESP.restart(); }

// No power monitoring — conservatively report USB present.
bool halOnUsb()       { return true; }

// No AXP power button
int  halPwrBtnEvent() { return 0; }

// --- IMU --- (no IMU on NMTV154)
void halImuGetAccel(float* ax, float* ay, float* az) {
    *ax = 0.0f; *ay = 0.0f; *az = 0.0f;
}

// --- RTC --- (no hardware RTC; clock face stays dormant)
void halRtcGet(RTC_TimeTypeDef* t, RTC_DateTypeDef* d) {
    (void)t; (void)d;
}

// --- Sound --- (no buzzer; log to serial)
void halBeep(uint16_t freq, uint16_t dur) {
    Serial.printf("[beep] freq=%u dur=%u\n", freq, dur);
}

#endif // NMTV154_BOARD
