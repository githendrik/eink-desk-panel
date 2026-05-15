# Getting Started

## Prerequisites

- [PlatformIO](https://platformio.org/) installed
- CrowPanel 4.2" e-ink display (ESP32-S3 WROOM)
- API keys (see below)

## Quick Setup

### 1. Clone and configure

```bash
git clone https://github.com/githendrik/eink-desk-panel.git
cd eink-desk-panel
cp src/credentials.h.example src/credentials.h
```

### 2. Get API credentials

**OpenWeatherMap** (free):
- Sign up at https://openweathermap.org/api
- Get an API key

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

Fill in all the `#define` values from the steps above, plus your WiFi credentials.

### 4. Build and upload

```bash
# Check your serial port
ls /dev/cu.usb*

# Update platformio.ini if port differs from /dev/cu.usbserial-110

# Build and upload
~/.platformio/penv/bin/pio run -t upload
```

## Refresh Intervals

| Data | Interval |
|------|----------|
| Weather, Pollen, Rain, Aare | 15 minutes |
| Strava activity | 1 hour |
| Weight | 6 hours |

## Using the Rocker Switch

- Press either direction to toggle the bottom-right between **Weight** (default) and **Strava** activity
