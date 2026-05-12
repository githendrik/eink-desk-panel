# E-Ink Desk Panel - Project Overview

## Project Description

This project implements a multi-screen information display for an E-Ink panel powered by an ESP32-S3 WROOM microcontroller. The device cycles through different information screens including:

- **Screen 1**: Weather forecast
- **Screen 2**: Public transport information  
- **Screen 3**: Spotify "Now Playing" display

## Hardware

- **Board**: Freenove ESP32-S3 WROOM
- **Display**: E-Paper (E-Ink) display via SPI
- **Connection**: USB serial for programming and power

## Architecture

### Directory Structure

```
eink-desk-panel/
├── src/              # Main source code files
│   ├── main.cpp      # Entry point and main loop
│   ├── credentials.h # WiFi and API credentials (not committed)
│   ├── weather_screen.h    # Weather display logic
│   ├── transport_screen.h  # Transport display logic
│   └── spotify_screen.h    # Spotify display logic
├── include/          # Header files and assets
│   ├── spotify_logo.h
│   └── pic.h
├── lib/              # Local libraries
│   ├── EPD/          # E-Paper Display driver
│   ├── EPD_GUI/      # GUI utilities for EPD
│   └── EPD_SPI/      # SPI communication layer
├── scripts/          # Utility scripts
│   ├── convert_svg_to_epd.py
│   └── get_spotify_refresh_token.py
├── docs/             # Documentation
└── platformio.ini    # PlatformIO configuration
```

### Key Components

1. **EPD Driver** (`lib/EPD/`): Low-level e-paper display control
2. **EPD_GUI** (`lib/EPD_GUI/`): Higher-level GUI functions (fonts, shapes, text)
3. **EPD_SPI** (`lib/EPD_SPI/`): SPI communication abstraction
4. **Screen Modules** (`src/*_screen.h`): Individual screen implementations
5. **Credentials** (`src/credentials.h`): API keys and WiFi credentials (template only)

## API Integrations

- **OpenWeatherMap**: Weather data
- **Spotify API**: Currently playing track information
- **Transport API**: Public transport departures (configurable)

## Display Characteristics

- Uses partial refresh for faster updates
- Full refresh when switching screens
- Power-efficient with deep sleep between updates
- Screen indicators show current active screen

## Development Environment

- **Framework**: PlatformIO with Arduino framework
- **Board**: `freenove_esp32_s3_wroom`
- **Libraries**:
  - Adafruit GFX Library
  - U8g2
  - U8g2_for_Adafruit_GFX

## Version History

- **v1**: Original implementation (in `/Users/taarihe1/Documents/PlatformIO/Projects/260121-111731-freenove_esp32_s3_wroom`)
- **v2**: Current version - restructured for better maintainability and AI-assisted development
