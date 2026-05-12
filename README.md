# E-Ink Desk Panel v2

Single-screen weather display for E-Ink panel using ESP32-S3 WROOM.

## Features

- **Weather Screen**: Current weather and forecast from OpenWeatherMap
- **Aare Temperature**: River temperature data for Bern (from aareguru)
- **Auto-refresh**: Updates every hour
- **Low Power**: Deep sleep between updates

## Quick Start

### Prerequisites

1. PlatformIO (VS Code extension or CLI)
2. Python 3.x (optional, for utility scripts)
3. Freenove ESP32-S3 WROOM board

### Setup

1. **Clone the repository**
   ```bash
   cd eink-desk-panel
   ```

2. **Create credentials file**
   
   Copy `src/credentials.h.example` to `src/credentials.h`:
   ```bash
   cp src/credentials.h.example src/credentials.h
   ```

3. **Edit credentials**
   
   Fill in your details in `src/credentials.h`:
   ```cpp
   const char* WIFI_SSID = "your_wifi_name";
   const char* WIFI_PASSWORD = "your_wifi_password";
   const char* OPENWEATHER_API_KEY = "your_weather_api_key";
   const char* WEATHER_CITY = "Bern";
   ```

4. **Configure PlatformIO**
   
   Edit `platformio.ini`:
   - `upload_port`: Your ESP32's serial port (e.g., `/dev/cu.usbserial-10`)
   - `monitor_port`: Same as upload_port

5. **Build and Upload**
   ```bash
   pio run -t upload
   pio device monitor
   ```

## Project Structure

```
eink-desk-panel/
├── src/
│   ├── main.ino              # Entry point
│   ├── credentials.h.example # Credentials template
│   └── weather_screen.h      # Weather display logic
├── include/                  # Assets (icons, logos)
├── lib/                      # EPD driver libraries
├── scripts/                  # Utility scripts
└── docs/                     # Documentation
```

## Documentation

- [Project Overview](docs/PROJECT_OVERVIEW.md)
- [AI Context](docs/AI_CONTEXT.md) - Guide for AI-assisted development
- [API Reference](docs/API_REFERENCE.md)
- [Getting Started](docs/GETTING_STARTED.md)

## Hardware

- Board: Freenove ESP32-S3 WROOM
- Display: E-Paper via SPI
- Power: USB or external 5V
- GPIO 7: Display power

## Display Layout

```
┌─────────────────────────┐
│  [Weather Icon]  23°    │
│                  18°/26°│
│                  Clear  │
├─────────────────────────┤
│  Aare  15°    swimming  │
└─────────────────────────┘
```

## API Integrations

- **OpenWeatherMap**: Weather data (temperature, conditions)
- **AareGuru**: Aare river temperature and swimming conditions

## Development

### Code Style

- Functions: `snake_case`
- Variables: `camelCase` (local), `snake_case` (global)
- Constants: `UPPER_CASE`

### Modifying Refresh Interval

Edit `src/main.ino`:
```cpp
if (millis() - lastWeatherFetch >= 1000*60*60) { // 1 hour
```

### Changing Location

Edit `src/weather_screen.h`:
```cpp
String city = "Bern";
String countryCode = "2661552"; // OpenWeatherMap city ID
```

## License

[Your license here]

## Acknowledgments

Based on the original v1 project, simplified to single-screen weather display.
