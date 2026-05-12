# API Reference

## OpenWeatherMap API

### Current Weather Data
```
GET https://api.openweathermap.org/data/2.5/weather?id={CITY_ID}&appid={API_KEY}&units=metric

Example:
GET https://api.openweathermap.org/data/2.5/weather?id=2661552&appid=YOUR_API_KEY&units=metric
```

### Response Fields Used
- `weather[0].main`: Weather condition (e.g., "Clear", "Clouds", "Rain")
- `main.temp`: Current temperature in Celsius
- `main.temp_min`: Minimum temperature
- `main.temp_max`: Maximum temperature
- `name`: City name

### City IDs
- Bern, CH: 2661552
- Find others: https://openweathermap.org/find

## AareGuru API

### Today's Aare Data
```
GET https://aareguru.existenz.ch/v2018/today?city=bern&app=li.richert.smartframe&version=0.0.1
```

### Response Fields Used
- `aare`: Water temperature in Celsius
- `text`: Swimming condition description
- `time`: Timestamp of reading

## Error Codes

- `200`: Success
- `401`: Unauthorized (invalid API key)
- `404`: Not found (invalid city ID)
- `429`: Too many requests (rate limited)
