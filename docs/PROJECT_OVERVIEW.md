# E-Ink Desk Panel - Project Overview

## Project Description

A single-screen ESP32-S3 e-ink desk panel displaying weather, river temperature, pollen/rain, weight, and Strava activity data. Temperatures are the dominant visual element using large 78px fonts.

## Hardware

- **Board**: Freenove ESP32-S3 WROOM (CrowPanel 4.2" e-ink)
- **Display**: 400x300 E-Paper (E-Ink) via SPI
- **Input**: Rocker switch (GPIO 6 = previous, GPIO 4 = next)
- **Connection**: USB serial (`/dev/cu.usbserial-110`) at 115200 baud
- **Upload speed**: 460800 baud

## Display Layout

```
┌──────────────────────────────────────┐
│                                      │
│    23°              11°              │  <- 78px Logisoso font
│    Bern             Aare             │  <- 16px labels
│                                      │
│         Geduld, geduld               │  <- AareGuru text (16px, centered)
│                                      │
│──────────────────────────────────────│
│   low            82.3 kg            │  <- 24px values
│  Pollen           stable            │  <- 12px labels
└──────────────────────────────────────┘
```

- Top 2/3: Two large temperatures (Bern air + Aare river) side by side
- Middle: AareGuru commentary text (centered, truncated if too long)
- Bottom left: Pollen level (or rain status when rain is current/forecast)
- Bottom right: Weight + trend (or Strava activity, toggled via rocker switch)

## Architecture

### Directory Structure

```
eink-desk-panel/
├── src/
│   ├── main.ino              # Entry point, WiFi, NTP, fetch/display loop, rocker switch
│   ├── main_screen.h         # All display + API fetch logic
│   ├── credentials.h         # API keys and tokens (gitignored)
│   └── credentials.h.example # Template for credentials
├── lib/
│   ├── EPD/                  # E-Paper Display driver
│   ├── EPD_GUI/              # GUI utilities (fonts, shapes, text, U8g2 integration)
│   └── EPD_SPI/              # SPI communication layer
├── scripts/
│   ├── get_withings_credentials.py  # OAuth2 flow for Withings
│   ├── get_strava_credentials.py    # OAuth2 flow for Strava
│   └── test_withings_api.py         # Debug script for Withings API
├── docs/                     # Documentation
└── platformio.ini            # Build config
```

### Key Components

1. **EPD Driver** (`lib/EPD/`): Low-level e-paper display control (SSD1683)
2. **EPD_GUI** (`lib/EPD_GUI/`): GUI functions with U8g2 font integration for large fonts
3. **EPD_SPI** (`lib/EPD_SPI/`): SPI communication abstraction
4. **Main Screen** (`src/main_screen.h`): All API fetching, data parsing, and display rendering

## API Integrations

| API | Data | Refresh | Protocol |
|-----|------|---------|----------|
| OpenWeatherMap (weather) | Temperature | 15 min | HTTP GET |
| OpenWeatherMap (air pollution) | PM2.5 as pollen proxy | 15 min | HTTP GET |
| OpenWeatherMap (forecast) | Rain forecast 24h | 15 min | HTTP GET |
| AareGuru | River temp + text | 15 min | HTTP GET |
| Withings | Weight + 6-month trend | 6 hours | HTTPS POST |
| Strava | Last activity | 1 hour | HTTPS GET |

## Token Management

Both Withings and Strava use OAuth2 with single-use refresh tokens:
- Tokens are stored in **NVS (ESP32 flash)** to persist across reboots
- On startup, tokens are loaded from NVS (falling back to `credentials.h` values)
- On successful refresh, both new access_token and refresh_token are saved to NVS
- Token refresh includes retry logic (max 1 retry) to prevent infinite loops

## Refresh Intervals

- Weather + Pollen + Rain + Aare: every 15 minutes
- Strava: every 1 hour
- Weight: every 6 hours

## Development Environment

- **Framework**: PlatformIO with Arduino framework
- **Board config**: `freenove_esp32_s3_wroom`
- **Libraries**: Adafruit GFX, U8g2, U8g2_for_Adafruit_GFX, Arduino_JSON, Preferences
- **Upload**: `~/.platformio/penv/bin/pio run -t upload`
- **Serial port**: `/dev/cu.usbserial-110` (may change on replug)

## Font System

Built-in EPD fonts max out at 48px. For larger temperatures, we use U8g2 fonts via `U8g2_for_Adafruit_GFX`, rendered through a custom `EPD_GFX` Adafruit_GFX subclass that writes pixels into the EPD paint buffer.

Font size ladder in `EPD_GUI.cpp`:
- `<=8`: `u8g2_font_6x10_tf`
- `<=12`: `u8g2_font_7x13_tf`
- `<=16`: `u8g2_font_9x15_tf`
- `<=24`: `u8g2_font_helvR14_tf`
- `<=48`: `u8g2_font_helvR18_tf`
- `<=62`: `u8g2_font_logisoso50_tf` (full charset)
- `<=78`: `u8g2_font_logisoso62_tn` (numbers only)
- `<=92`: `u8g2_font_logisoso78_tn` (numbers only)
- `>92`: `u8g2_font_logisoso92_tn` (numbers only)

The `_tn` suffix means numbers-only; degree symbols are rendered separately as a small "o".
