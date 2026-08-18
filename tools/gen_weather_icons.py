#!/usr/bin/env python3
"""Generate 1-bit weather icon bitmaps for the e-ink panel.

CURRENTLY UNUSED. The icons were dropped from the 4.2" layout because drawing
them via EPD_ShowPicture() correlated with an intermittent boot crash (jump into
the heap during WiFi bring-up) that was not root-caused. Kept so the icon set can
be restored once that is fixed.

Icons are drawn as vectors in a normalised 0..1 space, rendered with 8x
supersampling, then thresholded to 1bpp and emitted as a C header.

Bit convention matches EPD_ShowPicture() in lib/EPD_GUI/EPD_GUI.cpp:
  bit == 0  ->  black (ink)
  bit == 1  ->  white (background)
Widths must be a multiple of 8 (EPD_ShowPicture advances 8 pixels per byte
without a bounds check inside the byte loop).

Usage:  python3 tools/gen_weather_icons.py > include/weather_icons.h
"""

import sys
from PIL import Image, ImageDraw

SS = 8                  # supersampling factor
SIZES = [48, 24]        # generated icon sizes (must be multiples of 8)


# --- drawing helpers (normalised 0..1 coordinates) -------------------------

class Canvas:
    def __init__(self, size):
        self.n = size * SS
        self.img = Image.new("L", (self.n, self.n), 255)
        self.d = ImageDraw.Draw(self.img)

    def _p(self, v):
        return v * self.n

    def circle(self, cx, cy, r, fill=0):
        n = self.n
        self.d.ellipse(
            [(cx - r) * n, (cy - r) * n, (cx + r) * n, (cy + r) * n], fill=fill
        )

    def ring(self, cx, cy, r, w):
        self.circle(cx, cy, r, 0)
        self.circle(cx, cy, r - w, 255)

    def rect(self, x0, y0, x1, y1, fill=0):
        n = self.n
        self.d.rectangle([x0 * n, y0 * n, x1 * n, y1 * n], fill=fill)

    def line(self, x0, y0, x1, y1, w):
        n = self.n
        self.d.line([x0 * n, y0 * n, x1 * n, y1 * n], fill=0, width=int(w * n))

    def poly(self, pts, fill=0):
        n = self.n
        self.d.polygon([(x * n, y * n) for x, y in pts], fill=fill)

    def to_bits(self, size):
        img = self.img.resize((size, size), Image.LANCZOS)
        return img.point(lambda v: 0 if v < 128 else 1, "1")


def cloud(c, cx=0.50, cy=0.62, s=1.0, fill=0):
    """A rounded cloud centred on (cx, cy), scaled by s."""
    c.circle(cx - 0.20 * s, cy - 0.02 * s, 0.150 * s, fill)
    c.circle(cx - 0.02 * s, cy - 0.13 * s, 0.200 * s, fill)
    c.circle(cx + 0.21 * s, cy - 0.01 * s, 0.155 * s, fill)
    c.rect(cx - 0.35 * s, cy - 0.02 * s, cx + 0.36 * s, cy + 0.15 * s, fill)
    c.circle(cx - 0.35 * s, cy + 0.07 * s, 0.085 * s, fill)
    c.circle(cx + 0.36 * s, cy + 0.07 * s, 0.085 * s, fill)


def cloud_outlined(c, **kw):
    """Cloud with a white halo, so it reads when overlapping the sun."""
    cloud(c, fill=255, s=kw.get("s", 1.0) * 1.10, cx=kw.get("cx", 0.5),
          cy=kw.get("cy", 0.62))
    cloud(c, **kw)


def sun(c, cx=0.5, cy=0.5, r=0.20, rays=True):
    c.circle(cx, cy, r)
    if not rays:
        return
    import math
    for i in range(8):
        a = i * math.pi / 4
        x0, y0 = cx + math.cos(a) * (r + 0.06), cy + math.sin(a) * (r + 0.06)
        x1, y1 = cx + math.cos(a) * (r + 0.16), cy + math.sin(a) * (r + 0.16)
        c.line(x0, y0, x1, y1, 0.055)


def drops(c, y=0.80, slant=0.05):
    for x in (0.28, 0.50, 0.72):
        c.line(x + slant, y, x - slant, y + 0.15, 0.055)


def flakes(c, y=0.86):
    import math
    for x in (0.28, 0.50, 0.72):
        for i in range(3):
            a = i * math.pi / 3
            dx, dy = math.cos(a) * 0.090, math.sin(a) * 0.090
            c.line(x - dx, y - dy, x + dx, y + dy, 0.030)


# --- icon definitions ------------------------------------------------------

def icon_clear_day(c):
    sun(c, 0.5, 0.5, 0.22)


def icon_clear_night(c):
    c.circle(0.52, 0.48, 0.30)
    c.circle(0.68, 0.36, 0.27, 255)


def icon_partly_day(c):
    sun(c, 0.34, 0.34, 0.15)
    cloud_outlined(c, cx=0.56, cy=0.66, s=0.92)


def icon_partly_night(c):
    c.circle(0.36, 0.34, 0.19)
    c.circle(0.46, 0.26, 0.17, 255)
    cloud_outlined(c, cx=0.56, cy=0.66, s=0.92)


def icon_cloudy(c):
    cloud(c, cx=0.50, cy=0.55, s=1.15)


def icon_fog(c):
    cloud(c, cx=0.50, cy=0.44, s=1.0)
    for i, y in enumerate((0.72, 0.83, 0.94)):
        inset = 0.10 if i == 1 else 0.04
        c.line(0.14 + inset, y, 0.86 - inset, y, 0.055)


def icon_rain(c):
    cloud(c, cx=0.50, cy=0.48, s=1.0)
    drops(c, y=0.76)


def icon_snow(c):
    cloud(c, cx=0.50, cy=0.44, s=1.0)
    flakes(c, y=0.84)


def icon_thunder(c):
    cloud(c, cx=0.50, cy=0.42, s=1.0)
    c.poly([(0.55, 0.60), (0.36, 0.86), (0.48, 0.86),
            (0.42, 1.00), (0.64, 0.72), (0.51, 0.72)])


ICONS = [
    ("clear_day", icon_clear_day),
    ("clear_night", icon_clear_night),
    ("partly_day", icon_partly_day),
    ("partly_night", icon_partly_night),
    ("cloudy", icon_cloudy),
    ("fog", icon_fog),
    ("rain", icon_rain),
    ("snow", icon_snow),
    ("thunder", icon_thunder),
]


def pack(bits, size):
    """Pack a 1-bit PIL image into MSB-first row-major bytes."""
    px = bits.load()
    out = []
    for y in range(size):
        for xb in range(size // 8):
            byte = 0
            for b in range(8):
                byte = (byte << 1) | (1 if px[xb * 8 + b, y] else 0)
            out.append(byte)
    return out


def emit(size, name, data, fh):
    fh.write(f"// {name} {size}x{size}\n")
    fh.write(f"const uint8_t icon_{name}_{size}[] PROGMEM = {{\n")
    per_row = size // 8
    for i in range(0, len(data), per_row):
        row = ", ".join(f"0x{b:02X}" for b in data[i:i + per_row])
        fh.write(f"  {row},\n")
    fh.write("};\n\n")


def main():
    fh = sys.stdout
    fh.write("// Generated by tools/gen_weather_icons.py -- do not edit by hand.\n")
    fh.write("// Regenerate with: python3 tools/gen_weather_icons.py > include/weather_icons.h\n")
    fh.write("#ifndef WEATHER_ICONS_H\n#define WEATHER_ICONS_H\n\n")
    fh.write("#include <stdint.h>\n#include <pgmspace.h>\n\n")
    for size in SIZES:
        fh.write(f"#define WEATHER_ICON_{size}_W {size}\n")
        fh.write(f"#define WEATHER_ICON_{size}_H {size}\n")
    fh.write("\n")

    for size in SIZES:
        for name, fn in ICONS:
            c = Canvas(size)
            fn(c)
            emit(size, name, pack(c.to_bits(size), size), fh)

    # index table + WMO mapping
    names = [n for n, _ in ICONS]
    fh.write("enum WeatherIcon {\n")
    for i, n in enumerate(names):
        fh.write(f"  WI_{n.upper()} = {i},\n")
    fh.write(f"  WI_COUNT = {len(names)}\n}};\n\n")

    for size in SIZES:
        fh.write(f"const uint8_t* const weatherIcons{size}[WI_COUNT] = {{\n")
        for n in names:
            fh.write(f"  icon_{n}_{size},\n")
        fh.write("};\n\n")

    fh.write("""// Map a WMO weather code (Open-Meteo) to an icon index.
// isDay selects the day/night variant for clear and partly-cloudy states.
inline uint8_t weatherIconForCode(int code, bool isDay) {
  switch (code) {
    case 0:            return isDay ? WI_CLEAR_DAY : WI_CLEAR_NIGHT;
    case 1:
    case 2:            return isDay ? WI_PARTLY_DAY : WI_PARTLY_NIGHT;
    case 3:            return WI_CLOUDY;
    case 45: case 48:  return WI_FOG;
    case 71: case 73: case 75: case 77:
    case 85: case 86:  return WI_SNOW;
    case 95: case 96: case 99: return WI_THUNDER;
    default:
      if (code >= 51 && code <= 67) return WI_RAIN;
      if (code >= 80 && code <= 82) return WI_RAIN;
      return WI_CLOUDY;
  }
}

#endif // WEATHER_ICONS_H
""")


if __name__ == "__main__":
    main()
