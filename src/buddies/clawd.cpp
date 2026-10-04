#include "../buddy.h"
#include "../buddy_common.h"
#include <M5StickCPlus.h>
#include <string.h>

// Clawd — the Claude Code mascot, drawn as pixel art with filled rects.
// Grid is 11 x 8 units: body 9x6, arms 1x2 on each side, four legs 1x2.

namespace clawd {

static const uint16_t ORANGE_C = 0xDBAA;   // #D97757
static const uint16_t PINK     = 0xF8B2;

enum Eye : uint8_t { E_OPEN, E_CLOSED, E_WIDE, E_HAPPY, E_X, E_HEART, E_DOWN };

struct Pose {
  int8_t  dx = 0, dy = 0;       // body offset in 1x pixels
  uint8_t armL = 0, armR = 0;   // 0 down, 1 mid, 2 up
  Eye     eye = E_OPEN;
  int8_t  look = 0;             // -1 / 0 / +1 eye shift in units
  uint8_t legs = 0;             // 0 flat, 1 left pair lifted, 2 right pair lifted
};

static void heart(int px, int py, int hs, uint16_t c) {
  static const char* const H[4] = { ".#.#.", "#####", ".###.", "..#.." };
  for (int r = 0; r < 4; r++)
    for (int k = 0; k < 5; k++)
      if (H[r][k] == '#') buddyFillRect(px + k * hs, py + r * hs, hs, hs, c);
}

static void draw(const Pose& p) {
  const int s  = buddyScale();
  const int u  = (s == 2) ? 8 : 3;
  const int h  = u / 2 ? u / 2 : 1;
  const int ox = BUDDY_X_CENTER - (11 * u) / 2 + p.dx * s;
  const int oy = 36 + p.dy * s;
  auto R = [&](int gx, int gy, int gw, int gh, uint16_t c) {
    buddyFillRect(ox + gx * u, oy + gy * u, gw * u, gh * u, c);
  };

  // legs
  for (int i = 0; i < 4; i++) {
    bool lift = (p.legs == 1 && i < 2) || (p.legs == 2 && i >= 2);
    R(2 + 2 * i, 6, 1, lift ? 1 : 2, ORANGE_C);
  }
  // body + arms
  R(1, 0, 9, 6, ORANGE_C);
  static const int8_t ARM_Y[3] = { 3, 2, -1 };
  R(0,  ARM_Y[p.armL], 1, 2, ORANGE_C);
  R(10, ARM_Y[p.armR], 1, 2, ORANGE_C);

  // eyes
  const int lx = 3 + p.look, rx = 7 + p.look;
  const uint16_t K = BUDDY_BG;
  switch (p.eye) {
    case E_OPEN:   R(lx, 1, 1, 2, K); R(rx, 1, 1, 2, K); break;
    case E_WIDE:   R(lx, 1, 1, 3, K); R(rx, 1, 1, 3, K); break;
    case E_DOWN:   R(lx, 2, 1, 2, K); R(rx, 2, 1, 2, K); break;
    case E_CLOSED: {
      int t = u / 3 ? u / 3 : 1, y = oy + 2 * u + u / 2;
      buddyFillRect(ox + lx * u, y, u, t, K);
      buddyFillRect(ox + rx * u, y, u, t, K);
      break;
    }
    case E_HAPPY: {            // ^ shape on a 4x2 half-unit grid
      for (int e = 0; e < 2; e++) {
        int x0 = ox + (e ? rx : lx) * u - h, y0 = oy + 1 * u + h;
        buddyFillRect(x0 + h,     y0,     2 * h, h, K);
        buddyFillRect(x0,         y0 + h, h,     h, K);
        buddyFillRect(x0 + 3 * h, y0 + h, h,     h, K);
      }
      break;
    }
    case E_X: {                // X on a 3x3 half-unit grid
      for (int e = 0; e < 2; e++) {
        int x0 = ox + (e ? rx : lx) * u - h / 2, y0 = oy + 1 * u + h / 2;
        buddyFillRect(x0,         y0,         h, h, K);
        buddyFillRect(x0 + 2 * h, y0,         h, h, K);
        buddyFillRect(x0 + h,     y0 + h,     h, h, K);
        buddyFillRect(x0,         y0 + 2 * h, h, h, K);
        buddyFillRect(x0 + 2 * h, y0 + 2 * h, h, h, K);
      }
      break;
    }
    case E_HEART: {
      int hs = (s == 2) ? 3 : 1;
      heart(ox + lx * u - hs, oy + u, hs, PINK);
      heart(ox + rx * u - hs, oy + u, hs, PINK);
      break;
    }
  }
}

// ─── SLEEP ─── closed eyes, slow breathing, drifting z
static void doSleep(uint32_t t) {
  Pose p; p.eye = E_CLOSED; p.dy = (t / 4) & 1;
  draw(p);
  int p1 = t % 10, p2 = (t + 4) % 10, p3 = (t + 7) % 10;
  buddySetColor(BUDDY_DIM);
  buddySetCursor(BUDDY_X_CENTER + 20 + p1, BUDDY_Y_OVERLAY + 18 - p1 * 2);
  buddyPrint("z");
  buddySetColor(BUDDY_CYAN);
  buddySetCursor(BUDDY_X_CENTER + 26 + p2, BUDDY_Y_OVERLAY + 14 - p2);
  buddyPrint("Z");
  buddySetColor(BUDDY_DIM);
  buddySetCursor(BUDDY_X_CENTER + 16 + p3 / 2, BUDDY_Y_OVERLAY + 10 - p3 / 2);
  buddyPrint("z");
}

// ─── IDLE ─── ~5s loop: look around, blink, little wave
static void doIdle(uint32_t t) {
  Pose p;
  uint8_t ph = (t / 4) % 24;
  if (ph == 14 || ph == 21)              p.eye = E_CLOSED;
  else if (ph == 10 || ph == 11)         p.look = -1;
  else if (ph == 12 || ph == 13)         p.look = 1;
  else if (ph == 18 || ph == 19) {
    p.armR = (t & 1) ? 1 : 2;
    p.dy = -1;
  }
  draw(p);
}

// ─── BUSY ─── typing arms, eyes scanning down, code glyphs overhead
static void doBusy(uint32_t t) {
  Pose p;
  p.eye  = E_DOWN;
  p.look = (int8_t)((t / 3) % 3) - 1;
  p.armL = (t & 1) ? 1 : 0;
  p.armR = (t & 1) ? 0 : 1;
  p.legs = (t / 2) & 1 ? 1 : 2;
  draw(p);
  static const char* const G[] = { "</>", "{ }", "01 ", "...", "=> ", "#!/" };
  buddySetColor(BUDDY_GREEN);
  buddySetCursor(BUDDY_X_CENTER - 9, BUDDY_Y_OVERLAY);
  buddyPrint(G[(t / 2) % 6]);
}

// ─── ATTENTION ─── wide eyes, arms up, shaking, flashing !
static void doAttention(uint32_t t) {
  Pose p;
  p.eye  = E_WIDE;
  p.armL = p.armR = 2;
  p.dx   = (t & 1) ? 1 : -1;
  p.legs = (t & 1) ? 1 : 2;
  p.look = ((t / 6) % 3) - 1;
  draw(p);
  buddySetColor((t / 2) & 1 ? BUDDY_YEL : BUDDY_RED);
  buddySetCursor(BUDDY_X_CENTER - 3, BUDDY_Y_OVERLAY);
  buddyPrint("!");
  if ((t / 3) & 1) {
    buddySetColor(BUDDY_RED);
    buddySetCursor(BUDDY_X_CENTER + 14, BUDDY_Y_OVERLAY + 4);
    buddyPrint("!");
    buddySetCursor(BUDDY_X_CENTER - 20, BUDDY_Y_OVERLAY + 4);
    buddyPrint("!");
  }
}

// ─── CELEBRATE ─── jumping, happy eyes, confetti
static void doCelebrate(uint32_t t) {
  static const int8_t JUMP[8] = { 0, -6, -10, -6, 0, 0, -3, 0 };
  Pose p;
  p.eye  = E_HAPPY;
  p.dy   = JUMP[(t / 2) % 8];
  p.armL = p.armR = (p.dy < 0) ? 2 : 1;
  p.legs = (p.dy < 0) ? 1 : 0;
  draw(p);
  static const uint16_t cols[] = { BUDDY_YEL, BUDDY_CYAN, BUDDY_GREEN, BUDDY_WHITE, BUDDY_PURPLE };
  for (int i = 0; i < 6; i++) {
    int phase = (t * 2 + i * 11) % 22;
    int x = BUDDY_X_CENTER - 36 + i * 14;
    int y = BUDDY_Y_OVERLAY - 6 + phase;
    if (y > BUDDY_Y_BASE + 20 || y < 0) continue;
    buddySetColor(cols[i % 5]);
    buddySetCursor(x, y);
    buddyPrint((i + (int)(t / 2)) & 1 ? "+" : "*");
  }
}

// ─── DIZZY ─── wobble, X eyes, orbiting ? and x
static void doDizzy(uint32_t t) {
  Pose p;
  p.eye  = E_X;
  p.dx   = (t & 1) ? 2 : -2;
  p.armL = (t & 1) ? 1 : 2;
  p.armR = (t & 1) ? 2 : 1;
  p.legs = (t & 1) ? 1 : 2;
  draw(p);
  static const int8_t OX[] = { 0, 5, 7, 5, 0, -5, -7, -5 };
  static const int8_t OY[] = { -5, -3, 0, 3, 5, 3, 0, -3 };
  uint8_t p1 = t % 8, p2 = (t + 4) % 8;
  buddySetColor(BUDDY_YEL);
  buddySetCursor(BUDDY_X_CENTER + OX[p1] * 2 - 2, BUDDY_Y_OVERLAY + 6 + OY[p1]);
  buddyPrint("?");
  buddySetColor(BUDDY_RED);
  buddySetCursor(BUDDY_X_CENTER + OX[p2] * 2 - 2, BUDDY_Y_OVERLAY + 6 + OY[p2]);
  buddyPrint("x");
}

// ─── HEART ─── heart eyes, bobbing, rising hearts
static void doHeart(uint32_t t) {
  Pose p;
  p.eye  = E_HEART;
  p.dy   = (t / 3) & 1 ? -1 : 0;
  p.armL = p.armR = 1;
  draw(p);
  const int s = buddyScale();
  const int hs = (s == 2) ? 2 : 1;
  for (int i = 0; i < 4; i++) {
    int phase = (t + i * 4) % 16;
    int y = (BUDDY_Y_OVERLAY + 24 - phase * 2) * s;
    if (y < 0 || y > 34 * s) continue;
    int x = BUDDY_X_CENTER + (-24 + i * 16 + ((phase / 3) & 1) * 2) * s - 5 * hs / 2;
    heart(x, y, hs, PINK);
  }
}

}  // namespace clawd

extern const Species CLAWD_SPECIES = {
  "clawd",
  0xDBAA,
  { clawd::doSleep, clawd::doIdle, clawd::doBusy, clawd::doAttention,
    clawd::doCelebrate, clawd::doDizzy, clawd::doHeart }
};
