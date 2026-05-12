# Getting Started with E-Ink Desk Panel v2

## Step 1: Install PlatformIO

If you haven't already, install PlatformIO:
- **VS Code**: Install the "PlatformIO IDE" extension
- **CLI**: `pip install platformio`

## Step 2: Configure Credentials

1. Copy the example credentials file:
   ```bash
   cp src/credentials.h.example src/credentials.h
   ```

2. Edit `src/credentials.h` and add your credentials:
   ```cpp
   const char* WIFI_SSID = "your_wifi_name";
   const char* WIFI_PASSWORD = "your_wifi_password";
   const char* OPENWEATHER_API_KEY = "your_openweathermap_api_key";
   ```

3. Get a free OpenWeatherMap API key:
   - Sign up at https://openweathermap.org/api
   - Navigate to API keys section
   - Copy your key to `credentials.h`

## Step 3: Configure Hardware

Edit `platformio.ini`:
- `upload_port`: Change to your ESP32's port (e.g., `/dev/cu.usbserial-10`)
- `monitor_port`: Same as upload_port

## Step 4: Build and Upload

### Using VS Code
1. Click the PlatformIO icon in the left sidebar
2. Click "Upload" (arrow icon)

### Using CLI
```bash
# Build
pio run

# Upload
pio run -t upload

# Monitor serial output
pio device monitor -b 115200
```

## Step 5: Test the Display

After upload:
1. Open Serial Monitor (`pio device monitor -b 115200`)
2. Watch for WiFi connection messages
3. The display should show temperature data
4. Check for API errors in serial output

## Troubleshooting

### WiFi Connection Fails
- Check SSID and password in `credentials.h`
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check signal strength at device location

### Display Not Working
- Check wiring (GPIO 7 powers the display)
- Verify SPI connections
- Listen for relay click on boot

### Weather Shows Default Values
- Check API key is valid
- Look for HTTP response codes in serial monitor
- Verify internet connectivity
- Check city ID is correct (2661552 for Bern)

### API Rate Limiting
- OpenWeatherMap free tier: 60 calls/minute, 1M/month
- Device fetches once per hour (well within limits)

## Customization

### Change Location
Edit `src/weather_screen.h`:
```cpp
String cityId = "2661552"; // OpenWeatherMap city ID
```

For air pollution/pollen, also update coordinates:
```cpp
String pollenServerPath = "http://api.openweathermap.org/data/2.5/air_pollution?lat=46.9480&lon=7.4474&appid=" + openWeatherMapApiKey;
```

### Change Refresh Interval
Edit `src/main.ino`:
```cpp
if (millis() - lastWeatherFetch >= 1000*60*30) { // 30 minutes
```

### Modify Display Layout
Edit `display_weather_screen()` in `src/weather_screen.h`:
- Adjust X/Y coordinates for text positioning
- Change font sizes (8, 12, 16, 24, 32, 48, 72 available)
- Reposition temperature, pollen, and Aare display

## Next Steps

- Read [AI_CONTEXT.md](docs/AI_CONTEXT.md) for development guidance
- Check [API_REFERENCE.md](docs/API_REFERENCE.md) for API details
- Customize the display layout to your preference
