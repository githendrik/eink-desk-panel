# Version 2 Changes

## What's New in v2

### Single Screen Design
- Removed multi-screen navigation
- Removed button input handling
- Focused on weather display only
- Simplified codebase significantly

### Removed Features (from v1)
- ~~Spotify "Now Playing" screen~~
- ~~Public transport departures screen~~
- ~~Screen switching with buttons~~
- ~~Screen indicator dots~~

### Retained Features
- Weather display from OpenWeatherMap
- Aare river temperature data
- Hourly auto-refresh
- Partial display refresh
- Low power operation

### Code Changes

#### main.ino
- Removed button interrupt handlers
- Removed screen state management
- Single screen update loop
- Simplified setup and loop functions

#### weather_screen.h
- Removed `httpGETRequest` callback parameter
- Direct HTTP calls in fetch function
- Removed screen indicator drawing
- Cleaner display function

#### credentials.h
- Removed Spotify credentials
- Removed transport credentials
- Added city configuration option

## File Changes

| File | Status | Notes |
|------|--------|-------|
| `src/main.ino` | Simplified | Single screen logic |
| `src/weather_screen.h` | Modified | Direct HTTP calls |
| `src/spotify_screen.h` | Removed | Not needed |
| `src/transport_screen.h` | Removed | Not needed |
| `include/spotify_logo.h` | Removed | Not needed |
| `scripts/get_spotify_refresh_token.py` | Removed | Not needed |
| `scripts/spotify_logo.svg` | Removed | Not needed |

## Benefits of v2 Structure

1. **Simpler Code**: Easier to understand and modify
2. **Faster Boot**: No unnecessary API calls
3. **Lower Power**: Fewer network requests
4. **Focused Purpose**: Single-purpose weather display
5. **Easier Maintenance**: Fewer dependencies

## Migration from v1

If you're migrating from the multi-screen v1:

1. Copy your WiFi credentials from v1
2. Copy your OpenWeatherMap API key
3. Remove Spotify and transport credentials (not needed)
4. Update to new simplified `main.ino` structure

## Upgrading to v2

To use this version:

1. Delete old `src/credentials.h`
2. Copy new `src/credentials.h.example` to `src/credentials.h`
3. Fill in WiFi and weather API credentials only
4. Build and upload
