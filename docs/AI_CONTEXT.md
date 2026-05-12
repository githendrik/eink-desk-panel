# AI Agent Context - E-Ink Desk Panel v2

## Purpose

This document provides context for AI agents working on this project. It explains the codebase structure, conventions, and common patterns.

## Code Conventions

### File Organization
- `.h` files in `src/` contain implementation code (not just declarations)
- `.h` files in `include/` contain assets (bitmap data, logos)
- `.cpp` files in `lib/` contain library implementations
- All screen logic is in separate header files under `src/`

### Naming Conventions
- **Functions**: `snake_case` (e.g., `fetch_spotify_token`, `display_weather_screen`)
- **Variables**: `camelCase` for local, `snake_case` for globals
- **Constants**: `UPPER_CASE` (e.g., `EPD_W`, `SPOTIFY_CLIENT_ID`)
- **Files**: `snake_case` (e.g., `spotify_screen.h`, `weather_screen.h`)

### Common Patterns

#### Screen Implementation Pattern
Each screen follows this pattern:
```cpp
// Data variables
String screenData = "";
String lastDisplayedData = "";

// Fetch function
void fetch_screen_data(int& httpResponseCode) {
    // API call logic
}

// Change detection
bool data_has_changed() {
    return (data != lastDisplayedData);
}

// Display function
void display_screen(uint8_t* ImageBW, bool& forceFullRefresh) {
    if (forceFullRefresh) {
        EPD_Init();
        EPD_Clear();
        forceFullRefresh = false;
    }
    EPD_Init_Fast(Fast_Seconds_1_5s);
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    // ... draw content ...
    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}
```

#### HTTP Request Pattern
```cpp
WiFiClientSecure client;
client.setInsecure();
HTTPClient http;
http.begin(client, "https://api.example.com/endpoint");
http.addHeader("Authorization", "Bearer " + token);
int httpResponseCode = http.GET();
if (httpResponseCode == 200) {
    String payload = http.getString();
    // Parse JSON...
}
http.end();
```

#### JSON Parsing Pattern
```cpp
JSONVar responseObject = JSON.parse(payload);
if (JSON.typeof(responseObject) == "undefined") {
    // Handle parse error
}
String value = JSON.stringify(responseObject["key"]);
value.replace("\"", ""); // Remove quotes
```

## Key Global Variables

### Display State
- `ImageBW`: Frame buffer for the display
- `forceFullRefresh`: Flag to trigger full screen refresh
- `currentScreen`: Index of currently displayed screen

### WiFi
- `wifiConnected`: Connection status flag
- Reconnection attempts handled in each fetch function

### HTTP
- `httpResponseCode`: Shared variable for HTTP response codes

## Display Functions (from EPD_GUI)

- `EPD_Init()`: Initialize display
- `EPD_Init_Fast(mode)`: Initialize in fast refresh mode
- `EPD_Clear()`: Clear display (full white)
- `EPD_Full(color)`: Fill display with color
- `EPD_ShowStringUTF8(x, y, text, font_size, color)`: Display text
- `EPD_GetUTF8TextWidth(text, font_size)`: Get text width
- `EPD_DrawCircle(x, y, radius, color)`: Draw circle
- `EPD_DrawLine(x1, y1, x2, y2, color)`: Draw line
- `EPD_Display_Part(x, y, w, h, image)`: Partial refresh of region

## Credentials Management

The `credentials.h` file contains sensitive data and should NOT be committed:
```cpp
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
const char* SPOTIFY_CLIENT_ID = "...";
const char* SPOTIFY_CLIENT_SECRET = "...";
const char* SPOTIFY_REFRESH_TOKEN = "...";
const char* WEATHER_API_KEY = "...";
const char* TRANSPORT_API_KEY = "...";

#endif
```

## Common Tasks for AI Agents

### Adding a New Screen
1. Create new `src/new_screen.h`
2. Follow the screen implementation pattern
3. Add screen index to `drawScreenIndicators()`
4. Update main loop to include new screen

### Modifying API Endpoints
- Update the relevant `fetch_*` function in the screen file
- Ensure proper error handling for non-200 responses
- Test with serial monitor output

### Debugging Tips
- Use `Serial.println()` for debugging (output at 115200 baud)
- Check `httpResponseCode` for API issues
- Monitor `WiFi.status()` for connectivity problems
- Use `forceFullRefresh = true` to force screen redraw

## Testing Checklist

- [ ] WiFi connects successfully
- [ ] API calls return 200 status codes
- [ ] Display updates without artifacts
- [ ] Partial refresh works (no full refresh flicker)
- [ ] Screen transitions are smooth
- [ ] Deep sleep conserves power

## Known Limitations

- Certificate verification disabled (`setInsecure()`) for simplicity
- Token refresh happens inline (can cause delays)
- No OTA updates implemented
- Credentials stored in plaintext (not committed to repo)
