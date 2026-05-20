# E-Ink Desk Panel

A single-screen ESP32-S3 e-ink dashboard displaying weather, river temperature, pollen/UV alerts, body weight, and Strava activity data. Designed for always-on desk use with automatic OTA firmware updates.

![Hardware](https://www.elecrow.com/media/catalog/product/cache/acf3559c3a735f485f0c55e28e1e8f3e/e/s/esp32_4.2_e-paper_1.jpg)

## Features

- **Local weather** — temperature, rain detection, and UV index via [Open-Meteo](https://open-meteo.com/) (1 km resolution, no API key)
- **Aare river temperature** — current water temp + commentary from [AareGuru](https://aareguru.ch/)
- **Pollen / UV alerts** — bottom-left shows rain status, UV warnings (>=8), or pollen level
- **Body weight + trend** — from [Withings](https://developer.withings.com/) smart scale, 6-month trend indicator
- **Last activity** — from [Strava](https://developers.strava.com/), toggled via rocker switch
- **OTA updates** — automatic firmware updates from GitHub Releases
- **Web dashboard** — configure tokens and trigger updates at `http://eink-panel.local`
- **WiFi captive portal** — no hardcoded credentials needed

## Hardware

Built for the [CrowPanel ESP32 4.2" E-Paper Display](https://www.elecrow.com/crowpanel-esp32-4-2-e-paper-hmi-display-with-400-300-resolution-black-white-color-driven-by-spi-interface.html) by Elecrow.

- **Board**: ESP32-S3 WROOM
- **Display**: 400x300 E-Paper (SSD1683) via SPI
- **Input**: 5-button navigation (rocker switch + MENU/HOME/OK)
- **Reference design**: [Elecrow CrowPanel GitHub](https://github.com/Elecrow-RD/CrowPanel-ESP32-4.2-E-paper-HMI-Display-with-400-300)

## Display Layout

```
┌──────────────────────────────────────┐
│                                      │
│    23°              11°              │  large temperatures
│    Local            Aare             │
│                                      │
│         Geduld, geduld               │  AareGuru commentary
│                                      │
│──────────────────────────────────────│
│   low            82.3 kg            │  alerts / weight
│  pollen           stable            │
└──────────────────────────────────────┘
```

## Quick Start

```bash
git clone https://github.com/githendrik/eink-desk-panel.git
cd eink-desk-panel
cp src/credentials.h.example src/credentials.h
# Edit credentials.h with your Withings/Strava tokens
~/.platformio/penv/bin/pio run -t upload
```

On first boot the device creates a WiFi access point (`EinkPanel`). Connect and configure your network via the captive portal.

See [Getting Started](docs/getting-started.md) for full setup instructions.

## Documentation

| Doc | Contents |
|-----|----------|
| [Getting Started](docs/getting-started.md) | Setup, build, and first boot |
| [Project Overview](docs/project-overview.md) | Architecture, layout, font system |
| [API Reference](docs/api-reference.md) | All API endpoints and response formats |
| [OAuth Setup](docs/oauth-setup.md) | Withings and Strava credential flow |
| [Release Procedure](docs/release-procedure.md) | How to tag and publish firmware |
| [OTA Architecture](docs/ota-architecture.md) | Design decisions and implementation plan |
| [v2 Changes](docs/v2-changes.md) | What changed from v1 |

## Tech Stack

- **Framework**: PlatformIO + Arduino
- **Weather**: [Open-Meteo](https://open-meteo.com/) (MeteoSwiss ICON-CH1 model)
- **Fonts**: U8g2 via Adafruit GFX bridge (up to 78px Logisoso)
- **Networking**: WiFiManager, ESPAsyncWebServer, mDNS
- **Storage**: ESP32 NVS (Preferences library)
- **CI/CD**: GitHub Actions — tag push builds and publishes firmware

## License

MIT
