#include "M5StickCPlus.h"

M5Class M5;

// ---------------------------------------------------------------- backlight
static const uint8_t BL_CH = 0;

void AxpShim::ScreenBreath(uint8_t level) {
  if (level > 100) level = 100;
  _level = level;
  ledcWrite(BL_CH, (uint32_t)level * 255 / 100);
}
void AxpShim::SetLDO2(bool on) {
  ledcWrite(BL_CH, on ? (uint32_t)_level * 255 / 100 : 0);
}
void AxpShim::PowerOff() {
  ledcWrite(BL_CH, 0);
  M5.Lcd.writecommand(0x10);   // SLPIN
  delay(50);
  esp_deep_sleep_start();      // wake with the reset button
}

// ---------------------------------------------------------------------- RTC
// Days since 1970-01-01 for a civil date (Howard Hinnant's algorithm).
static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int64_t)doe - 719468;
}

void RtcShim::SetTime(RTC_TimeTypeDef* t) { _t = *t; }
void RtcShim::SetDate(RTC_DateTypeDef* d) {
  // data.h always calls SetTime then SetDate: this is the commit point.
  _d = *d;
  _baseLocal = daysFromCivil(_d.Year, _d.Month, _d.Date) * 86400
             + _t.Hours * 3600 + _t.Minutes * 60 + _t.Seconds;
  _baseMs = millis();
  _set = true;
}
int64_t RtcShim::now() const { return _baseLocal + (millis() - _baseMs) / 1000; }

void RtcShim::GetTime(RTC_TimeTypeDef* t) {
  if (!_set) { *t = {0, 0, 0}; return; }
  int64_t s = now() % 86400;
  t->Hours = s / 3600; t->Minutes = (s / 60) % 60; t->Seconds = s % 60;
}
void RtcShim::GetDate(RTC_DateTypeDef* d) {
  if (!_set) { *d = {0, 1, 1, 2000}; return; }
  time_t e = (time_t)now();
  struct tm lt; gmtime_r(&e, &lt);
  d->WeekDay = lt.tm_wday; d->Month = lt.tm_mon + 1;
  d->Date = lt.tm_mday;    d->Year = lt.tm_year + 1900;
}

// ------------------------------------------------------------------- buzzer
#ifdef BUDDY_BUZZER_PIN
static const uint8_t BZ_CH = 1;
void BeepShim::begin() { ledcSetup(BZ_CH, 2000, 8); ledcAttachPin(BUDDY_BUZZER_PIN, BZ_CH); ledcWrite(BZ_CH, 0); }
void BeepShim::tone(uint16_t f, uint16_t d) { ledcWriteTone(BZ_CH, f); _until = millis() + d; }
void BeepShim::update() { if (_until && (int32_t)(millis() - _until) >= 0) { ledcWriteTone(BZ_CH, 0); ledcWrite(BZ_CH, 0); _until = 0; } }
#else
void BeepShim::begin() {}
void BeepShim::tone(uint16_t, uint16_t) {}
void BeepShim::update() {}
#endif

// ------------------------------------------------------------------ buttons
static const uint32_t TAP_LONG_MS   = 600;   // hold threshold (matches upstream)
static const uint32_t TAP_DOUBLE_MS = 300;   // window to wait for a 2nd tap

bool VButton::isPressed() const { return _raw; }
bool VButton::pressedFor(uint32_t ms) const {
  return _raw && _holdOk && (millis() - _downAt) >= ms;
}

// ESP32-S3 touch: the reading RISES when touched. Track an idle baseline and
// compare against a threshold relative to it.
static uint32_t touchBase = 0;
#ifndef BUDDY_TOUCH_PCT
#define BUDDY_TOUCH_PCT 4          // % above baseline that counts as a touch
#endif
#ifndef BUDDY_TOUCH_MIN_DELTA
#define BUDDY_TOUCH_MIN_DELTA 400
#endif

static uint32_t touchThreshold() {
  uint32_t d = touchBase * BUDDY_TOUCH_PCT / 100;
  return touchBase + (d > BUDDY_TOUCH_MIN_DELTA ? d : BUDDY_TOUCH_MIN_DELTA);
}

// Debounced reader for one touch input. Digital mode = external module
// (TTP223 & co, push-pull output). Native mode = bare pad on an S3 touch channel
// (reading RISES when touched); only supported for the primary button.
struct TouchIn {
  int8_t  pin;
  bool    digital;
  bool    state;
  uint8_t streak;
  TouchIn(int8_t p, bool d) : pin(p), digital(d), state(false), streak(0) {}
};

static bool readTouch(TouchIn& in, bool primary) {
  bool over;
  if (in.digital) {
#ifdef BUDDY_TOUCH_ACTIVE_LOW
    over = digitalRead(in.pin) == LOW;
#else
    over = digitalRead(in.pin) == HIGH;
#endif
#ifdef BUDDY_TOUCH_DEBUG
    static uint32_t lastDbgD[2] = {0, 0};
    uint32_t& ld = lastDbgD[primary ? 0 : 1];
    if (millis() - ld > 500) {
      ld = millis();
      Serial.printf("[touch %s] digital=%d %s\n", primary ? "A" : "B", digitalRead(in.pin), over ? "TOUCH" : "");
    }
#endif
  } else {
    uint32_t v = touchRead(in.pin);
    over = v > touchThreshold();
    if (!over) touchBase += ((int32_t)v - (int32_t)touchBase) / 64;   // slow drift tracking
#ifdef BUDDY_TOUCH_DEBUG
    static uint32_t lastDbgN = 0;
    if (millis() - lastDbgN > 500) {
      lastDbgN = millis();
      Serial.printf("[touch A] raw=%u base=%u thr=%u %s\n", v, touchBase, touchThreshold(), over ? "TOUCH" : "");
    }
#endif
  }
  // 2-sample debounce in both directions
  if (over == in.state) in.streak = 0;
  else if (++in.streak >= 2) { in.state = over; in.streak = 0; }
  return in.state;
}

#ifdef BUDDY_TOUCH_DIGITAL
static const bool A_DIGITAL = true;
#else
static const bool A_DIGITAL = false;
#endif
static TouchIn inA(BUDDY_TOUCH_PIN, A_DIGITAL);
#ifdef BUDDY_BTNB_PIN
static TouchIn inB(BUDDY_BTNB_PIN, true);
#endif

void M5Class::begin() {
  Serial.begin(115200);

  ledcSetup(BL_CH, 5000, 8);
  ledcAttachPin(BUDDY_BL_PIN, BL_CH);
  ledcWrite(BL_CH, 0);

#ifdef BUDDY_DEBUG_LOG
  delay(1500);   // let the host open the USB serial port before the first lines
  Serial.printf("[m5] begin: SCLK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d A=%d B=%d\n",
                (int)TFT_SCLK, (int)TFT_MOSI, (int)TFT_CS, (int)TFT_DC, (int)TFT_RST,
                (int)BUDDY_BL_PIN, (int)BUDDY_TOUCH_PIN,
#ifdef BUDDY_BTNB_PIN
                (int)BUDDY_BTNB_PIN
#else
                -1
#endif
                );
#endif
  Lcd.init();
#ifdef BUDDY_DEBUG_LOG
  Serial.println("[m5] tft.init() returned");
#endif
#ifdef BUDDY_LCD_INVERT
  Lcd.invertDisplay(true);
#endif
  Lcd.setRotation(0);
  Lcd.fillScreen(TFT_BLACK);
#ifdef BUDDY_BOOT_TEST
  Axp.ScreenBreath(100);
  { const uint16_t c[3] = { TFT_RED, TFT_GREEN, TFT_BLUE };
    for (int i = 0; i < 3; i++) { Lcd.fillScreen(c[i]); delay(500); }
    Lcd.fillScreen(TFT_BLACK); }
#endif
  Axp.ScreenBreath(25);   // low at boot: limits inrush current (main.cpp restores user brightness)

  if (A_DIGITAL) {
    pinMode(BUDDY_TOUCH_PIN, INPUT);
  } else {
    // Calibrate the idle level of the bare pad (don't touch it at boot).
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) { sum += touchRead(BUDDY_TOUCH_PIN); delay(10); }
    touchBase = sum / 16;
  }
#ifdef BUDDY_BTNB_PIN
  pinMode(BUDDY_BTNB_PIN, INPUT);
#endif
}

void M5Class::update() {
  uint32_t now = millis();
  bool raw = readTouch(inA, true);

  BtnA._wasPressed = BtnA._wasReleased = false;
  BtnB._wasPressed = BtnB._wasReleased = false;

#ifdef BUDDY_BTNB_PIN
  // ---- Two physical buttons: A = next/approve (hold = menu), B = select/deny.
  static bool prevA = false, prevB = false;
  bool rawB = readTouch(inB, false);

  if (raw && !prevA)       { BtnA._downAt = now; BtnA._holdOk = true; }
  else if (!raw && prevA)  { BtnA._wasReleased = true; BtnA._holdOk = false; }
  if (rawB && !prevB)      { BtnB._wasPressed = true; }
  if (!rawB && prevB)      { BtnB._wasReleased = true; }
  prevA = raw; prevB = rawB;
  BtnA._raw = raw;
  BtnB._raw = rawB;
#else
  // ---- One button: tap = A, hold = B, double tap = menu.
  static bool prev = false, pendingTap = false, secondTap = false, holdFired = false;
  static uint32_t tapReleasedAt = 0;
  BtnA._holdOk = false;                                  // no "hold A" in this mode
  menuGesture = false;

  if (raw && !prev) {                                    // finger down
    if (pendingTap && now - tapReleasedAt < TAP_DOUBLE_MS) {
      pendingTap = false; secondTap = true;
      menuGesture = true;                                // double tap
    } else {
      secondTap = false;
    }
    BtnA._downAt = now;
    holdFired = false;
  } else if (!raw && prev) {                             // finger up
    if (secondTap)       secondTap = false;
    else if (holdFired)  holdFired = false;              // hold already acted as B
    else { pendingTap = true; tapReleasedAt = now; }     // maybe a double tap
  }
  if (raw && !secondTap && !holdFired && now - BtnA._downAt >= TAP_LONG_MS) {
    holdFired = true;
    BtnB._wasPressed = true;                             // long press = B
  }
  if (pendingTap && !raw && now - tapReleasedAt >= TAP_DOUBLE_MS) {
    pendingTap = false;
    BtnA._wasReleased = true;                            // confirmed single tap
  }
  prev = raw;
  BtnA._raw = raw;
#endif
}
