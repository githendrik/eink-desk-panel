# Getting Started

## Prerequisites

- [PlatformIO](https://platformio.org/) installed
- One of the supported CrowPanel displays:
  - CrowPanel 4.2" e-ink (400x300, single SSD1683)
  - CrowPanel 5.79" e-ink (792x272, dual SSD1683)

## Quick Setup

### 1. Clone and configure

```bash
git clone https://github.com/githendrik/eink-desk-panel.git
cd eink-desk-panel
cp src/credentials.h.example src/credentials.h
```

### 2. Get API credentials

No API key is needed for weather data (Open-Meteo is free and keyless).

**Withings** (for weight):
```bash
pip3 install requests
python3 scripts/get_withings_credentials.py
```

**Strava** (for activities):
```bash
python3 scripts/get_strava_credentials.py
```

### 3. Edit credentials.h

Fill in the Withings and Strava `#define` values from the steps above, plus your WiFi credentials.

### 4. Build and upload

```bash
# Check your serial port
ls /dev/cu.usb*

# Update platformio.ini upload_port if needed

# Build and upload for 4.2" panel
~/.platformio/penv/bin/pio run -e panel-42 -t upload

# Or for 5.79" panel
~/.platformio/penv/bin/pio run -e panel-579 -t upload
```

### Build both panels (no upload)

```bash
~/.platformio/penv/bin/pio run
```

## After First Boot

1. If no WiFi credentials are saved, the device enters AP mode (SSID: `EinkPanel`)
2. Scan the QR code on the display or manually connect to the AP
3. Configure WiFi via the captive portal
4. Once connected, access the web dashboard at `http://eink-panel.local`
5. Enter Withings/Strava tokens via the dashboard (or pre-fill in `credentials.h`)
6. Optionally set a custom mDNS hostname in the Network section (e.g. `eink-panel-579`)

## Web Dashboard

The web dashboard at `http://<hostname>.local` provides:

- API token configuration (Withings, Strava, Google Pollen)
- Network settings (mDNS hostname)
- Remote logging configuration (Discord webhook + log level)
- OTA update check and apply
- Device reboot
- Status overview (firmware, IP, signal, uptime)

## Refresh Intervals

| Data | Interval |
|------|----------|
| Weather, UV, Pollen, Aare | 10 minutes |
| Strava activity | 1 hour |
| Weight | 6 hours |

## Using the Buttons

- **Rocker switch** (left/right): Toggle bottom-right between Weight and Strava
- **MENU**: Toggle status screen (shows IP, mDNS name, firmware version, signal strength)
- **OK** (on status screen): Check for OTA update / apply update

## OTA Updates

The device checks for firmware updates on boot. Each panel looks for its own binary in the release (`firmware-42.bin` or `firmware-579.bin`). You can also trigger a check from:
- The web dashboard
- The on-device status screen (MENU then OK)

## Remote Logging

Optional Discord webhook logging for remote diagnostics:
1. Create a Discord webhook in your channel settings
2. Enter the webhook URL in the web dashboard under "System Logging"
3. Set the log level (DEBUG, INFO, WARN, ERROR, or OFF)

Logs are sent asynchronously and won't block normal operation.
