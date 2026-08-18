# 4.2" Layout Simulator

Renders the 4.2" panel layout on the desktop, without flashing the ESP32.

Unlike the previous mock, this compiles the **real** `src/screen_42.h`, so the
layout code being previewed is the same code that runs on the device. Only the
EPD primitives (`EPD_ShowStringUTF8`, `EPD_DrawLine`, `EPD_DrawRectangle`,
`EPD_ShowPicture`) and the Arduino `String` type are stubbed.

## Usage

```bash
cd sim
make open        # build + render + open the preview
make render      # build + write sim42_preview.png
make clean
```

Requires `g++` and Pillow.

If `make` reports that Pillow is missing, either install it into your own
python3:

```bash
pip3 install -r requirements.txt
```

or create a self-contained virtualenv for the simulator:

```bash
make venv        # creates ./.venv with Pillow
make open
```

`make` auto-detects: it uses `python3` when that has Pillow, otherwise `./.venv`.
Override explicitly with `make open PYTHON=/path/to/python3`.

## Files

| File | Purpose |
|---|---|
| `sim42.cpp` | EPD primitive stubs + a real 400x300 framebuffer; includes `../src/screen_42.h` |
| `data_stub.h` | Sample weather + calendar data (edit this to test other states) |
| `render_42.py` | Composites the framebuffer and the text draw ops into a PNG |
| `EPD_GUI.h`, `pgmspace.h` | Empty stubs so the device headers resolve on the host |

## Accuracy

Text **advance widths** are modelled on the u8g2 fonts that `EPD_GUI.cpp`
selects by size:

| size | u8g2 font | advance |
|---|---|---|
| <= 12 | `7x13_tf` | 7 px (exact) |
| <= 16 | `9x15_tf` | 9 px (exact) |
| <= 24 | `helvR14_tf` | ~8 px (approximate, proportional) |
| <= 78 | `logisoso62_tn` | 34 px, digits only (approximate) |

The fixed-width cases are exact, which is what the right-alignment and
truncation logic in `screen_42.h` relies on. Glyph *shapes* in the preview come
from a system monospace font and will not match the device exactly — use this to
check layout and alignment, not typography.

## Testing other states

Edit `data_stub.h`, e.g. to check the no-events path:

```c
static int calendarTotalEvents = 0;
```

or negative temperatures, long event titles (truncation), missing sunrise, etc.
