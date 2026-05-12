# AI Agent Context - E-Ink Desk Panel v2

## Purpose

This document provides context for AI agents working on this project. It explains the codebase structure, conventions, and common patterns.

## Code Conventions

### File Organization
- `.ino` file in `src/` contains main entry point
- `.h` files in `src/` contain screen implementation
- `.h` files in `include/` contain assets (bitmap data, logos)
- `.cpp` files in `lib/` contain library implementations

### Naming Conventions
- **Functions**: `snake_case` (e.g., `fetch_weather_data`, `display_weather_screen`)
- **Variables**: `camelCase` for local, `snake_case` for globals
- **Constants**: `UPPER_CASE` (e.g., `EPD_W`, `WIFI_SSID`)
- **Files**: `snake_case` (e.g., `weather_screen.h`, `main.ino`)

## Key Global Variables

### Display State
- `ImageBW`: Frame buffer for the display (15000 bytes)
- `forceFullRefresh`: Flag to trigger full screen refresh on boot

### Weather Data
- `weather`: Weather condition string
- `temperature`: Current temperature
- `temperatureMin`: Minimum temperature
- `temperatureMax`: Maximum temperature
- `aareTemp`: Aare river temperature
- `aareText`: Swimming condition text
- `aareTime`: Timestamp of Aare reading

### WiFi
- `WIFI_SSID`, `WIFI_PASSWORD`: From credentials.h
- `WiFi.status()`: Check connection status

### HTTP
- `httpResponseCode`: Variable for HTTP response codes

## Display Functions (from EPD_GUI)

- `EPD_GPIOInit()`: Initialize display GPIO
- `EPD_Init()`: Initialize display (full refresh mode)
- `EPD_Init_Fast(mode)`: Initialize in fast refresh mode
- `EPD_Clear()`: Clear display (full white)
- `EPD_Full(color)`: Fill display with color
- `EPD_ShowStringUTF8(x, y, text, font_size, color)`: Display text
- `EPD_GetUTF8TextWidth(text, font_size)`: Get text width
- `EPD_ShowPicture(x, y, w, h, index, color)`: Display bitmap
- `EPD_DrawLine(x1, y1, x2, y2, color)`: Draw line
- `EPD_Display_Part(x, y, w, h, image)`: Partial refresh of region

## Credentials Management

The `credentials.h` file contains sensitive data and should NOT be committed:
```cpp
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
const char* OPENWEATHER_API_KEY = "your_api_key";
const char* WEATHER_CITY = "Bern";

#endif
```

## Common Patterns

### HTTP Request Pattern
```cpp
WiFiClient client;
HTTPClient http;
http.begin(client, "https://api.example.com/endpoint");
int httpResponseCode = http.GET();
if (httpResponseCode == 200) {
    String payload = http.getString();
    // Parse JSON...
}
http.end();
```

### JSON Parsing Pattern
```cpp
JSONVar responseObject = JSON.parse(payload);
if (JSON.typeof(responseObject) == "undefined") {
    // Handle parse error
}
String value = JSON.stringify(responseObject["key"]);
value.replace("\"", ""); // Remove quotes
```

### Display Update Pattern
```cpp
void display_weather_screen(uint8_t* ImageBW, bool& forceFullRefresh)
{
    if (forceFullRefresh) {
        EPD_Init();
        EPD_Clear();
        forceFullRefresh = false;
    }
    EPD_Init_Fast(Fast_Seconds_1_5s);
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    EPD_Full(WHITE);
    // ... draw content ...
    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}
```

## Common Tasks for AI Agents

### Modifying Weather Data Source
1. Edit `fetch_weather_data()` in `src/weather_screen.h`
2. Update API endpoint and parsing logic
3. Test with serial monitor output

### Changing Display Layout
1. Edit `display_weather_screen()` in `src/weather_screen.h`
2. Adjust coordinates and font sizes
3. Test with partial refresh (set `forceFullRefresh = false`)

### Adding New Data Fields
1. Add global variables in `src/weather_screen.h`
2. Update `fetch_weather_data()` to retrieve new data
3. Update `display_weather_screen()` to show new data

### Changing Refresh Interval
Edit `src/main.ino`:
```cpp
if (millis() - lastWeatherFetch >= 1000*60*60) { // Change interval here
```

## Debugging Tips

- Use `Serial.println()` for debugging (output at 115200 baud)
- Check `httpResponseCode` for API issues
- Monitor `WiFi.status()` for connectivity problems
- Use `forceFullRefresh = true` to force screen redraw

## Testing Checklist

- [ ] WiFi connects successfully
- [ ] Weather API returns 200 status code
- [ ] Aare API returns 200 status code
- [ ] Display updates without artifacts
- [ ] Partial refresh works (no full refresh flicker)
- [ ] Hourly refresh timer works correctly

## Known Limitations

- Certificate verification disabled (`setInsecure()`) for simplicity
- Credentials stored in plaintext (not committed to repo)
- Single screen only (no multi-screen support)
- No button input handling (removed from v2)
