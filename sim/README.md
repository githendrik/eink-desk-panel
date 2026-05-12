# E-Ink Display Simulator

Quick desktop simulation of the weather display without flashing the ESP32.

## Requirements

- macOS with Xcode Command Line Tools (for g++)
- Python 3 with Pillow

## Setup

```bash
# Install Python dependencies
pip3 install -r requirements.txt

# Or just Pillow
pip3 install Pillow
```

## Usage

### Quick Start (Build + Render + Open)
```bash
make open
```

### Step by Step
```bash
# Build the simulator
make

# Run simulator and generate PNG
make render

# Open the result
open display_preview.png
```

## Workflow

1. Edit `weather_screen.h` in the main `src/` folder
2. Copy changes to `weather_screen_mock.cpp` in `sim/`
3. Run `make open` to see the preview
4. Iterate on the layout

## How It Works

1. `weather_screen_mock.cpp` - Mock implementation of Arduino/ESP32 functions
2. `epd_mock.h` - Mock EPD display functions that collect drawing commands
3. `render.py` - Python script that converts mock data to PNG using Pillow
4. `display_preview.png` - Output image showing what would appear on the e-ink display

## Customization

### Test Different Weather
Edit `weather_screen_mock.cpp`:
```cpp
// In fetch_weather_data() or globals
weather = "Clouds";  // Try: Clear, Clouds, Rain, Thunderstorm, Snow, Mist
temperature = "18";
aareTemp = "14";
aareText = "too cold for swimming";
```

### Test Text Truncation
```cpp
aareText = "This is a very long text that should be truncated properly";
```

## Limitations

- Text rendering uses system fonts (may differ from EPD)
- Weather icons are simple placeholders
- No actual API calls (data is mocked)
- Layout dimensions match real display (400x300)

## Troubleshooting

### "Pillow not found"
```bash
pip3 install Pillow
```

### "g++ not found"
```bash
xcode-select --install
```

### Font warnings
The simulator will fall back to default fonts if Helvetica is not found.
