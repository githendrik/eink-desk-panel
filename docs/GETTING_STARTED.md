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
   const char* OPENWEATHER_API_KEY = "your_weather_api_key";
   const char* SPOTIFY_CLIENT_ID = "your_spotify_client_id";
   const char* SPOTIFY_CLIENT_SECRET = "your_spotify_secret";
   const char* SPOTIFY_REFRESH_TOKEN = "your_refresh_token";
   ```

3. Get your Spotify refresh token:
   ```bash
   python scripts/get_spotify_refresh_token.py
   ```

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
pio device monitor
```

## Step 5: Test the Display

After upload:
1. Open Serial Monitor (`pio device monitor -b 115200`)
2. Watch for WiFi connection messages
3. The display should show the weather screen first
4. Press buttons (if connected) to cycle through screens

## Troubleshooting

### WiFi Connection Fails
- Check SSID and password in `credentials.h`
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)

### Display Not Working
- Check wiring (GPIO 7 powers the display)
- Verify SPI connections

### Spotify Shows "Not Playing"
- Ensure Spotify is playing on a device
- Check that refresh token is valid
- Look for HTTP 401 errors in serial monitor

### API Errors
- Check API keys are valid
- Look at HTTP response codes in serial monitor
- Verify internet connectivity

## Next Steps

- Customize screen layouts in `src/*_screen.h` files
- Add new screens by following the existing pattern
- Adjust refresh intervals in `src/main.ino`
- Read [AI_CONTEXT.md](docs/AI_CONTEXT.md) for development guidance
