# API Reference

## Spotify API

### OAuth Token Endpoint
```
POST https://accounts.spotify.com/api/token
Headers:
  Content-Type: application/x-www-form-urlencoded
Body:
  grant_type=refresh_token
  &refresh_token={REFRESH_TOKEN}
  &client_id={CLIENT_ID}
  &client_secret={CLIENT_SECRET}
```

### Currently Playing Track
```
GET https://api.spotify.com/v1/me/player/currently-playing
Headers:
  Authorization: Bearer {ACCESS_TOKEN}

Response: 200 OK (playing), 204 No Content (nothing playing), 401 Unauthorized (expired token)
```

## OpenWeatherMap API

### Current Weather Data
```
GET https://api.openweathermap.org/data/2.5/weather?q={CITY}&appid={API_KEY}&units=metric
```

### 5-Day Forecast
```
GET https://api.openweathermap.org/data/2.5/forecast?q={CITY}&appid={API_KEY}&units=metric
```

## Transport API

Configure based on your local transport authority. Example structure:
```
GET https://api.transport-api.com/departures?station={STATION_ID}&key={API_KEY}
```

## Utility Scripts

### get_spotify_refresh_token.py
Obtains a Spotify refresh token using OAuth2 authorization code flow.

Usage:
```bash
python scripts/get_spotify_refresh_token.py
```

Follow the browser prompts to authorize and copy the refresh token.

### convert_svg_to_epd.py
Converts SVG graphics to a format compatible with the E-Ink display.

Usage:
```bash
python scripts/convert_svg_to_epd.py input.svg output.h
```
