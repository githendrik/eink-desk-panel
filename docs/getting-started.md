# Getting Started

## Prerequisites

- [PlatformIO](https://platformio.org/) installed
- CrowPanel 4.2" e-ink display (ESP32-S3 WROOM)

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

# Update platformio.ini if port differs from /dev/cu.usbserial-110

# Build and upload
~/.platformio/penv/bin/pio run -t upload
```

## After First Boot

1. If no WiFi credentials are saved, the device enters AP mode (SSID: `EinkPanel`)
2. Connect to the AP and configure WiFi via the captive portal
3. Once connected, access the web dashboard at `http://eink-panel.local`
4. Enter Withings/Strava tokens via the dashboard (or pre-fill in `credentials.h`)

## Refresh Intervals

| Data | Interval |
|------|----------|
| Weather, UV, Pollen, Aare | 10 minutes |
| Strava activity | 1 hour |
| Weight | 6 hours |

## Using the Buttons

- **Rocker switch** (left/right): Toggle bottom-right between Weight and Strava
- **MENU**: Toggle status screen (shows IP, firmware version, signal strength)
- **OK** (on status screen): Check for OTA update / apply update

## OTA Updates

The device checks for firmware updates on boot. You can also trigger a check from:
- The web dashboard at `http://eink-panel.local`
- The on-device status screen (MENU → OK)
