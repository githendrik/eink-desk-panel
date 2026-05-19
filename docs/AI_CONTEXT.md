# AI Agent Context - E-Ink Desk Panel v2

## Purpose

This document provides context for AI agents working on this project.

## IMPORTANT: Keep Docs Updated

**After completing any task, always update the relevant docs in `docs/` to reflect changes.** This ensures context is preserved across sessions and models. Key files to check:

- `AI_CONTEXT.md` — Update if new pins, coordinates, patterns, or pitfalls are discovered
- `PROJECT_OVERVIEW.md` — Update if architecture, layout, APIs, or refresh intervals change
- `API_REFERENCE.md` — Update if API endpoints, auth, or response fields change
- `V2_CHANGES.md` — Update if significant features are added or removed
- `WITHINGS_SETUP.md` — Update if token management or setup steps change
- `GETTING_STARTED.md` — Update if setup steps change
- `RELEASE_PROCEDURE.md` — Reference for creating new firmware releases

## Critical Facts

- **Display**: 400x300 pixels, e-ink (SSD1683), SPI
- **Board**: Freenove ESP32-S3 WROOM (CrowPanel 4.2")
- **Serial port**: `/dev/cu.usbserial-110` (can change to `-210` on replug)
- **Upload command**: `~/.platformio/penv/bin/pio run -t upload` (plain `pio` not in PATH)
- **Serial monitor**: `pio device monitor` broken; use Python `serial.Serial` instead
- **User location**: Bern, Switzerland
- **WiFi SSID**: `Hello@richert.li`

## File Map

| File | Purpose |
|------|---------|
| `src/main.ino` | Entry point, WiFiManager setup, NTP, OTA boot check, rocker switch ISR, fetch/display loop |
| `src/main_screen.h` | API fetch functions, data parsing, display rendering, OTA progress screen, AP mode screen |
| `src/config_manager.h` | ConfigManager class: per-service NVS load/save/clear |
| `src/web_dashboard.h` | AsyncWebServer dashboard HTML + endpoints (/, /save, /reset, /status, /check-update, /apply-update) |
| `src/ota_update.h` | OTA check (GitHub API) + download + apply via Update library, progress callback |
| `partitions.csv` | nvs 20KB + otadata 8KB + app0 2MB + app1 2MB |
| `lib/EPD_GUI/EPD_GUI.cpp` | Font rendering with U8g2 integration, drawing primitives |
| `lib/EPD_GUI/EPD_GUI.h` | GUI function declarations |
| `lib/EPD_GUI/EPD_font.h` | Built-in bitmap fonts (max 48px) |
| `lib/EPD/EPD.h` | Display driver (EPD_Init, EPD_Display_Part, etc.) |
| `platformio.ini` | Board config, library deps, custom partitions, build flags |
| `.github/workflows/build.yml` | CI: build on v* tag push, create release with .bin |
| `docs/OTA_AND_CONFIG_PLAN.md` | Full implementation plan with design decisions and phase roadmap |

## Code Conventions

- **Functions**: `snake_case` (e.g., `fetch_weather_data`, `display_main_screen`)
- **Variables**: `camelCase` (e.g., `withingsAccessToken`, `bottomRightMode`)
- **Constants**: `UPPER_CASE` (e.g., `EPD_W`, `WIFI_SSID`, `PRV_KEY`)
- **HTTP pattern**: `WiFiClient`/`WiFiClientSecure` + `HTTPClient`, always call `http.end()`
- **JSON parsing**: `Arduino_JSON` library, check `JSON.typeof()` before accessing
- **Token storage**: NVS via `Preferences` library (wrapped by ConfigManager)
- **NVS namespaces**: `wifi`, `openweather`, `withings`, `strava`, `firmware`
- **NVS key limit**: 15 chars max (e.g., `client_sec` not `client_secret`)

## Display Screens

### Main Screen
```
EPD_W = 400, EPD_H = 300

leftCenter  = midX/2 - 10  = 90   (left column center)
rightCenter = midX + midX/2 - 10 = 290 (right column center)
midX = 200

Temperature Y: 30 (78px font, Logisoso numbers-only)
Degree symbol: next to temp, 16px "o"
Location labels ("Bern"/"Aare"): Y=115, 16px
AareGuru text: Y=170, 16px, centered at midX
topHeight = EPD_H - 70 = 230
bottomY = topHeight + 10 = 240
Bottom values: 24px
Bottom labels: Y = bottomY + 28, 12px
```

### OTA Progress Screen (`display_ota_screen`)
- Title "Updating Firmware" at Y=60, 24px
- Version transition (e.g., "v0.2.2 -> v0.2.3") at Y=100, 16px
- Progress bar: 280x30px centered at Y=150, outline + filled
- Percentage at Y=200, 24px
- "Do not power off!" warning at Y=250, 12px
- Updated at 25% intervals via `otaSetProgressCallback`

### AP Mode Screen (`display_ap_screen`)
- "WiFi Setup" title at Y=40, 24px
- Step 1: "Connect to WiFi:" + SSID ("EinkPanel") at Y=90/115
- Step 2: "Open browser:" + IP at Y=160/185
- Step 3: "Select your WiFi network" at Y=230
- Triggered by WiFiManager AP callback

### Status Screen (`display_status_screen()`)
- Shows: WiFi SSID, IP, signal strength (dBm + text), firmware version, uptime
- OTA section: "Press OK to check" → "Checking..." → "Update available: vX.Y.Z / Press OK to install" or "Firmware is up to date"
- MENU button or rocker to go back to main screen
- OK button triggers OTA check; if update found, second OK press applies it

## Buttons

| GPIO | Button | Function |
|------|--------|----------|
| 6 | PRV_KEY (rocker left) | Toggle bottomRightMode |
| 4 | NEXT_KEY (rocker right) | Toggle bottomRightMode |
| 1 | MENU | Toggle status screen |
| 2 | HOME | Unused |
| 5 | OK | On status screen: check for OTA / apply update |

All buttons: `INPUT_PULLUP`, ISR on `FALLING`, 200ms debounce.

## OTA Update System

- GitHub releases API (public repo, no auth needed)
- Check: boot + manual from dashboard `/check-update` + MENU button status screen (OK to check/apply)
- `/check-update` uses deferred pattern: first call sets flag, main loop runs check, second call returns cached result (async handler can't do blocking HTTPS)
- `/apply-update` sets `otaTriggered` flag, main loop applies
- Progress callback updates e-ink at 25% intervals
- Rollback: `esp_ota_mark_app_valid_cancel_rollback()` after WiFi + NTP succeed
- `FIRMWARE_VERSION` must have `v` prefix to match GitHub tags
- Version set via platformio.ini build flag: `-DFIRMWARE_VERSION='"v0.2.3"'`
- CI `sed` injects version from git tag into platformio.ini

## Token Management Pattern

Both Withings and Strava use OAuth2 with **single-use refresh tokens**:

1. On boot: load tokens from NVS via ConfigManager
2. On API call: if 401, refresh token and retry (max 1 retry)
3. On successful refresh: save BOTH access_token AND refresh_token to NVS
4. Withings token endpoint: `POST https://wbsapi.withings.net/v2/oauth2` with `action=requesttoken`
   - NOT `oauth2.withings.com` (DNS fails on ESP32)
   - Response wraps tokens in `body` object, check `status` field for errors
5. Strava token endpoint: `POST https://www.strava.com/oauth/token`
6. All HTTPS clients use `client.setInsecure()` and `http.setTimeout(10000)`
7. All credentials managed via web dashboard at `http://eink-panel.local`

## Common Pitfalls

1. `pio` command not in PATH — use `~/.platformio/penv/bin/pio`
2. Serial port changes on USB replug — check `ls /dev/cu.usb*`
3. Withings refresh tokens are single-use — must save new one on every refresh
4. Recursive retry on token failure causes stack overflow — always limit retries
5. `EPD_ShowPicture` bitmap format: MSB first, inverted color logic
6. Numbers-only fonts (`_tn`) can't render degree symbol — use separate small "o"
7. ESP32 `time_t` from NTP — check `now < 1000000000` before using timestamps
8. ESPAsyncWebServer handlers must not do blocking HTTPS — defer to main loop via flags
9. NVS keys max 15 chars — use abbreviations (e.g., `client_sec`)
10. `FIRMWARE_VERSION` must include `v` prefix to match GitHub tag names

## Debugging

- Serial output only available at boot (no real-time monitor working)
- Reset device via DTR toggle for serial capture: `s.setDTR(False); sleep(0.1); s.setDTR(True)`
- Or use Python: `serial.Serial('/dev/cu.usbserial-110', 115200, timeout=2)`
- All API responses are printed to Serial for debugging
- Web dashboard `/status` endpoint returns health JSON
