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
- Rain forecast detection (next 8 hours)
- AareGuru commentary text
- NVS token persistence for OAuth tokens
- Rocker switch to toggle between Strava/Weight
- NTP time sync for API date calculations
- OTA firmware updates via GitHub Releases
- Web dashboard for configuration
- WiFiManager captive portal
- Status screen (MENU button)
- UV index display (when daily max >= 8)

### Changed
- `weather_screen.h` → `main_screen.h` (all data on one screen)
- Font rendering: built-in EPD fonts (max 48px) → U8g2 fonts (up to 92px)
- Weather provider: OpenWeatherMap → Open-Meteo (no API key, 1km resolution)
- Weather refresh: 1 hour → 15 minutes → 10 minutes
- Pollen: replaced by rain status when precipitation is detected/forecast, or UV when >= 8
- Token management: hardcoded → NVS-persisted with auto-refresh
- Location: Bern → Herrenschwanden (46.9725, 7.4528)
- Location label: "Bern" → "Local"

## File Changes

| File | Status | Notes |
|------|--------|-------|
| `src/main.ino` | Rewritten | Single screen + rocker switch + multi-timer loop + OTA |
| `src/main_screen.h` | New (was weather_screen.h) | All APIs + display logic |
| `src/config_manager.h` | New | NVS configuration management |
| `src/web_dashboard.h` | New | AsyncWebServer dashboard |
| `src/ota_update.h` | New | GitHub Releases OTA |
| `src/spotify_screen.h` | Removed | |
| `src/transport_screen.h` | Removed | |
| `lib/EPD_GUI/EPD_GUI.cpp` | Modified | Added large U8g2 font sizes (50-92px) |
| `scripts/get_strava_credentials.py` | New | Strava OAuth helper |
| `scripts/get_withings_credentials.py` | Kept | Withings OAuth helper |
| `platformio.ini` | Modified | Added deps, partitions, build flags |
| `.github/workflows/build.yml` | New | CI/CD for firmware releases |

## Architecture Decisions

1. **Single screen**: All data visible at once, no navigation needed
2. **NVS for tokens**: OAuth refresh tokens are single-use; must persist new ones across reboots
3. **U8g2 fonts**: Built-in EPD fonts max at 48px; U8g2 provides arbitrary sizes via Adafruit GFX bridge
4. **Google Pollen API for real plant data (using GRASS type instead of PM2.5 proxy)
5. **Rocker switch for toggle**: Single screen but Strava/Weight share the bottom-right slot
6. **Rain overrides pollen**: More actionable information when precipitation is imminent
7. **Open-Meteo over OpenWeatherMap**: No API key, 1km Swiss resolution (MeteoSwiss ICON-CH1), free tier sufficient
8. **UV threshold at 8**: WHO "very high" — lower thresholds triggered too often due to daily max vs current mismatch
