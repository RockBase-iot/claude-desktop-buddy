#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// Hardware Abstraction Layer — lets one source tree target both
//   [env:m5stickc-plus]  (original M5StickC Plus, 135×240)
//   [env:nm-display-28]  (NM 2.8" ESP32-S3 board, 320×240)
//
// Define NM28_BOARD in build_flags to activate the NM28 path.
// Everything that touched M5.xxx in main.cpp now calls hal___() instead.
// ---------------------------------------------------------------------------

#ifdef NM28_BOARD
  // ── NM28: bare TFT_eSPI, no M5 library ──────────────────────────────────
  #include <TFT_eSPI.h>
  extern TFT_eSPI nm28_display;
  #define HAL_DISPLAY  nm28_display

  // Physical screen dimensions (rotation 1 = landscape 320×240)
  #define HAL_SCREEN_W 320
  #define HAL_SCREEN_H 240

  // Sprite (135×240) is centered horizontally on the 320-px-wide screen
  #define HAL_SPR_X    92
  #define HAL_SPR_Y    0

  // Minimal RTC structs that match M5's field names so drawClock compiles
  // unchanged. clockRefreshRtc() returns zeros; clock face stays dormant on NM28.
  struct RTC_TimeTypeDef { uint8_t Hours, Minutes, Seconds; };
  struct RTC_DateTypeDef { uint8_t WeekDay, Month, Date; uint16_t Year; };

  // Color constants that M5StickCPlus.h normally provides via TFT_eSPI
  #ifndef GREEN
  #define GREEN 0x07E0
  #endif
  #ifndef RED
  #define RED   0xF800
  #endif

#else
  // ── M5StickC Plus: original library ─────────────────────────────────────
  #include <M5StickCPlus.h>
  #define HAL_DISPLAY  M5.Lcd
  #define HAL_SCREEN_W 135
  #define HAL_SCREEN_H 240
  #define HAL_SPR_X    0
  #define HAL_SPR_Y    0
  // RTC_TimeTypeDef / RTC_DateTypeDef already provided by M5StickCPlus.h
#endif

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void halBegin();    // replaces M5.begin() + M5.Imu.Init() + M5.Beep.begin()
void halUpdate();   // replaces M5.update() + M5.Beep.update()

// ---------------------------------------------------------------------------
// Buttons
// BtnA = primary (front face on M5, GPIO0 on NM28)
// BtnB = secondary (side on M5, not available on NM28 → always false)
// ---------------------------------------------------------------------------
bool halBtnA_isPressed();
bool halBtnA_wasReleased();
bool halBtnA_pressedFor(uint32_t ms);
bool halBtnB_isPressed();
bool halBtnB_wasPressed();

// ---------------------------------------------------------------------------
// Power / Screen
// ---------------------------------------------------------------------------
void halBrightness(uint8_t level);   // 0..4  replaces M5.Axp.ScreenBreath()
void halScreenOn(bool on);           // replaces M5.Axp.SetLDO2()
void halPowerOff();                  // replaces M5.Axp.PowerOff()
bool halOnUsb();                     // replaces M5.Axp.GetVBusVoltage() > 4
int  halPwrBtnEvent();               // 0x02 = short AXP press; 0 = none (NM28)

// ---------------------------------------------------------------------------
// IMU
// ---------------------------------------------------------------------------
void halImuGetAccel(float* ax, float* ay, float* az);

// ---------------------------------------------------------------------------
// RTC  (uses RTC_TimeTypeDef / RTC_DateTypeDef, real on M5, stub on NM28)
// ---------------------------------------------------------------------------
void halRtcGet(RTC_TimeTypeDef* t, RTC_DateTypeDef* d);

// ---------------------------------------------------------------------------
// Sound  (beep on M5, Serial stub on NM28)
// ---------------------------------------------------------------------------
void halBeep(uint16_t freq, uint16_t dur);
