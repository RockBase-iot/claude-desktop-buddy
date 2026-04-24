// hal_m5stick.cpp — M5StickC Plus HAL implementation
// Only compiled when NM28_BOARD is NOT defined.
#ifndef NM28_BOARD

#include "hal.h"

void halBegin() {
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Imu.Init();
  M5.Beep.begin();
}

void halUpdate() {
  M5.update();
  M5.Beep.update();
}

bool halBtnA_isPressed()             { return M5.BtnA.isPressed(); }
bool halBtnA_wasReleased()           { return M5.BtnA.wasReleased(); }
bool halBtnA_pressedFor(uint32_t ms) { return M5.BtnA.pressedFor(ms); }
bool halBtnB_isPressed()             { return M5.BtnB.isPressed(); }
bool halBtnB_wasPressed()            { return M5.BtnB.wasPressed(); }

void halBrightness(uint8_t level)    { M5.Axp.ScreenBreath(20 + level * 20); }
void halScreenOn(bool on)            { M5.Axp.SetLDO2(on); }
void halPowerOff()                   { M5.Axp.PowerOff(); }
bool halOnUsb()                      { return M5.Axp.GetVBusVoltage() > 4.0f; }
int  halPwrBtnEvent()                { return M5.Axp.GetBtnPress(); }

void halImuGetAccel(float* ax, float* ay, float* az) {
  M5.Imu.getAccelData(ax, ay, az);
}

void halRtcGet(RTC_TimeTypeDef* t, RTC_DateTypeDef* d) {
  M5.Rtc.GetTime(t);
  M5.Rtc.GetDate(d);
}

void halBeep(uint16_t freq, uint16_t dur) {
  M5.Beep.tone(freq, dur);
}

#endif // NM28_BOARD
