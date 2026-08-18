#!/usr/bin/env python3
"""Render the output of sim42 to a PNG preview.

sim42 writes two files:
  sim42.pgm       - the real 400x300 framebuffer (lines, rectangles, bars)
  sim42_text.txt  - text draw ops (x, y, size, string), tab separated

Text is composited here using a monospace font scaled so its advance matches
the u8g2 advance that sim42.cpp used for layout, so alignment in the preview
reflects what the device computes.
"""

import sys
from PIL import Image, ImageDraw, ImageFont

# Must mirror advance() in sim42.cpp, which mirrors the font buckets in
# EPD_GUI.cpp. Use thresholds, not exact keys: a size of e.g. 32 falls in the
# "<= 48" bucket and must not silently hit a default.
ADVANCE_BUCKETS = [(8, 6), (12, 7), (16, 9), (24, 8), (48, 10), (62, 28), (78, 34)]


def advance_for(size):
    for limit, adv in ADVANCE_BUCKETS:
        if size <= limit:
            return adv
    return 34

FONT_CANDIDATES = [
    "/System/Library/Fonts/Supplemental/Andale Mono.ttf",
    "/System/Library/Fonts/Menlo.ttc",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
]


def font_path():
    import os
    for p in FONT_CANDIDATES:
        if os.path.exists(p):
            return p
    raise SystemExit("No monospace font found; edit FONT_CANDIDATES in render_42.py")


_cache = {}


def font_for(size):
    adv = advance_for(size)
    if adv not in _cache:
        path = font_path()
        best, best_err = None, None
        for s in range(6, 130):
            f = ImageFont.truetype(path, s)
            err = abs(f.getlength("0") - adv)
            if best_err is None or err < best_err:
                best, best_err = f, err
        _cache[adv] = best
    return _cache[adv]


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "sim42_preview.png"
    img = Image.open("sim42.pgm").convert("RGB")
    draw = ImageDraw.Draw(img)

    with open("sim42_text.txt", encoding="utf-8") as fh:
        for line in fh:
            if not line.strip():
                continue
            x, y, size, text = line.rstrip("\n").split("\t", 3)
            x, y, size = int(x), int(y), int(size)
            # EPD_ShowStringUTF8 sets the cursor to (x, y + size) = baseline
            draw.text((x, y + size), text, font=font_for(size), fill="black", anchor="ls")

    img.resize((img.width * 2, img.height * 2), Image.NEAREST).save(out)
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
