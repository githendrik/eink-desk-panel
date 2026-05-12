# API Reference

## OpenWeatherMap API

### Current Weather Data
```
GET https://api.openweathermap.org/data/2.5/weather?id={CITY_ID}&appid={API_KEY}&units=metric

Example:
GET https://api.openweathermap.org/data/2.5/weather?id=2661552&appid=YOUR_API_KEY&units=metric
```

### Response Fields Used
- `main.temp`: Current temperature in Celsius
- `main.temp_min`: Minimum temperature
- `main.temp_max`: Maximum temperature

### City IDs
- Bern, CH: 2661552
- Find others: https://openweathermap.org/find

### Air Pollution Data (for pollen estimate)
```
GET https://api.openweathermap.org/data/2.5/air_pollution?lat={LAT}&lon={LON}&appid={API_KEY}

Example:
GET https://api.openweathermap.org/data/2.5/air_pollution?lat=46.9480&lon=7.4474&appid=YOUR_API_KEY
```

### Response Fields Used
- `list[0].components.pm2_5`: PM2.5 particle concentration

### PM2.5 Levels (used as pollen proxy)
- < 20: low
- 20-40: moderate
- 40-60: high
- > 60: very high

## AareGuru API

### Today's Aare Data
```
GET https://aareguru.existenz.ch/v2018/today?city=bern&app=li.richert.smartframe&version=0.0.1
```

### Response Fields Used
- `aare`: Water temperature in Celsius

## Error Codes

- `200`: Success
- `401`: Unauthorized (invalid API key)
- `404`: Not found (invalid city ID)
- `429`: Too many requests (rate limited)

## API Registration

OpenWeatherMap offers a free tier:
- 60 API calls per minute
- 1,000,000 calls per month
- No credit card required
- Sign up at https://openweathermap.org/api
