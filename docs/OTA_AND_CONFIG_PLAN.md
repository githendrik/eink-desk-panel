# OTA and Configuration Planning

## 1. Goal
Implement a robust Over-The-Air (OTA) update mechanism and a dynamic configuration portal for the E-Ink panel. This allows firmware to be distributed without baked-in secrets and enables users to configure the device via a web interface.

## 2. Feature Requirements

### 2.1 WiFi Configuration & Credential Management (Captive Portal / AP Mode)
* **Flow**:
  1. Boot up.
  2. Attempt to connect to saved WiFi.
  3. If connection fails or no credentials exist, fall back to Access Point (AP) mode.
  4. Serve a Captive Portal web page where the user can enter network details and application-specific API tokens.
  5. Save credentials safely in non-volatile memory (ESP32 NVS).

### 2.2 Local Web Dashboard
* **Flow**:
  1. Once connected to the local network, host a lightweight local web server.
  2. Serve a dashboard accessible via the device's IP (e.g., `http://<device-ip>`).
  3. Dashboard allows updating API tokens, returning to AP mode, and modifying device settings.
  4. Include a "Check for Updates" / "Trigger OTA Update" button.

### 2.3 Over-The-Air (OTA) Updates & UI Feedback
* **Flow**:
  1. Panel requests the latest release data from the GitHub API.
  2. Compares the remote version with the currently running version.
  3. If newer, UI (E-Ink) updates to display: "Update available. Downloading..."
  4. Downloads the update over an HTTPS connection.
  5. Applies the update safely using the ESP32's dual-partition A/B system.
  6. On completion, E-Ink updates to: "Update complete. Rebooting..." and device restarts.

## 3. Technology Stack & Mechanics

### 3.1 Libraries
* **WiFi & Initial Config:** `tzapu/WiFiManager`
* **Storage:** ESP32 `Preferences` library (NVS)
* **Web Dashboard:** `WebServer` (built into ESP32 core)
* **OTA Core:** `HTTPClient`, `WiFiClientSecure`, `Update`

### 3.2 Two-Partition A/B Safety
The ESP32 flash is divided into `otadata` (pointer), `app0` (A), and `app1` (B). OTA writes to the inactive partition and only flips the pointer after MD5 verification. On power loss or corrupt download, the device retries `app0`. Rollback is supported: if the new app crashes on boot, the bootloader can revert.

### 3.3 USB Serial Flashing vs OTA
Manual flashing via `esptool` (PlatformIO) writes directly to `app0` and resets `otadata`, forcing a boot from `app0`. This acts as a master override regardless of what the previous OTA state was.

---

## 4. Implementation Roadmap

### Phase A — Partitioning & Storage Foundation
Goal: Set up the flash layout and persistent storage layer before adding any new features.

- **A1:** Create a custom `partitions.csv` for the ESP32-S3 that reserves two OTA app partitions (`app0` and `app1` of ~2 MB each) plus `otadata`, `nvs`, `spiffs`.
- **A2:** Update `platformio.ini` to reference the custom partition table and set `board_build.partitions`.
- **A3:** Verify the partition scheme flashes correctly and boots.
- **A4:** Implement a `ConfigManager` class wrapping `Preferences.h` with getters/setters for API tokens, WiFi credentials, and the current firmware version string.

### Phase B — WiFi Manager Integration
Goal: Replace hardcoded WiFi credentials with the captive portal flow.

- **B1:** Add `tzapu/WiFiManager` to `platformio.ini` lib_deps.
- **B2:** Replace the current WiFi connection code in `main.cpp` / `setup()` with a WiFiManager lifecycle (`WiFiManager` object, `autoConnect()` fallback).
- **B3:** Register custom `WiFiManagerParameter` fields for any API tokens (so users can enter them right on the WiFi config page).
- **B4:** Wire the WiFiManager save callback to persist the custom fields into `ConfigManager` (NVS) when the user clicks "Save" in the portal.
- **B5:** On the next boot cycle, read saved tokens from NVS and pass them to the appropriate display/service logic.
- **B6:** Remove all hardcoded credentials and tokens from the source code.

### Phase C — Web Dashboard
Goal: Host an internal web interface for managing device state, even when WiFi is already configured.

- **C1:** Initialize the `WebServer` library in `setup()` (after WiFi connects).
- **C2:** Create a single-page HTML dashboard with a minimal, mobile-friendly UI.
- **C3:** Serve the dashboard at the root `/` route.
- **C4:** Implement a `/save` API endpoint: accepts token/settings updates via POST and persists them to `ConfigManager`.
- **C5:** Implement a `/reset` endpoint: wipes saved WiFi creds from NVS and calls `ESP.restart()` so the device drops back into AP mode.
- **C6:** Implement a `/status` JSON endpoint returning current firmware version, WiFi signal strength, and a brief health summary.

### Phase D — OTA Update Core
Goal: Check for, download, and apply firmware updates from GitHub Releases.

- **D1:** Define `CURRENT_FIRMWARE_VERSION` as a string in code (or baked into the build from `platformio.ini`).
- **D2:** Query `https://api.github.com/repos/{owner}/{repo}/releases/latest` with `HTTPClient` + `WiFiClientSecure`.
- **D3:** Parse the JSON response with `ArduinoJson` — extract `tag_name` and check if it's newer.
- **D4:** If an update is found, resolve the `.bin` asset URL from `assets[].browser_download_url`.
- **D5:** Enable the ESP32 Root CA Certificate Bundle to validate the HTTPS connection to GitHub (and the CDN redirect).
- **D6:** Open the firmware `.bin` stream and feed it into the `Update` library (writing to the inactive OTA partition).
- **D7:** Report download/verification progress back to a callback.
- **D8:** Verify the update with MD5 (built into `Update`), then `Update.end()`.
- **D9:** Save the new version string into NVS and reboot via `ESP.restart()`.

### Phase E — E-Ink Screen Feedback
Goal: Display update progress on the panel itself (not just the web dashboard).

- **E1:** Pause the normal E-Ink display refresh loop when an OTA cycle begins.
- **E2:** Draw a static splash screen: "Update found: vX.Y.Z. Downloading..." (full refresh).
- **E3:** Hook into the update progress callback and draw coarse progress updates at 25%, 50%, 75% intervals (avoiding heavy E-Ink refresh on every byte).
- **E4:** On completion, draw "Update complete. Rebooting..." (full refresh).
- **E5:** On failure, draw "Update failed. Please try again." and return to normal display mode.

### Phase F — Dashboard OTA Trigger
Goal: Expose the update mechanism via the web dashboard.

- **F1:** Add a "Check for Updates" and an "Update Now" button to the dashboard HTML.
- **F2:** Create a `/check-update` API endpoint: calls the GitHub API and returns JSON `{available: true, version: "v1.2.3"}`.
- **F3:** Create an `/apply-update` API endpoint: triggers the OTA flow (Phase D) asynchronously. The endpoint responds immediately with `{status: "started"}`, and the actual download runs in the background loop.

### Phase G — Rollback & Safety
Goal: Ensure the device can recover if the new firmware is broken.

- **G1:** Configure the Arduino ESP32 framework to enable "App Rollback" (set `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` via `menuconfig` or build flags in `platformio.ini`).
- **G2:** On the first boot after an OTA update, the app must explicitly call `esp_ota_mark_app_valid_cancel_rollback()` after it successfully connects to WiFi and initializes its core features.
- **G3:** If the app crashes before marking itself valid (e.g., watchdog reset, crash loop), the bootloader will automatically revert to the previous partition on the next power cycle.
- **G4:** If /reset is called from the dashboard AND we are currently running from an OTA partition, revert to `app0` before rebooting.

### Phase H — CI/CD & Release Automation
Goal: Automate building and publishing firmware.

- **H1:** Create `.github/workflows/build.yml` that triggers on tag pushes (`v*`).
- **H2:** Use `platformio` CLI in CI to compile the firmware for the target board.
- **H3:** Create a GitHub Release automatically from the tag and attach the compiled `.bin` file.
- **H4:** Verify the OTA endpoint detects the newly published release.

---

## 5. Open Questions / Future Considerations
* **OTA File Size**: E-Ink displays need large frame buffers. Ensure partitions are sized appropriately (2 MB per app slot is a good starting point; verify the compiled `.bin` fits).
* **HTTPS Redirects**: GitHub release downloads redirect through a CDN. The ESP32 `HTTPClient` follows redirects by default, but this should be explicitly tested.
* **OTA Interval**: Decide if the panel should check for updates on every boot, or on a schedule (e.g., once per day).
