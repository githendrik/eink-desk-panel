# Migration Plan: OpenWeatherMap -> Open-Meteo

## Overview

Replace OpenWeatherMap API with Open-Meteo for weather data. This removes the API key requirement, improves forecast resolution (1 km via MeteoSwiss ICON-CH1), adds UV index, and targets Herrenschwanden instead of Bern.

**Coordinates:** 46.9725, 7.4528 (Herrenschwanden, Switzerland)

## Changes

### 1. Replace weather + forecast with single Open-Meteo call

Currently 2 separate OpenWeatherMap calls (current + forecast). Open-Meteo can return both in one request:

```
https://api.open-meteo.com/v1/forecast?latitude=46.9725&longitude=7.4528
  &current=temperature_2m,weather_code
  &hourly=weather_code,precipitation_probability
  &daily=uv_index_max
  &forecast_hours=8
  &timezone=Europe/Zurich
  &models=meteoswiss_icon_ch1
```

- `current.temperature_2m` replaces `main.temp`
- `current.weather_code` for rain detection (WMO codes 51-67, 80-82, 95-99 = precipitation; replaces OWM IDs 200-599)
- `hourly.weather_code` array for rain forecast logic (true hourly instead of 3h slots)
- `daily.uv_index_max` for UV warning display

**File:** `src/main_screen.h` (lines 406-510)

### 2. Replace air quality / pollen call

Open-Meteo air quality endpoint:

```
https://air-quality-api.open-meteo.com/v1/air-quality?latitude=46.9725&longitude=7.4528
  &current=pm2_5
```

Same PM2.5 -> pollen level mapping stays as-is.

**File:** `src/main_screen.h` (lines 512-542)

### 3. Add UV index (new feature)

Add `String uvIndex` variable. Fetch `daily.uv_index_max` from the weather request in step 1.

### 4. Bottom-left display priority logic

The bottom-left slot shows the most relevant alert, in priority order:

1. **Rain status** -- if raining or rain approaching/ending (label: "Weather")
2. **UV warning** -- if daily max UV >= 6 (label: "UV Index"), display value like "high", "very high", "extreme"
3. **Pollen level** -- default fallback (label: "Pollen")

UV thresholds (WHO scale):
- 0-5: Don't show (low/moderate)
- 6-7: Show "high"
- 8-10: Show "very high"
- 11+: Show "extreme"

**File:** `src/main_screen.h` (lines 850-869)

### 5. Simplify config (remove OpenWeather API key)

- **config_manager.h**: Remove `openWeatherApiKey`, `cityId`, `saveOpenWeather()`, `"openweather"` NVS namespace
- **web_dashboard.h**: Remove "OpenWeather" section (API key + city ID inputs), remove from save/status JSON
- **credentials.h.example**: Remove `OPENWEATHER_API_KEY`

### 6. Update refresh interval

Change from 15 min to 10 min in `main.ino:254` (`1000UL*60*15` -> `1000UL*60*10`).

### 7. Update location label

Decide whether to change the display label from "Bern" to something else (e.g. keep "Bern", or use "Local"). Herrenschwanden is close enough that "Bern" may still be appropriate.

## Summary

| Before | After |
|--------|-------|
| 3 HTTP calls (current + forecast + air quality) | 2 HTTP calls (weather+forecast combined, air quality) |
| API key required | No API key |
| Bern coordinates (46.9480, 7.4474) | Herrenschwanden (46.9725, 7.4528) |
| 11 km resolution (ICON Global) | 1 km resolution (MeteoSwiss ICON-CH1) |
| 3h forecast slots | Hourly forecast slots |
| No UV index | UV index with smart display |
| 15 min refresh | 10 min refresh |
| ~144 calls/day | ~144 calls/day (well within 10k/day free limit) |
