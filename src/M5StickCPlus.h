// Compatibility shim: lets the upstream M5StickC Plus firmware build on an
// ESP32-S3 + GC9A01 round display + one capacitive touch pad.
//
//   M5.Lcd   -> plain TFT_eSPI (same base class as TFT_eSprite)
//   M5.Axp   -> backlight PWM; battery/USB readings are stubs
//   M5.Imu   -> stub (device always "flat, still"): no shake/nap/rotation
//   M5.Rtc   -> software clock seeded by the bridge's {"time":[...]} message
//   M5.Beep  -> optional passive buzzer (BUDDY_BUZZER_PIN)
//   BtnA/B   -> gestures on a single touch pad:
//                 tap          = BtnA click   (next / approve)
//                 hold 600 ms  = BtnB press   (select / deny), fires at the threshold
//                 double tap   = menuGesture  (open / close the menu)
//               With BUDDY_BTNB_PIN set, a second touch module is BtnB
//               (select / deny) and BtnA reacts instantly (no double-tap wait).
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Colour names the M5 library exposed globally
#define BLACK   TFT_BLACK
#define WHITE   TFT_WHITE
#define RED     TFT_RED
#define GREEN   TFT_GREEN
#define BLUE    TFT_BLUE
#define YELLOW  TFT_YELLOW
#define CYAN    TFT_CYAN
#define MAGENTA TFT_MAGENTA
#define ORANGE  TFT_ORANGE

#ifndef BUDDY_BL_PIN
#define BUDDY_BL_PIN 7
#endif
#ifndef BUDDY_TOUCH_PIN
#define BUDDY_TOUCH_PIN 4
#endif

struct RTC_TimeTypeDef { uint8_t Hours, Minutes, Seconds; };
struct RTC_DateTypeDef { uint8_t WeekDay, Month, Date; uint16_t Year; };

struct AxpShim {
  void ScreenBreath(uint8_t level);        // 0..100
  void SetLDO2(bool on);                   // backlight on/off
  void PowerOff();
  uint8_t GetBtnPress() { return 0; }
  float GetBatVoltage()   { return 0; }
  float GetBatCurrent()   { return 0; }
  float GetVBusVoltage()  { return 5.0f; }  // treated as always on USB power
  float GetTempInAXP192() { return temperatureRead(); }
 private:
  uint8_t _level = 100;
};

struct ImuShim {
  void Init() {}
  void getAccelData(float* x, float* y, float* z) { *x = 0; *y = 0; *z = 1.0f; }
};

struct RtcShim {
  void SetTime(RTC_TimeTypeDef* t);
  void SetDate(RTC_DateTypeDef* d);
  void GetTime(RTC_TimeTypeDef* t);
  void GetDate(RTC_DateTypeDef* d);
 private:
  RTC_TimeTypeDef _t = {0, 0, 0};
  RTC_DateTypeDef _d = {0, 1, 1, 2000};
  bool     _set = false;
  int64_t  _baseLocal = 0;   // local epoch seconds at _baseMs
  uint32_t _baseMs = 0;
  int64_t  now() const;
};

struct BeepShim {
  void begin();
  void tone(uint16_t freq, uint16_t durMs);
  void update();
 private:
  uint32_t _until = 0;
};

// Gesture-synthesised buttons. BtnA owns the raw touch state; BtnB only
// ever reports wasPressed() (double tap).
struct VButton {
  bool isPressed() const;
  bool wasPressed() const  { return _wasPressed; }
  bool wasReleased() const { return _wasReleased; }
  bool pressedFor(uint32_t ms) const;
 private:
  friend struct M5Class;
  bool _raw = false, _wasPressed = false, _wasReleased = false, _holdOk = false;
  uint32_t _downAt = 0;
};

struct M5Class {
  TFT_eSPI Lcd;
  AxpShim  Axp;
  ImuShim  Imu;
  RtcShim  Rtc;
  BeepShim Beep;
  VButton  BtnA, BtnB;
  bool     menuGesture = false;   // one-button mode: double tap, true for one update()
  void begin();
  void update();
};

extern M5Class M5;
