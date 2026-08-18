// Host-side preview for the 4.2" layout.
//
// Unlike the old mock, this compiles the REAL src/screen_42.h so the layout
// code under test is the same code that runs on the device. Only the EPD
// primitives and the Arduino String type are stubbed.
//
// Text advance widths mirror the u8g2 fonts selected in EPD_GUI.cpp:
//   size <= 12 -> 7x13_tf   (fixed 7px advance)
//   size <= 16 -> 9x15_tf   (fixed 9px advance)
//   size <= 24 -> helvR14_tf (proportional, approximated)
//   size <= 78 -> logisoso62_tn (digits, fixed 34px advance)
// The fixed-width cases are exact, which is what the right-alignment and
// truncation logic in screen_42.h depends on.

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>

// ---- Arduino shims --------------------------------------------------------
struct String : std::string {
  String() {}
  String(const char* s) : std::string(s ? s : "") {}
  String(const std::string& s) : std::string(s) {}
  const char* c_str() const { return std::string::c_str(); }
  size_t length() const { return std::string::length(); }
};
#define PROGMEM
#define BLACK 0
#define WHITE 0xFF

// ---- framebuffer ----------------------------------------------------------
static const int W = 400, H = 300;
static uint8_t fb[W * H];  // 1 byte per pixel, 0 = ink

struct TextOp { int x, y, size; std::string s; };
static std::vector<TextOp> texts;

static void px(int x, int y) {
  if (x < 0 || x >= W || y < 0 || y >= H) return;
  fb[y * W + x] = 0;
}

// ---- EPD_GUI stubs --------------------------------------------------------
static int advance(int size) {
  if (size <= 8)  return 6;
  if (size <= 12) return 7;
  if (size <= 16) return 9;
  if (size <= 24) return 8;   // helvR14 average
  if (size <= 48) return 10;  // helvR18 average
  if (size <= 62) return 28;
  return 34;                  // logisoso62 digits
}

// Count UTF-8 code points, not bytes (umlauts are 2 bytes).
static int glyphCount(const char* s) {
  int n = 0;
  for (const unsigned char* p = (const unsigned char*)s; *p; p++)
    if ((*p & 0xC0) != 0x80) n++;
  return n;
}

int EPD_GetUTF8TextWidth(const char* s, uint16_t size) {
  return glyphCount(s) * advance(size);
}

void EPD_ShowStringUTF8(uint16_t x, uint16_t y, const char* s, uint16_t size, uint16_t) {
  texts.push_back({(int)x, (int)y, (int)size, std::string(s)});
}

void EPD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t) {
  int dx = abs((int)x1 - (int)x0), dy = abs((int)y1 - (int)y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;
  int x = x0, y = y0;
  for (;;) {
    px(x, y);
    if (x == (int)x1 && y == (int)y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x += sx; }
    if (e2 <  dx) { err += dx; y += sy; }
  }
}

void EPD_DrawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t, uint8_t mode) {
  if (mode) {
    for (int y = y0; y <= (int)y1; y++)
      for (int x = x0; x <= (int)x1; x++) px(x, y);
  } else {
    EPD_DrawLine(x0, y0, x1, y0, 0); EPD_DrawLine(x0, y1, x1, y1, 0);
    EPD_DrawLine(x0, y0, x0, y1, 0); EPD_DrawLine(x1, y0, x1, y1, 0);
  }
}

// Mirrors EPD_ShowPicture(): draws at (x+1, y+1); a 0 bit is ink.
void EPD_ShowPicture(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy,
                     const uint8_t bmp[], uint16_t) {
  x += 1; y += 1;
  int bytesPerRow = sx / 8;
  for (int r = 0; r < (int)sy; r++)
    for (int b = 0; b < bytesPerRow; b++) {
      uint8_t v = bmp[r * bytesPerRow + b];
      for (int t = 0; t < 8; t++)
        if (!((v << t) & 0x80)) px(x + b * 8 + t, y + r);
    }
}

// ---- data the layout reads ------------------------------------------------
#include "data_stub.h"

// The real layout, compiled verbatim.
#include "../src/screen_42.h"

// ---- output ---------------------------------------------------------------
int main() {
  memset(fb, 0xFF, sizeof(fb));
  draw_layout_42();

  FILE* f = fopen("sim42.pgm", "wb");
  fprintf(f, "P5\n%d %d\n255\n", W, H);
  fwrite(fb, 1, sizeof(fb), f);
  fclose(f);

  f = fopen("sim42_text.txt", "w");
  for (auto& t : texts) fprintf(f, "%d\t%d\t%d\t%s\n", t.x, t.y, t.size, t.s.c_str());
  fclose(f);

  printf("wrote sim42.pgm (%d text ops)\n", (int)texts.size());
  return 0;
}
