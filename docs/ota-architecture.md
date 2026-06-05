# OTA and Configuration Planning

## 1. Goal
Implement a robust Over-The-Air (OTA) update mechanism and a dynamic configuration portal for the E-Ink panel. This allows firmware to be distributed without baked-in secrets and enables users to configure the device via a web interface.

## 2. Design Decisions

- **Captive portal**: WiFi credentials only. API tokens are configured via the web dashboard.
- **Web server**: `ESPAsyncWebServer` (non-blocking) instead of the built-in blocking `WebServer`.
- **GitHub repo**: Will be made public — no auth token needed for the GitHub Releases API.
- **HTTPS validation**: ESP32 Root CA Certificate Bundle (~100KB flash). Proper cert validation everywhere.
- **Version comparison**: Simple string inequality (`tag_name != CURRENT_VERSION`). No semver parsing — we only go forward.
- **OTA check frequency**: Once at boot + manual trigger from the web dashboard.
- **Flash layout**: No SPIFFS partition. HTML is embedded as string literals in code. NVS handles all config storage.
- **Rollback criteria**: App marks itself valid after WiFi + NTP succeed (no API credentials required).
- **Credential removal**: Hardcoded credentials are removed only after the web dashboard (Phase C) is functional.
- **Status screen**: Dedicated MENU button (GPIO 1) toggles a status screen showing IP, firmware version, and future diagnostics. Separate from the rocker switch (GPIO 4/6) which toggles Weight/Strava on the main display.
- **Screen model**: Three screens — main-weight (default), main-strava (rocker), status (MENU button).
- **AP mode display**: E-ink shows AP SSID + `192.168.4.1` when in captive portal mode.
- **mDNS**: Device accessible at configurable hostname (default `http://eink-panel.local`). Set via web dashboard, persisted in NVS.

## 3. Feature Requirements

### 3.1 WiFi Configuration (Captive Portal / AP Mode)
* **Flow**:
  1. Boot up.
  2. Attempt to connect to saved WiFi.
  3. If connection fails or no credentials exist, fall back to Access Point (AP) mode.
  4. Serve a Captive Portal web page where the user can enter WiFi network details.
  5. Save WiFi credentials in non-volatile memory (ESP32 NVS).

### 3.2 Local Web Dashboard
* **Flow**:
  1. Once connected to the local network, host a lightweight async web server.
  2. Serve a dashboard accessible via the device's IP (e.g., `http://<device-ip>`).
  3. Dashboard allows updating API tokens (OpenWeather, Withings, Strava), returning to AP mode, and modifying device settings.
  4. Include a "Check for Updates" / "Update Now" button.

### 3.3 Over-The-Air (OTA) Updates & UI Feedback
* **Flow**:
  1. On boot, panel requests the latest release data from the GitHub API.
  2. Compares the remote `tag_name` with `CURRENT_FIRMWARE_VERSION` (string inequality).
  3. If different, UI (E-Ink) updates to display: "Update available. Downloading..."
  4. Downloads the `.bin` over HTTPS (`setInsecure()`).
  5. Applies the update safely using the ESP32's dual-partition A/B system.
  6. On completion, E-Ink updates to: "Update complete. Rebooting..." and device restarts.

## 4. Technology Stack & Mechanics

### 4.1 Libraries
* **WiFi & Initial Config:** `tzapu/WiFiManager`
* **Storage:** ESP32 `Preferences` library (NVS)
* **Web Dashboard:** `ESPAsyncWebServer` + `AsyncTCP` (non-blocking)
* **OTA Core:** `HTTPClient`, `WiFiClientSecure`, `Update`
* **HTTPS:** ESP32 Root CA Certificate Bundle (built into framework, ~100KB)
* **mDNS:** `ESPmDNS` (built into ESP32 core)

### 4.2 Two-Partition A/B Safety
The ESP32 flash is divided into `otadata` (pointer), `app0` (A), and `app1` (B). OTA writes to the inactive partition and only flips the pointer after MD5 verification. On power loss or corrupt download, the device retries `app0`. Rollback is supported: if the new app crashes on boot, the bootloader can revert.

### 4.3 USB Serial Flashing vs OTA
Manual flashing via `esptool` (PlatformIO) writes directly to `app0` and resets `otadata`, forcing a boot from `app0`. This acts as a master override regardless of what the previous OTA state was.

---

## 5. Implementation Roadmap

### Phase A — Partitioning & Storage Foundation
Goal: Set up the flash layout and persistent storage layer before adding any new features.

- **A1:** Create a custom `partitions.csv` for the ESP32-S3 with: `nvs`, `otadata`, `app0` (~2 MB), `app1` (~2 MB). No SPIFFS.
- **A2:** Update `platformio.ini` to reference the custom partition table and set `board_build.partitions`.
- **A3:** Verify the partition scheme flashes correctly and boots.
- **A4:** Implement a `ConfigManager` class wrapping `Preferences.h` with getters/setters for API tokens, WiFi credentials, and the current firmware version string.

### Phase B — WiFi Manager Integration
Goal: Replace hardcoded WiFi credentials with the captive portal flow.

- **B1:** Add `tzapu/WiFiManager` to `platformio.ini` lib_deps.
- **B2:** Replace the current WiFi connection code in `main.cpp` / `setup()` with a WiFiManager lifecycle (`WiFiManager` object, `autoConnect()` fallback).
- **B3:** On the next boot cycle, read saved WiFi credentials from NVS and connect automatically.
- **B4:** Verify fallback to AP mode works when saved credentials are invalid.

### Phase C — Web Dashboard
Goal: Host an internal async web interface for managing device state.

- **C1:** Add `ESPAsyncWebServer` and `AsyncTCP` to `platformio.ini` lib_deps.
- **C2:** Initialize the async web server in `setup()` (after WiFi connects).
- **C3:** Create a single-page HTML dashboard (embedded as string literal), minimal and mobile-friendly.
- **C4:** Serve the dashboard at the root `/` route.
- **C5:** Implement a `/save` API endpoint: accepts token/settings updates via POST and persists them to `ConfigManager` (NVS).
- **C6:** Implement a `/reset` endpoint: wipes saved WiFi creds from NVS and calls `ESP.restart()` so the device drops back into AP mode.
- **C7:** Implement a `/status` JSON endpoint returning current firmware version, WiFi signal strength, and a brief health summary.
- **C8:** Remove all hardcoded credentials from source code. Read all API tokens from NVS (populated via dashboard).
- **C9:** Enable mDNS so the dashboard is accessible at `http://eink-panel.local`.

### Phase C2 — Status Screen & AP Display
Goal: Provide on-device feedback for network state and diagnostics.

- **C2-1:** Configure GPIO 1 (MENU button) as INPUT with ISR on FALLING edge + debounce.
- **C2-2:** MENU button toggles between the main display and a status screen.
- **C2-3:** Status screen shows: device IP, firmware version, WiFi SSID, signal strength.
- **C2-4:** (Future) Add last-fetch timestamps and error log to the status screen.
- **C2-5:** When entering AP mode (captive portal), draw a setup screen on e-ink: AP SSID (`EinkPanel`) + IP (`192.168.4.1`) + instructions.

### Phase D — OTA Update Core
Goal: Check for, download, and apply firmware updates from GitHub Releases.

- **D1:** Define `CURRENT_FIRMWARE_VERSION` as a string baked into the build (via `platformio.ini` build flags or `#define`).
- **D2:** On boot, query `https://api.github.com/repos/{owner}/{repo}/releases/latest` with `HTTPClient` + `WiFiClientSecure` (using Root CA bundle for cert validation).
- **D3:** Parse the JSON response — extract `tag_name` and compare with `CURRENT_FIRMWARE_VERSION` (string inequality).
- **D4:** If an update is found, resolve the panel-specific `.bin` asset URL from `assets[].browser_download_url` (matching `firmware-42.bin` or `firmware-579.bin` based on build flag `OTA_ASSET_NAME`).
- **D5:** Open the firmware `.bin` stream and feed it into the `Update` library (writing to the inactive OTA partition).
- **D6:** Report download/verification progress back to a callback.
- **D7:** Verify the update with MD5 (built into `Update`), then `Update.end()`.
- **D8:** Save the new version string into NVS and reboot via `ESP.restart()`.

### Phase E — E-Ink Screen Feedback
Goal: Display update progress on the panel itself.

- **E1:** Pause the normal E-Ink display refresh loop when an OTA cycle begins.
- **E2:** Draw a static splash screen: "Update found: vX.Y.Z. Downloading..." (full refresh).
- **E3:** Hook into the update progress callback and draw coarse progress updates at 25%, 50%, 75% intervals (avoiding heavy E-Ink refresh on every byte).
- **E4:** On completion, draw "Update complete. Rebooting..." (full refresh).
- **E5:** On failure, draw "Update failed. Please try again." and return to normal display mode.

### Phase F — Dashboard OTA Trigger
Goal: Expose the update mechanism via the web dashboard.

- **F1:** Add a "Check for Updates" and an "Update Now" button to the dashboard HTML.
- **F2:** Create a `/check-update` API endpoint: calls the GitHub API and returns JSON `{available: true, version: "v1.2.3"}`.
- **F3:** Create an `/apply-update` API endpoint: triggers the OTA flow (Phase D). The endpoint responds immediately with `{status: "started"}`, and the actual download runs in the main loop.

### Phase G — Rollback & Safety
Goal: Ensure the device can recover if the new firmware is broken.

- **G1:** Configure the Arduino ESP32 framework to enable "App Rollback" (set `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` via build flags in `platformio.ini`).
- **G2:** On the first boot after an OTA update, call `esp_ota_mark_app_valid_cancel_rollback()` after WiFi + NTP succeed.
- **G3:** If the app crashes before marking itself valid (e.g., watchdog reset, crash loop), the bootloader will automatically revert to the previous partition on the next power cycle.
- **G4:** If `/reset` is called from the dashboard AND we are currently running from an OTA partition, revert to `app0` before rebooting.

### Phase H — CI/CD & Release Automation
Goal: Automate building and publishing firmware.

- **H1:** Create `.github/workflows/build.yml` that triggers on tag pushes (`v*`).
- **H2:** Use `platformio` CLI in CI to compile the firmware for both target panels (`panel-42` and `panel-579`).
- **H3:** Create a GitHub Release automatically from the tag and attach both compiled binaries (`firmware-42.bin` and `firmware-579.bin`).
- **H4:** Verify the OTA endpoint detects the newly published release.

---

## 6. Open Questions / Future Considerations
* **OTA File Size**: Current firmware is ~975KB. Two 2MB app partitions in 8MB flash should be comfortable, but verify after adding async web server + dashboard HTML + CA bundle.
* **HTTPS Redirects**: GitHub release downloads redirect through a CDN. The ESP32 `HTTPClient` follows redirects by default, but this should be explicitly tested.
* **Status screen expansion**: Future iterations could add last-fetch timestamps per API, error counts, and uptime.

## 7. Button & Pin Summary

| Button | GPIO | Function |
|--------|------|----------|
| PRV | 6 | Rocker prev (toggle Weight/Strava on main screen) |
| NEXT | 4 | Rocker next (toggle Weight/Strava on main screen) |
| MENU | 1 | Toggle status screen |
| HOME | 2 | Unused (available) |
| OK | 5 | On status screen: check for OTA / apply update |
