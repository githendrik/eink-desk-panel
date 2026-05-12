# E-Ink Desk Panel v2

Multi-screen information display for E-Ink panel using ESP32-S3 WROOM.

## Features

- **Weather Screen**: Current weather and forecast from OpenWeatherMap
- **Transport Screen**: Real-time public transport departures
- **Spotify Screen**: "Now Playing" display with album art support

## Quick Start

### Prerequisites

1. PlatformIO (VS Code extension or CLI)
2. Python 3.x (for utility scripts)
3. Freenove ESP32-S3 WROOM board

### Setup

1. **Clone the repository**
   ```bash
   cd eink-desk-panel
   ```

2. **Create credentials file**
   
   Copy `src/credentials.h.example` to `src/credentials.h` and fill in your credentials:
   - WiFi SSID and password
   - Spotify API credentials (run `scripts/get_spotify_refresh_token.py`)
   - Weather API key from OpenWeatherMap
   - Transport API key

3. **Configure PlatformIO**
   
   Edit `platformio.ini` if needed:
   - `upload_port`: Your ESP32's serial port
   - `monitor_port`: Same as upload_port

4. **Build and Upload**
   ```bash
   pio run -t upload
   pio device monitor
   ```

## Project Structure

```
eink-desk-panel/
├── src/              # Source code
├── include/          # Headers and assets
├── lib/              # Local libraries (EPD drivers)
├── scripts/          # Utility scripts
├── docs/             # Documentation
└── platformio.ini    # Configuration
```

## Documentation

- [Project Overview](docs/PROJECT_OVERVIEW.md) - Architecture and components
- [AI Context](docs/AI_CONTEXT.md) - Guide for AI-assisted development

## Scripts

- `get_spotify_refresh_token.py`: Obtain Spotify refresh token
- `convert_svg_to_epd.py`: Convert SVG graphics to E-Ink compatible format

## Development

### Adding New Features

1. Follow the existing screen pattern in `src/`
2. Use `Serial.println()` for debugging
3. Test partial vs full refresh behavior

### Code Style

- Functions: `snake_case`
- Variables: `camelCase` (local), `snake_case` (global)
- Constants: `UPPER_CASE`

## Hardware

- Board: Freenove ESP32-S3 WROOM
- Display: E-Paper via SPI
- Power: USB or external 5V

## License

[Your license here]

## Acknowledgments

Based on the original v1 project with improvements for maintainability and AI-assisted development.
