# Version 2 Changes

## Overview

v2 is a complete redesign from a multi-screen navigation app to a single-screen information dashboard with large temperature fonts and multiple data sources.

## What Changed from v1

### Removed
- Spotify "Now Playing" screen
- Public transport departures screen
- Multi-screen navigation with buttons
- Screen indicator dots

### Added
- Large 78px temperature fonts (via U8g2 Logisoso fonts)
- Withings weight tracking with 6-month trend
- Strava last activity display
- Rain forecast detection (24h ahead)
- AareGuru commentary text
- NVS token persistence for OAuth tokens
- Rocker switch to toggle between Strava/Weight
- NTP time sync for API date calculations

### Changed
- `weather_screen.h` → `main_screen.h` (all data on one screen)
- Font rendering: built-in EPD fonts (max 48px) → U8g2 fonts (up to 92px)
- Weather refresh: 1 hour → 15 minutes
- Pollen: replaced by rain status when precipitation is detected/forecast
- Token management: hardcoded → NVS-persisted with auto-refresh

## File Changes

| File | Status | Notes |
|------|--------|-------|
| `src/main.ino` | Rewritten | Single screen + rocker switch + multi-timer loop |
| `src/main_screen.h` | New (was weather_screen.h) | All APIs + display logic |
| `src/spotify_screen.h` | Removed | |
| `src/transport_screen.h` | Removed | |
| `lib/EPD_GUI/EPD_GUI.cpp` | Modified | Added large U8g2 font sizes (50-92px) |
| `scripts/get_strava_credentials.py` | New | Strava OAuth helper |
| `scripts/get_withings_credentials.py` | Kept | Withings OAuth helper |
| `scripts/convert_svg_to_epd.py` | Removed | Leftover from bitmap digit attempt |
| `scripts/generate_digits.py` | Removed | Leftover from bitmap digit attempt |
| `platformio.ini` | Modified | Added Adafruit GFX, U8g2, Preferences deps |

## Architecture Decisions

1. **Single screen**: All data visible at once, no navigation needed
2. **NVS for tokens**: OAuth refresh tokens are single-use; must persist new ones across reboots
3. **U8g2 fonts**: Built-in EPD fonts max at 48px; U8g2 provides arbitrary sizes via Adafruit GFX bridge
4. **PM2.5 as pollen proxy**: Real pollen API (MeteoSwiss) not available until Q2 2026
5. **Rocker switch for toggle**: Single screen but Strava/Weight share the bottom-right slot
6. **Rain overrides pollen**: More actionable information when precipitation is imminent
