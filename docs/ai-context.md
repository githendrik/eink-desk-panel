# AI Agent Context - E-Ink Desk Panel

## Purpose

This document provides context for AI agents working on this project.

## IMPORTANT: Keep Docs Updated

**After completing any task, always update the relevant docs in `docs/` to reflect changes.** This ensures context is preserved across sessions and models. Key files to check:

- `ai-context.md` — Update if new pins, coordinates, patterns, or pitfalls are discovered
- `project-overview.md` — Update if architecture, layout, APIs, or refresh intervals change
- `api-reference.md` — Update if API endpoints, auth, or response fields change
- `v2-changes.md` — Update if significant features are added or removed
- `oauth-setup.md` — Update if token management or setup steps change
- `getting-started.md` — Update if setup steps change
- `release-procedure.md` — Reference for creating new firmware releases

## Critical Facts

- **Panels**: Two supported panels, built from one codebase
  - 4.2" CrowPanel: 400x300, single SSD1683, rotation 0
  - 5.79" CrowPanel: 792x272 (800x272 virtual), dual SSD1683 cascade, rotation 180
- **Board**: Freenove ESP32-S3 WROOM (both panels)
- **Build**: `~/.platformio/penv/bin/pio run -e panel-42` or `-e panel-579` (plain `pio` not in PATH)
- **Serial ports**: Configured in platformio.ini per environment (typically `/dev/cu.usbserial-110` or `-1110`)
- **Serial monitor**: `pio device monitor` broken in this environment; use Python `serial.Serial` instead
- **User location**: Herrenschwanden, Switzerland (46.9725, 7.4528)
- **mDNS**: Configurable via web dashboard, defaults to `eink-panel.local`

## Multi-Panel Architecture

| Aspect | 4.2" Panel | 5.79" Panel |
|--------|-----------|-------------|
| Build env | `panel-42` | `panel-579` |
| Build flag | (none) | `-DPANEL_579` |
| Resolution | 400x300 | 800x272 virtual (792x272 visible) |
| EPD Driver | `EPD_42.cpp` (row-major) | `EPD_579.cpp` (column-major cascade) |
| Rotation | 0 | 180 |
| Framebuffer | 15000 bytes (stack) | 27200 bytes (stack) + prevFrame in PSRAM |
| Layout width | 400px | 396px (left half, right half currently blank) |
| OTA asset | `firmware-42.bin` | `firmware-579.bin` |
| PSRAM | not used | enabled (`-DBOARD_HAS_PSRAM`) |

Key `#define` constants per panel (from `main_screen.h`):
- `PANEL_ROTATION` — 0 or 180
- `LAYOUT_W` — 400 or 396
- `LAYOUT_H` — 300 or 272

EPD_GUI `Paint_SetPixel` has a conditional 8-pixel gap offset at x=396 for the 5.79" IC junction (`#ifdef PANEL_579`).

## 5.79" Panel: Right Half Status

The right half (x=396..791, i.e. 396px wide) is currently **blank/white**. The user plans to add additional stats there. When implementing:
- Content at logical x >= 396 will automatically cross the IC junction gap (handled by `Paint_SetPixel`)
- The gap offset means logical x=396 maps to physical column 404 (8 invisible pixels in between)
- Available space: 396x272 pixels

## File Map

| File | Purpose |
|------|---------|
| `src/main.ino` | Entry point, WiFiManager setup, NTP, OTA boot check, rocker switch ISR, fetch/display loop |
| `src/main_screen.h` | API fetch functions, data parsing, display rendering, per-panel layout (`LAYOUT_W`/`PANEL_ROTATION`) |
| `src/config_manager.h` | ConfigManager class: per-service NVS load/save/clear (incl. mDNS hostname) |
| `src/web_dashboard.h` | AsyncWebServer dashboard HTML + endpoints (/, /save, /reset, /status, /check-update, /apply-update, /reboot) |
| `src/ota_update.h` | OTA check (GitHub API) + download + apply, per-panel asset matching (`OTA_ASSET_NAME`) |
| `src/remote_log.h` | Discord webhook remote logging with configurable level |
| `partitions.csv` | nvs 20KB + otadata 8KB + app0 2MB + app1 2MB |
| `lib/EPD/EPD.h` | Unified header: `#ifdef PANEL_579` sets resolution constants |
| `lib/EPD/EPD_42.cpp` | 4.2" single-SSD1683 driver (guarded by `#ifndef PANEL_579`) |
| `lib/EPD/EPD_579.cpp` | 5.79" dual-SSD1683 cascade driver (guarded by `#ifdef PANEL_579`) |
| `lib/EPD_GUI/EPD_GUI.cpp` | Font rendering with U8g2 integration, drawing primitives, gap handling |
| `lib/EPD_GUI/EPD_GUI.h` | GUI function declarations |
| `lib/EPD_GUI/EPD_font.h` | Built-in bitmap fonts (max 48px) |
| `lib/EPD_SPI/` | SPI bit-bang communication (shared, same pins both panels) |
| `platformio.ini` | Two environments: `panel-42` and `panel-579`, shared `[env]` section |
| `.github/workflows/build.yml` | CI: build both envs on v* tag push, create release with both .bin assets |

## Code Conventions

- **Functions**: `snake_case` (e.g., `fetch_weather_data`, `display_main_screen`)
- **Variables**: `camelCase` (e.g., `withingsAccessToken`, `bottomRightMode`)
- **Constants**: `UPPER_CASE` (e.g., `EPD_W`, `LAYOUT_W`, `PRV_KEY`)
- **Panel conditionals**: `#ifdef PANEL_579` / `#else` / `#endif`
- **HTTP pattern**: `WiFiClient`/`WiFiClientSecure` + `HTTPClient`, always call `http.end()`
- **JSON parsing**: `Arduino_JSON` library, check `JSON.typeof()` before accessing
- **Token storage**: NVS via `Preferences` library (wrapped by ConfigManager)
- **NVS namespaces**: `wifi`, `withings`, `strava`, `google`, `firmware`, `misc`
- **NVS key limit**: 15 chars max (e.g., `client_sec` not `client_secret`)

## Display Screens

### Main Screen (both panels)
```
Layout constants:
  midX = LAYOUT_W / 2
  leftCenter = LAYOUT_W / 4 - 10
  rightCenter = LAYOUT_W * 3 / 4
  topHeight = EPD_H - 60
  bottomY = topHeight + 5

Temperature: Y=20, 78px font (Logisoso numbers-only)
Degree symbol: next to temp, 16px "o"
Location labels ("Local"/"Aare"): Y=102, 16px
AareGuru text: Y=152, 16px, centered at midX
Bottom values: 24px at bottomY
Bottom labels: Y = bottomY + 26, 12px
```

### AP Mode Screen
- QR code (left side): WiFi format `WIFI:T:nopass;S:EinkPanel;;`, 4x scale, vertically centered
- Text instructions (right side): title, SSID, IP, steps
- Uses `ricmoo/QRCode` library

### Status Screen
- Shows: WiFi SSID, IP, mDNS hostname, signal strength, firmware version, uptime
- OTA section with state machine (0=idle, 1=checking, 2=available, 3=up to date)

### OTA Progress Screen
- Title, version transition, progress bar (240px wide), percentage, warning text

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
- Each panel looks for its specific asset: `firmware-42.bin` or `firmware-579.bin` (defined as `OTA_ASSET_NAME`)
- Check: boot + manual from dashboard `/check-update` + status screen (OK button)
- `/check-update` uses deferred pattern: first call sets flag, main loop runs check, second call returns cached result
- `/apply-update` sets `otaTriggered` flag, main loop applies
- Progress callback updates e-ink at 25% intervals
- Rollback: `esp_ota_mark_app_valid_cancel_rollback()` after WiFi + NTP succeed
- `FIRMWARE_VERSION` must have `v` prefix to match GitHub tags
- Version set via platformio.ini build flag: `-DFIRMWARE_VERSION='"v0.4.1"'`
- CI `sed` injects version from git tag into platformio.ini (both environments)

## Remote Logging

- Discord webhook for remote diagnostics
- Configurable log level: DEBUG(0), INFO(1), WARN(2), ERROR(3), OFF(4)
- Configured via web dashboard, persisted in NVS (`misc` namespace, `webhook` + `log_level` keys)
- Non-blocking, won't affect display refresh

## Configuration (NVS)

All config managed via `ConfigManager` class and web dashboard:

| Namespace | Keys | Notes |
|-----------|------|-------|
| `wifi` | `ssid`, `password` | Managed by WiFiManager |
| `withings` | `client_id`, `client_sec`, `access_token`, `refresh_token`, `user_id` | OAuth2 single-use refresh |
| `strava` | `client_id`, `client_sec`, `access_token`, `refresh_token` | OAuth2 single-use refresh |
| `google` | `pollen_key` | API key |
| `misc` | `webhook`, `log_level`, `mdns_host` | Discord, logging, mDNS |
| `firmware` | `version` | Updated after OTA |

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
11. 5.79" panel: data sent column-major to SSD1683 (not row-major like 4.2")
12. 5.79" panel: 8-pixel gap at column 396 in framebuffer — `Paint_SetPixel` handles it
13. 5.79" panel: old-data RAM (0x26/0xA6) must be set to 0x00 (black) for update waveform to work
14. Both panels share same SPI pins — only the driver init sequence and data ordering differ

## Debugging

- Serial output via Python: `serial.Serial('/dev/cu.usbserial-XXXX', 115200, timeout=2)`
- Reset device via RTS toggle: `s.setRTS(True); sleep(0.1); s.setRTS(False)`
- All API responses are printed to Serial for debugging
- Web dashboard `/status` endpoint returns health JSON
- Discord webhook for remote logging when device is deployed
