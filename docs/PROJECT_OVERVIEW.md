# E-Ink Desk Panel - Project Overview

## Project Description

This project implements a single-screen weather display for an E-Ink panel powered by an ESP32-S3 WROOM microcontroller. The device displays:

- Current weather conditions
- Temperature (current, min, max)
- Aare river temperature and swimming conditions

## Hardware

- **Board**: Freenove ESP32-S3 WROOM
- **Display**: E-Paper (E-Ink) display via SPI
- **Connection**: USB serial for programming and power

## Architecture

### Directory Structure

```
eink-desk-panel/
├── src/
│   ├── main.ino              # Entry point and main loop
│   ├── credentials.h         # WiFi and API credentials (not committed)
│   └── weather_screen.h      # Weather display logic
├── include/                  # Header files and assets
├── lib/                      # Local libraries
│   ├── EPD/                  # E-Paper Display driver
│   ├── EPD_GUI/              # GUI utilities for EPD
│   └── EPD_SPI/              # SPI communication layer
├── scripts/                  # Utility scripts
└── docs/                     # Documentation
```

### Key Components

1. **EPD Driver** (`lib/EPD/`): Low-level e-paper display control
2. **EPD_GUI** (`lib/EPD_GUI/`): Higher-level GUI functions (fonts, shapes, text)
3. **EPD_SPI** (`lib/EPD_SPI/`): SPI communication abstraction
4. **Weather Screen** (`src/weather_screen.h`): Weather and Aare data display

## API Integrations

- **OpenWeatherMap**: Weather data (temperature, conditions, forecast)
- **AareGuru**: Aare river temperature and swimming conditions for Bern

## Display Characteristics

- Uses partial refresh for faster updates
- Full refresh on initial boot
- Power-efficient with sleep between updates
- Updates every hour

## Development Environment

- **Framework**: PlatformIO with Arduino framework
- **Board**: `freenove_esp32_s3_wroom`
- **Libraries**:
  - Adafruit GFX Library
  - U8g2
  - U8g2_for_Adafruit_GFX
  - Arduino_JSON
  - TimeLib

## Version History

- **v1**: Original multi-screen implementation (weather, transport, Spotify)
- **v2**: Single-screen weather display, simplified architecture
