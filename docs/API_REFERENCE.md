# API Reference

## OpenWeatherMap API

### Current Weather
```
GET http://api.openweathermap.org/data/2.5/weather?id={CITY_ID}&APPID={API_KEY}&units=metric
```

**City ID**: Bern = `2661552`

**Response fields used**:
- `main.temp`: Current temperature (Celsius)
- `weather[0].id`: Weather condition code (200-599 = precipitation)

### Air Pollution (Pollen Proxy)
```
GET http://api.openweathermap.org/data/2.5/air_pollution?lat=46.9480&lon=7.4474&appid={API_KEY}
```

**Response fields used**:
- `list[0].components.pm2_5`: PM2.5 concentration

**PM2.5 → pollen mapping**:
- < 20: low
- 20-40: moderate
- 40-60: high
- > 60: very high

### Forecast (Rain Detection)
```
GET http://api.openweathermap.org/data/2.5/forecast?id={CITY_ID}&APPID={API_KEY}&units=metric&cnt=8
```

Fetches 8 × 3-hour slots (24h ahead). Checks `list[i].weather[0].id` for rain codes (200-599).

### Free Tier Limits
- 60 calls/minute, 1M calls/month
- Sign up: https://openweathermap.org/api

---

## AareGuru API

```
GET http://aareguru.existenz.ch/v2018/today?city=bern&app=li.richert.smartframe&version=0.0.1
```

**Response fields used**:
- `aare`: Water temperature (Celsius)
- `text`: Commentary text in Bärndütsch (e.g. "Wird äuä wermer")
- `text_short`: Shorter version

---

## Withings API

### Token Refresh
```
POST https://wbsapi.withings.net/v2/oauth2
Content-Type: application/x-www-form-urlencoded

action=requesttoken&grant_type=refresh_token&refresh_token={TOKEN}&client_id={ID}&client_secret={SECRET}
```

**Important**: Do NOT use `oauth2.withings.com` — DNS fails on ESP32.

**Response**: Check `status` field (0 = success, 401 = expired, 503 = invalid refresh token)
- `body.access_token`: New access token
- `body.refresh_token`: New refresh token (MUST save — old one is invalidated)

### Weight Measurements
```
POST https://wbsapi.withings.net/measure
Authorization: Bearer {ACCESS_TOKEN}
Content-Type: application/x-www-form-urlencoded

action=getmeas&startdate={UNIX_TS}&enddate={UNIX_TS}
```

**Do NOT include `userid` parameter.**

**Response**:
- `status`: 0 = success, 401 = token expired
- `body.measuregrps[0].measures[0].value`: Latest weight (divide by 1000 for kg)
- `body.measuregrps[0].date`: Measurement Unix timestamp

### Setup Script
```bash
python3 scripts/get_withings_credentials.py
```

---

## Strava API

### Token Refresh
```
POST https://www.strava.com/oauth/token
Content-Type: application/x-www-form-urlencoded

client_id={ID}&client_secret={SECRET}&grant_type=refresh_token&refresh_token={TOKEN}
```

**Response**:
- `access_token`: New access token (expires every 6 hours)
- `refresh_token`: New refresh token (MUST save — old one is invalidated)

### Last Activity
```
GET https://www.strava.com/api/v3/athlete/activities?per_page=1&page=1
Authorization: Bearer {ACCESS_TOKEN}
```

**Response** (array of 1):
- `type` / `sport_type`: Activity type (Run, Ride, MountainBikeRide, etc.)
- `distance`: Distance in meters
- `moving_time`: Duration in seconds
- `start_date_local`: ISO 8601 date string

### Rate Limits
- 200 requests/15 min, 2000/day

### Setup Script
```bash
python3 scripts/get_strava_credentials.py
```

---

## Error Handling

All APIs use this pattern:
1. Check HTTP response code (200 = success)
2. For Withings: also check `status` field in JSON body
3. On 401: refresh token, retry once
4. On failure: show fallback values ("--.-", "err", "n/a")
5. All HTTPS connections use `setInsecure()` (no cert verification)
6. All HTTPS clients use 10-second timeout
