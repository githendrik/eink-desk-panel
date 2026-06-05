# E-Ink Desk Panel - Project Overview

## Project Description

A multi-panel ESP32-S3 e-ink desk panel displaying weather, river temperature, pollen/rain/UV, weight, and Strava activity data. Temperatures are the dominant visual element using large 78px fonts. Supports both 4.2" (400x300) and 5.79" (792x272) CrowPanel displays from a single codebase.

## Hardware

| Spec | 4.2" Panel | 5.79" Panel |
|------|-----------|-------------|
| **Board** | ESP32-S3 WROOM | ESP32-S3 WROOM-1-N8R8 |
| **Display** | 400x300 (single SSD1683) | 792x272 (dual SSD1683 cascade) |
| **PSRAM** | — | 8MB |
| **Input** | Rocker, MENU, OK, HOME | Rocker, MENU, Back, Reset, Boot |
| **SPI pins** | SCK=12, MOSI=11, RES=47, DC=46, CS=45, BUSY=48 | Same |
| **Connection** | USB serial at 115200 baud | Same |

## Multi-Panel Architecture

The codebase uses compile-time `#ifdef PANEL_579` to select between panels:

- **EPD Driver**: `EPD_42.cpp` (single IC, row-major) vs `EPD_579.cpp` (dual IC, column-major cascade)
- **EPD_GUI**: Shared, with conditional 8-pixel gap handling for the 5.79" IC junction
- **Layout**: `LAYOUT_W` / `LAYOUT_H` / `PANEL_ROTATION` constants per panel
- **Framebuffer**: `EPD_BUF_SIZE` — 15000 bytes (4.2") or 27200 bytes (5.79")
- **OTA**: Each panel pulls its own binary (`firmware-42.bin` or `firmware-579.bin`)

PlatformIO environments:
```bash
pio run -e panel-42    # 4.2" panel
pio run -e panel-579   # 5.79" panel
pio run                # builds both
```

## Display Layout

```
┌──────────────────────────────────────┐
│                                      │
│    23°              11°              │  <- 78px Logisoso font
│    Local            Aare             │  <- 16px labels
│                                      │
│         Geduld, geduld               │  <- AareGuru text (16px, centered)
│                                      │
│──────────────────────────────────────│
│   low            82.3 kg            │  <- 24px values
│  pollen           stable            │  <- 12px labels
└──────────────────────────────────────┘
```

- Top 2/3: Two large temperatures (local air + Aare river) side by side
- Middle: AareGuru commentary text (centered, truncated if too long)
- Bottom left: Priority display — Rain > UV (>=8) > Pollen
- Bottom right: Weight + trend (or Strava activity, toggled via rocker switch)

## Architecture

### Directory Structure

```
eink-desk-panel/
├── src/
│   ├── main.ino              # Entry point, WiFi, NTP, fetch/display loop, rocker switch
│   ├── main_screen.h         # All display + API fetch logic (per-panel layout)
│   ├── config_manager.h      # ConfigManager class: per-service NVS load/save/clear
│   ├── web_dashboard.h       # AsyncWebServer dashboard HTML + endpoints
│   ├── ota_update.h          # OTA check + download + apply (per-panel asset matching)
│   ├── remote_log.h          # Discord webhook remote logging
│   ├── credentials.h         # API keys and tokens (gitignored)
│   └── credentials.h.example # Template for credentials
├── lib/
│   ├── EPD/                  # E-Paper Display driver
│   │   ├── EPD.h             # Unified header (#ifdef PANEL_579 for constants)
│   │   ├── EPD_42.cpp        # 4.2" single-SSD1683 driver
│   │   └── EPD_579.cpp       # 5.79" dual-SSD1683 cascade driver
│   ├── EPD_GUI/              # GUI utilities (fonts, shapes, text, U8g2 integration)
│   └── EPD_SPI/              # SPI communication layer (shared)
├── scripts/
│   ├── get_withings_credentials.py  # OAuth2 flow for Withings
│   ├── get_strava_credentials.py    # OAuth2 flow for Strava
│   └── test_withings_api.py         # Debug script for Withings API
├── docs/                     # Documentation
├── .github/workflows/        # CI: builds both panels, creates release
└── platformio.ini            # Build config (two environments: panel-42, panel-579)
```

### Key Components

1. **EPD Driver** (`lib/EPD/`): Low-level e-paper display control (SSD1683). Two implementations selected at compile time:
   - `EPD_42.cpp`: Single SSD1683, row-major data, 400x300
   - `EPD_579.cpp`: Dual SSD1683 cascade, column-major data, 800x272 virtual (792x272 visible)
2. **EPD_GUI** (`lib/EPD_GUI/`): GUI functions with U8g2 font integration for large fonts. Handles 8-pixel gap at IC junction for 5.79" panel.
3. **EPD_SPI** (`lib/EPD_SPI/`): SPI communication abstraction (shared between panels)
4. **Main Screen** (`src/main_screen.h`): All API fetching, data parsing, and display rendering. Uses `LAYOUT_W`/`LAYOUT_H`/`PANEL_ROTATION` for per-panel layout.
5. **Config Manager** (`src/config_manager.h`): NVS-backed persistent configuration (API tokens, mDNS hostname, log level)
6. **Web Dashboard** (`src/web_dashboard.h`): Local web UI for token management, mDNS config, logging, and OTA
7. **OTA Update** (`src/ota_update.h`): GitHub Releases-based firmware updates (per-panel asset matching)
8. **Remote Log** (`src/remote_log.h`): Optional Discord webhook logging with configurable level

## API Integrations

| API | Data | Refresh | Protocol |
|-----|------|---------|----------|
| Open-Meteo (weather) | Temperature, rain, UV index | 10 min | HTTPS GET |
| Open-Meteo (air quality) | PM2.5 as pollen proxy | 10 min | HTTPS GET |
| AareGuru | River temp + text | 10 min | HTTP GET |
| Withings | Weight + 6-month trend | 6 hours | HTTPS POST |
| Strava | Last activity | 1 hour | HTTPS GET |

## Token Management

Both Withings and Strava use OAuth2 with single-use refresh tokens:
- Tokens are stored in **NVS (ESP32 flash)** to persist across reboots
- On startup, tokens are loaded from NVS (falling back to `credentials.h` values)
- On successful refresh, both new access_token and refresh_token are saved to NVS
- Token refresh includes retry logic (max 1 retry) to prevent infinite loops
- Credentials can be managed via web dashboard at `http://eink-panel.local`

## Refresh Intervals

- Weather + Pollen + UV + Aare: every 10 minutes
- Strava: every 1 hour
- Weight: every 6 hours

## Development Environment

- **Framework**: PlatformIO with Arduino framework
- **Board config**: `freenove_esp32_s3_wroom`
- **Environments**: `panel-42` (4.2") and `panel-579` (5.79")
- **Libraries**: Adafruit GFX, U8g2, U8g2_for_Adafruit_GFX, Arduino_JSON, Preferences, ESPAsyncWebServer, QRCode
- **Upload**: `~/.platformio/penv/bin/pio run -e <env> -t upload`
- **Serial ports**: Configured per environment in `platformio.ini`

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
