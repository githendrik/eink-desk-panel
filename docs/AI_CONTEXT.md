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
| `src/main.ino` | Entry point, WiFi/NTP setup, fetch/display loop, rocker switch ISR |
| `src/main_screen.h` | All API fetch functions, data parsing, display rendering |
| `src/credentials.h` | API keys + tokens (gitignored) |
| `lib/EPD_GUI/EPD_GUI.cpp` | Font rendering with U8g2 integration, drawing primitives |
| `lib/EPD_GUI/EPD_GUI.h` | GUI function declarations |
| `lib/EPD_GUI/EPD_font.h` | Built-in bitmap fonts (max 48px) |
| `lib/EPD/EPD.h` | Display driver (EPD_Init, EPD_Display_Part, etc.) |
| `platformio.ini` | Board config, library deps, port settings |

## Code Conventions

- **Functions**: `snake_case` (e.g., `fetch_weather_data`, `display_main_screen`)
- **Variables**: `camelCase` (e.g., `withingsAccessToken`, `bottomRightMode`)
- **Constants**: `UPPER_CASE` (e.g., `EPD_W`, `WIFI_SSID`, `PRV_KEY`)
- **HTTP pattern**: `WiFiClient`/`WiFiClientSecure` + `HTTPClient`, always call `http.end()`
- **JSON parsing**: `Arduino_JSON` library, check `JSON.typeof()` before accessing
- **Token storage**: NVS via `Preferences` library, namespaces: `"withings"`, `"strava"`

## Display Layout Coordinates

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

## Rocker Switch

- GPIO 6 (`PRV_KEY`): previous button
- GPIO 4 (`NEXT_KEY`): next button
- Both configured with `INPUT_PULLUP` + ISR on `FALLING` edge
- 200ms debounce
- Currently toggles `bottomRightMode` between 0 (Strava) and 1 (Weight, default)

## Token Management Pattern

Both Withings and Strava use OAuth2 with **single-use refresh tokens**:

1. On boot: load tokens from NVS, fall back to `credentials.h`
2. On API call: if 401, refresh token and retry (max 1 retry)
3. On successful refresh: save BOTH access_token AND refresh_token to NVS
4. Withings token endpoint: `POST https://wbsapi.withings.net/v2/oauth2` with `action=requesttoken`
   - NOT `oauth2.withings.com` (DNS fails on ESP32)
   - Response wraps tokens in `body` object, check `status` field for errors
5. Strava token endpoint: `POST https://www.strava.com/oauth/token`
6. All HTTPS clients use `client.setInsecure()` and `http.setTimeout(10000)`

## Withings API Specifics

- Endpoint: `POST https://wbsapi.withings.net/measure` with `action=getmeas`
- Auth: `Bearer` token in header
- Body: form-urlencoded with `startdate` and `enddate` (Unix timestamps)
- NO `userid` parameter
- Response: `body.measuregrps[0].measures[0].value` (divide by 1000 for kg)
- Measurement date: `body.measuregrps[0].date` (Unix timestamp)
- Shows "STALE" if last measurement > 7 days old
- Weight trend: compares latest vs oldest measurement in 6-month window

## Rain Detection

- Current rain: check `weather[0].id` from current weather (200-599 = precipitation)
- Forecast rain: `GET /data/2.5/forecast?cnt=8` (8 slots × 3h = 24h ahead)
- When rain detected, replaces pollen display with "Raining" or "Rain in Xh"

## Strava API

- Endpoint: `GET https://www.strava.com/api/v3/athlete/activities?per_page=1&page=1`
- Auth: `Bearer` token in header
- Access tokens expire every 6 hours
- Shows: activity type + distance (e.g. "Run 5.2km") with date (e.g. "12 May")
- Activity types mapped to short names (MountainBikeRide→MTB, TrailRun→Trail, etc.)

## Common Pitfalls

1. `pio` command not in PATH — use `~/.platformio/penv/bin/pio`
2. Serial port changes on USB replug — check `ls /dev/cu.usb*`
3. Withings refresh tokens are single-use — must save new one on every refresh
4. Recursive retry on token failure causes stack overflow — always limit retries
5. `EPD_ShowPicture` bitmap format: MSB first, inverted color logic
6. Numbers-only fonts (`_tn`) can't render degree symbol — use separate small "o"
7. ESP32 `time_t` from NTP — check `now < 1000000000` before using timestamps

## Debugging

- Serial output only available at boot (no real-time monitor working)
- Reset device via DTR toggle for serial capture: `s.setDTR(False); sleep(0.1); s.setDTR(True)`
- Or use Python: `serial.Serial('/dev/cu.usbserial-110', 115200, timeout=2)`
- All API responses are printed to Serial for debugging

## Credentials Template

```cpp
#define WIFI_SSID "..."
#define WIFI_PASSWORD "..."
#define OPENWEATHER_API_KEY "..."
#define WITHINGS_CLIENT_ID "..."
#define WITHINGS_CLIENT_SECRET "..."
#define WITHINGS_ACCESS_TOKEN "..."
#define WITHINGS_REFRESH_TOKEN "..."
#define WITHINGS_USER_ID "..."
#define STRAVA_CLIENT_ID "..."
#define STRAVA_CLIENT_SECRET "..."
#define STRAVA_ACCESS_TOKEN "..."
#define STRAVA_REFRESH_TOKEN "..."
```
