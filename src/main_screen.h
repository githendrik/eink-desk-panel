#ifndef WEATHER_SCREEN_H
#define WEATHER_SCREEN_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <time.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "remote_log.h"
#include "config_manager.h"
#include "qrcode.h"

// Panel-specific layout constants
#ifdef PANEL_579
  #define PANEL_ROTATION  180
  #define LAYOUT_W        396   // visible left-half width
  #define LAYOUT_H        272
#else
  #define PANEL_ROTATION  0
  #define LAYOUT_W        400
  #define LAYOUT_H        300
#endif

extern ConfigManager config;

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

String temperature;
String aareTemp;
String aareText;
String pollenLevel;
String rainStatus;  // empty = no rain, otherwise "Rain in 3h/2h/1h", "Rain imminent", or "Raining"
int uvIndexMax = 0; // Daily max UV index from Open-Meteo
String weight = "--.-";
String weightTrend = "n/a";

// Bottom-right display mode: 0 = UV index, 1 = weight
int bottomRightMode = 1;

// ---- Rich weather model (used by the 4.2" forecast layout) ----
// The 5.79" layout keeps using the simple globals above; this struct carries
// the extra Open-Meteo fields without changing that behaviour.
#define WX_HOURS 12
#define WX_DAYS  3

struct WxNow {
  int   temp;      // degC
  int   feels;     // apparent temperature, degC
  int   wind;      // km/h
  int   humidity;  // %
  int   code;      // WMO weather code
  bool  isDay;
};
struct WxHour {
  int hour;  // 0-23, local time
  int temp;  // degC
  int pop;   // precipitation probability, %
  int code;  // WMO weather code
};
struct WxDay {
  int tmin, tmax;  // degC
  int pop;         // max precipitation probability, %
  int code;        // WMO weather code
  int wday;        // 0=Mon .. 6=Sun
};
struct WeatherData {
  WxNow  now;
  WxHour hours[WX_HOURS];
  int    hourCount;
  WxDay  days[WX_DAYS];
  int    dayCount;
  String sunrise;   // "HH:MM"
  String sunset;    // "HH:MM"
  int    todayYear;   // e.g. 2026
  int    todayDay;    // day of month
  int    todayMonth;  // 1-12
  int    todayWday;   // 0=Mon .. 6=Sun
  bool   valid;
};
WeatherData wx = {};

// "2026-08-17T08:00:00" -> minutes since midnight (488). -1 if unparseable.
static int isoTimeToMinutes(const String& iso) {
  int t = iso.indexOf('T');
  if (t < 0 || (int)iso.length() < t + 6) return -1;
  int hh = iso.substring(t + 1, t + 3).toInt();
  int mm = iso.substring(t + 4, t + 6).toInt();
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return -1;
  return hh * 60 + mm;
}

// Minutes since local midnight, or -1 while the clock is not yet set.
static int localMinutesNow() {
  time_t now = time(NULL);
  if (now < 1000000000) return -1;
  struct tm tmv;
  localtime_r(&now, &tmv);
  return tmv.tm_hour * 60 + tmv.tm_min;
}

// Sakamoto's algorithm. Returns 0=Mon .. 6=Sun.
static int weekdayFromDate(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  int dow = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;  // 0=Sun
  return (dow + 6) % 7;                                          // 0=Mon
}

// Calendar data
struct CalendarEvent {
  String summary;
  String startTime;  // "HH:MM" or "" for all-day
  bool allDay;
  int startMin;      // minutes since local midnight, -1 if unknown
  int endMin;        // minutes since local midnight, -1 if unknown
};
struct CalendarDay {
  String label;      // "Today" or "Tomorrow"
  CalendarEvent events[5];
  int eventCount;
};
CalendarDay calendarDays[2];  // [0]=today, [1]=tomorrow

// True once a timed event scheduled for *today* has finished. All-day events
// never count as past. If the clock is not set we show everything rather than
// risk hiding upcoming events.
static bool calendarEventIsPast(const CalendarEvent& ev) {
  if (ev.allDay) return false;
  int nowMin = localMinutesNow();
  if (nowMin < 0) return false;
  int endMin = ev.endMin;
  if (endMin < 0) {
    if (ev.startMin < 0) return false;
    endMin = ev.startMin + 60;   // no end given: assume an hour
  }
  return endMin <= nowMin;
}

int calendarTotalEvents = 0;
String uselessFact = "";
int calendarFailCount = 0;

// Staleness tracking: count consecutive failures per source
// After 2 consecutive failures, data is considered stale
int openmeteoFailCount = 0;
int aareFailCount = 0;
int pollenFailCount = 0;
const int STALE_THRESHOLD = 2;

// Build a staleness message for display in the aare text area
String getStalenessMessage() {
  String msg = "";
  if (openmeteoFailCount >= STALE_THRESHOLD) {
    msg += "Weather stale";
  }
  if (aareFailCount >= STALE_THRESHOLD) {
    if (msg.length() > 0) msg += " | ";
    msg += "Aare stale";
  }
  if (pollenFailCount >= STALE_THRESHOLD) {
    if (msg.length() > 0) msg += " | ";
    msg += "Pollen stale";
  }
  return msg;
}

String jsonBuffer;
JSONVar myObject;
String aareJsonBuffer;
JSONVar aareObject;
String pollenJsonBuffer;
JSONVar pollenObject;
String weightJsonBuffer;
JSONVar weightObject;

String getPollenText(int level) {
  switch(level) {
    case 0: return "none";
    case 1: return "low";
    case 2: return "moderate";
    case 3: return "high";
    case 4: return "very high";
    case 5: return "extreme";
    default: return "n/a";
  }
}

String getTrendText(float change) {
  if (change < -0.5) return "losing";
  if (change > 0.5) return "gaining";
  return "stable";
}

void fetch_withings_token(int& httpResponseCode) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected - cannot fetch Withings token");
    return;
  }
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  
  http.begin(client, "https://wbsapi.withings.net/v2/oauth2");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "action=requesttoken&grant_type=refresh_token&refresh_token=" + config.withingsRefreshToken + 
                    "&client_id=" + config.withingsClientId + 
                    "&client_secret=" + config.withingsClientSecret;
  
  httpResponseCode = http.POST(postData);
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    remote_log(LOG_INFO, "Withings_Token", "Success");
    Serial.println("Withings token response:");
    Serial.println(payload);
    
    JSONVar tokenObject = JSON.parse(payload);
    
    if (JSON.typeof(tokenObject) != "undefined") {
      int tokenStatus = (int)round((double)tokenObject["status"]);
      if (tokenStatus != 0) {
        Serial.print("Token refresh failed, status: ");
        Serial.println(tokenStatus);
        httpResponseCode = -1;
        http.end();
        return;
      }
      // New endpoint returns token inside body object
      String token;
      String newRefresh;
      if (JSON.typeof(tokenObject["body"]["access_token"]) != "undefined") {
        token = JSON.stringify(tokenObject["body"]["access_token"]);
      } else if (JSON.typeof(tokenObject["access_token"]) != "undefined") {
        token = JSON.stringify(tokenObject["access_token"]);
      }
      if (JSON.typeof(tokenObject["body"]["refresh_token"]) != "undefined") {
        newRefresh = JSON.stringify(tokenObject["body"]["refresh_token"]);
      }
      if (token.length() > 0) {
        token.replace("\"", "");
        config.withingsAccessToken = token;
        if (newRefresh.length() > 0) {
          newRefresh.replace("\"", "");
          config.withingsRefreshToken = newRefresh;
        }
        config.saveWithings();
        Serial.println("Token updated successfully");
      }
    }
  } else {
    remote_log(LOG_ERROR, "Withings_Token", "Failed to refresh token. HTTP: " + String(httpResponseCode));
    Serial.print("Failed to refresh Withings token. HTTP code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void fetch_weight_data(int& httpResponseCode, bool retry = true) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    return;
  }

  if (config.withingsAccessToken.length() == 0) {
    Serial.println("No access token, fetching...");
    fetch_withings_token(httpResponseCode);
    if (httpResponseCode != 200) return;
  }

  time_t now = time(NULL);
  if (now < 1000000000) {
    Serial.println("Time not set yet, skipping weight fetch");
    return;
  }
  time_t sixMonthsAgo = now - (180 * 24 * 60 * 60);
  
  String serverPath = "https://wbsapi.withings.net/measure";
  Serial.print("Withings URL: ");
  Serial.println(serverPath);
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(client, serverPath);
  http.addHeader("Authorization", "Bearer " + config.withingsAccessToken);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "action=getmeas&startdate=" + String(sixMonthsAgo) + 
                    "&enddate=" + String(now);
  
  Serial.print("Withings POST: ");
  Serial.println(postData);
  
  httpResponseCode = http.POST(postData);

  Serial.print("Withings HTTP code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {
    weightJsonBuffer = http.getString();
    remote_log(LOG_INFO, "Withings_Weight", "Success HTTP 200");
    Serial.println(weightJsonBuffer);
    weightObject = JSON.parse(weightJsonBuffer);

    if (JSON.typeof(weightObject) == "undefined") {
      Serial.println("Parsing weight data failed!");
      http.end();
      return;
    }

    int apiStatus = (int)round((double)weightObject["status"]);
    Serial.print("Withings API status: ");
    Serial.println(apiStatus);
    
    if (apiStatus != 0) {
      Serial.println("Withings API returned error status");
      Serial.print("Error: ");
      String errorMsg = JSON.stringify(weightObject["error"]);
      Serial.println(errorMsg);
      
      // Status 401 = expired token, refresh and retry once
      if (apiStatus == 401 && retry) {
        Serial.println("Token expired (API status 401), refreshing...");
        http.end();
        fetch_withings_token(httpResponseCode);
        if (httpResponseCode == 200) {
          fetch_weight_data(httpResponseCode, false);
        } else {
          weight = "--.-";
          weightTrend = "err";
        }
        return;
      }
      
      weight = "--.-";
      weightTrend = "err";
      http.end();
      return;
    }

    if (JSON.typeof(weightObject["body"]["measuregrps"]) != "undefined") {
      int groupsCount = weightObject["body"]["measuregrps"].length();
      Serial.print("Measure groups count: ");
      Serial.println(groupsCount);
      
      if (groupsCount > 0) {
        int measuresCount = weightObject["body"]["measuregrps"][0]["measures"].length();
        if (measuresCount > 0) {
          double latestWeight = (double)weightObject["body"]["measuregrps"][0]["measures"][0]["value"];
          weight = String(latestWeight / 1000.0, 1);
          
          // Check if measurement is stale (older than 7 days)
          long measureDate = (long)weightObject["body"]["measuregrps"][0]["date"];
          long sevenDays = 7 * 24 * 60 * 60;
          if ((now - measureDate) > sevenDays) {
            weightTrend = "STALE";
          } else if (groupsCount >= 2) {
            int oldMeasuresCount = weightObject["body"]["measuregrps"][groupsCount - 1]["measures"].length();
            if (oldMeasuresCount > 0) {
              double oldWeight = (double)weightObject["body"]["measuregrps"][groupsCount - 1]["measures"][0]["value"];
              float change = (latestWeight - oldWeight) / 1000.0;
              weightTrend = getTrendText(change);
            } else {
              weightTrend = "stable";
            }
          } else {
            weightTrend = "stable";
          }
          
          Serial.print("Weight: ");
          Serial.print(weight);
          Serial.print(" kg, Trend: ");
          Serial.println(weightTrend);
        }
      } else {
        Serial.println("No weight measurements found");
        weight = "--.-";
        weightTrend = "n/a";
      }
    }
  } else if (httpResponseCode == 401 && retry) {
    remote_log(LOG_WARN, "Withings_Weight", "Token expired (401), refreshing");
    Serial.println("Token expired, refreshing...");
    fetch_withings_token(httpResponseCode);
    if (httpResponseCode == 200) {
      Serial.println("Token refreshed, retrying weight fetch...");
      http.end();
      fetch_weight_data(httpResponseCode, false);
      return;
    }
  } else {
    remote_log(LOG_ERROR, "Withings_Weight", "API Error HTTP " + String(httpResponseCode));
    Serial.print("Withings API error: ");
    Serial.println(httpResponseCode);
    weight = "--.-";
    weightTrend = "err";
  }

  http.end();
}

// Returns true on success, false on failure
bool fetch_openmeteo_data() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    openmeteoFailCount++;
    return false;
  }

  bool currentlyRaining = false;

  // Open-Meteo: weather + hourly forecast + daily UV in one call
  // Herrenschwanden coordinates: 46.9725, 7.4528
  String serverPath = "http://api.open-meteo.com/v1/forecast?latitude=46.9725&longitude=7.4528"
    "&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m,relative_humidity_2m,is_day"
    "&hourly=temperature_2m,precipitation_probability,weather_code"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,uv_index_max,sunrise,sunset"
    "&forecast_hours=" STRINGIFY(WX_HOURS)
    "&forecast_days=" STRINGIFY(WX_DAYS)
    "&timezone=Europe/Zurich";
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverPath);
  int rc = http.GET();

  if (rc == 200) {
    jsonBuffer = http.getString();
    Serial.println(jsonBuffer);
    myObject = JSON.parse(jsonBuffer);

    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      http.end();
      openmeteoFailCount++;
      return false;
    }

    // Current temperature
    temperature = String((int)round((double)myObject["current"]["temperature_2m"]));

    // Current rain detection via WMO weather codes
    // 51-67: drizzle/rain, 71-77: snow, 80-82: rain showers, 85-86: snow showers, 95-99: thunderstorm
    int weatherCode = (int)(double)myObject["current"]["weather_code"];
    currentlyRaining = (weatherCode >= 51 && weatherCode <= 67) ||
                       (weatherCode >= 80 && weatherCode <= 82) ||
                       (weatherCode >= 95 && weatherCode <= 99);
    if (currentlyRaining) {
      rainStatus = "Raining";
      Serial.println("Currently raining");
    } else {
      rainStatus = "";
    }

    // Daily UV index max
    if (JSON.typeof(myObject["daily"]["uv_index_max"]) != "undefined") {
      uvIndexMax = (int)round((double)myObject["daily"]["uv_index_max"][0]);
      Serial.print("UV Index Max: ");
      Serial.println(uvIndexMax);
    }

    // Hourly forecast for rain prediction
    if (JSON.typeof(myObject["hourly"]["weather_code"]) != "undefined") {
      int hourlyCount = myObject["hourly"]["weather_code"].length();

      for (int i = 0; i < hourlyCount && i < 8; i++) {
        int fCode = (int)(double)myObject["hourly"]["weather_code"][i];
        bool isRain = (fCode >= 51 && fCode <= 67) ||
                      (fCode >= 80 && fCode <= 82) ||
                      (fCode >= 95 && fCode <= 99);
        int hoursAway = i; // Each slot is ~1 hour from the current hour

        if (currentlyRaining) {
          // Look for when rain ends
          if (!isRain) {
            if (hoursAway >= 3) {
              rainStatus = "Rain ends in 3h";
            } else if (hoursAway >= 2) {
              rainStatus = "Rain ends in 2h";
            } else if (hoursAway >= 1) {
              rainStatus = "Rain end soon";
            }
            Serial.print("Rain ending: ");
            Serial.println(rainStatus);
            break;
          }
        } else {
          // Look for upcoming rain
          if (isRain) {
            if (hoursAway >= 4) break;
            if (hoursAway >= 3) {
              rainStatus = "Rain in 3h";
            } else if (hoursAway >= 2) {
              rainStatus = "Rain in 2h";
            } else if (hoursAway >= 1) {
              rainStatus = "Rain in 1h";
            } else {
              rainStatus = "Rain imminent";
            }
            Serial.print("Rain forecast: ");
            Serial.println(rainStatus);
            break;
          }
        }
      }
    }

    // ---- Rich model for the 4.2" forecast layout ----
    {
      JSONVar cur = myObject["current"];
      wx.now.temp     = (int)round((double)cur["temperature_2m"]);
      wx.now.feels    = (int)round((double)cur["apparent_temperature"]);
      wx.now.wind     = (int)round((double)cur["wind_speed_10m"]);
      wx.now.humidity = (int)round((double)cur["relative_humidity_2m"]);
      wx.now.code     = weatherCode;
      wx.now.isDay    = ((int)(double)cur["is_day"]) != 0;

      // Hourly: temperature / precipitation probability / condition
      JSONVar hTime = myObject["hourly"]["time"];
      JSONVar hTemp = myObject["hourly"]["temperature_2m"];
      JSONVar hPop  = myObject["hourly"]["precipitation_probability"];
      JSONVar hCode = myObject["hourly"]["weather_code"];
      wx.hourCount = 0;
      if (JSON.typeof(hTime) != "undefined") {
        int n = hTime.length();
        if (n > WX_HOURS) n = WX_HOURS;
        for (int i = 0; i < n; i++) {
          // "2026-08-16T20:00" -> hour 20
          String ts = (const char*)hTime[i];
          int tPos = ts.indexOf('T');
          wx.hours[i].hour = (tPos >= 0) ? ts.substring(tPos + 1, tPos + 3).toInt() : 0;
          wx.hours[i].temp = (int)round((double)hTemp[i]);
          wx.hours[i].pop  = (JSON.typeof(hPop[i]) != "undefined")
                             ? (int)round((double)hPop[i]) : 0;
          wx.hours[i].code = (int)(double)hCode[i];
          wx.hourCount++;
        }
      }

      // Daily: min/max, precipitation probability, condition
      JSONVar dTime = myObject["daily"]["time"];
      wx.dayCount = 0;
      if (JSON.typeof(dTime) != "undefined") {
        int n = dTime.length();
        if (n > WX_DAYS) n = WX_DAYS;
        for (int i = 0; i < n; i++) {
          String ds = (const char*)dTime[i];  // "YYYY-MM-DD"
          int yy = ds.substring(0, 4).toInt();
          int mm = ds.substring(5, 7).toInt();
          int dd = ds.substring(8, 10).toInt();
          wx.days[i].wday = weekdayFromDate(yy, mm, dd);
          wx.days[i].tmax = (int)round((double)myObject["daily"]["temperature_2m_max"][i]);
          wx.days[i].tmin = (int)round((double)myObject["daily"]["temperature_2m_min"][i]);
          wx.days[i].code = (int)(double)myObject["daily"]["weather_code"][i];
          wx.days[i].pop  = (int)round((double)myObject["daily"]["precipitation_probability_max"][i]);
          wx.dayCount++;
          if (i == 0) {
            wx.todayYear  = yy;
            wx.todayDay   = dd;
            wx.todayMonth = mm;
            wx.todayWday  = wx.days[0].wday;
          }
        }
      }

      // Sunrise / sunset for today ("2026-08-16T06:28" -> "06:28")
      if (JSON.typeof(myObject["daily"]["sunrise"][0]) != "undefined") {
        String s = (const char*)myObject["daily"]["sunrise"][0];
        int tPos = s.indexOf('T');
        if (tPos >= 0) wx.sunrise = s.substring(tPos + 1, tPos + 6);
      }
      if (JSON.typeof(myObject["daily"]["sunset"][0]) != "undefined") {
        String s = (const char*)myObject["daily"]["sunset"][0];
        int tPos = s.indexOf('T');
        if (tPos >= 0) wx.sunset = s.substring(tPos + 1, tPos + 6);
      }

      wx.valid = true;
    }

    Serial.print("Temperature: ");
    Serial.println(temperature);
    remote_log(LOG_INFO, "OpenMeteo", "Success. Temp: " + temperature + " Rain: " + rainStatus + " UV: " + String(uvIndexMax));
    http.end();
    openmeteoFailCount = 0;
    return true;
  } else {
    remote_log(LOG_ERROR, "OpenMeteo", "API Error HTTP: " + String(rc));
    Serial.print("Weather API error: ");
    Serial.println(rc);
    http.end();
    openmeteoFailCount++;
    return false;
  }
}

// Returns true on success, false on failure
bool fetch_pollen_data() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    pollenFailCount++;
    return false;
  }

  // Google Pollen API (specifically GRASS)
  if (config.googlePollenApiKey.length() > 0) {
    String pollenServerPath = "https://pollen.googleapis.com/v1/forecast:lookup?key=" + config.googlePollenApiKey + "&location.latitude=46.9725&location.longitude=7.4528&days=1";
    
    // Google API requires HTTPS. Using setInsecure to bypass cert checks for simplicity constraint.
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    
    HTTPClient httpPollen;
    httpPollen.begin(secureClient, pollenServerPath);
    int rc = httpPollen.GET();

    if (rc == 200) {
      pollenJsonBuffer = httpPollen.getString();
      remote_log(LOG_INFO, "Pollen_Google", "Success");
      pollenObject = JSON.parse(pollenJsonBuffer);

      if (JSON.typeof(pollenObject) != "undefined") {
        int maxUpi = -1;
        JSONVar dailyInfo = pollenObject["dailyInfo"][0];
        if (JSON.typeof(dailyInfo) != "undefined") {
          JSONVar pollenTypeInfo = dailyInfo["pollenTypeInfo"];
          if (JSON.typeof(pollenTypeInfo) != "undefined") {
            for (int i = 0; i < pollenTypeInfo.length(); i++) {
              String code = (const char*)pollenTypeInfo[i]["code"];
              if (code == "GRASS") {
                JSONVar indexInfo = pollenTypeInfo[i]["indexInfo"];
                if (JSON.typeof(indexInfo) != "undefined") {
                  maxUpi = (int)indexInfo["value"];
                }
                break;
              }
            }
          }
        }

        if (maxUpi >= 0) {
          if (maxUpi == 0) pollenLevel = "none";
          else if (maxUpi == 1) pollenLevel = "very low";
          else if (maxUpi == 2) pollenLevel = "low";
          else if (maxUpi == 3) pollenLevel = "moderate";
          else if (maxUpi == 4) pollenLevel = "high";
          else if (maxUpi >= 5) pollenLevel = "very high";
        } else {
          pollenLevel = "n/a";
        }
        Serial.print("Pollen (GRASS) level: ");
        Serial.println(pollenLevel);
      }
      httpPollen.end();
      pollenFailCount = 0;
      return true;
    } else {
      remote_log(LOG_ERROR, "Pollen_Google", "Error HTTP: " + String(rc));
      Serial.print("Pollen API error: ");
      Serial.println(rc);
      httpPollen.end();
      pollenFailCount++;
      return false;
    }
  } else {
    // Open-Meteo Fallback
    String pollenServerPath = "http://air-quality-api.open-meteo.com/v1/air-quality?latitude=46.9725&longitude=7.4528&current=pm2_5";
    WiFiClient client;
    HTTPClient httpPollen;
    httpPollen.begin(client, pollenServerPath);
    int rc = httpPollen.GET();

    if (rc == 200) {
      pollenJsonBuffer = httpPollen.getString();
      remote_log(LOG_INFO, "Pollen_OpenMeteo", "Success");
      pollenObject = JSON.parse(pollenJsonBuffer);

      if (JSON.typeof(pollenObject) != "undefined") {
        double pm25Value = (double)pollenObject["current"]["pm2_5"];
        int pm25 = (int)pm25Value;
        
        if (pm25 < 20) pollenLevel = "low";
        else if (pm25 < 40) pollenLevel = "moderate";
        else if (pm25 < 60) pollenLevel = "high";
        else pollenLevel = "very high";
        Serial.print("Pollen (PM2.5) level: ");
        Serial.println(pollenLevel);
      }
      httpPollen.end();
      pollenFailCount = 0;
      return true;
    } else {
      remote_log(LOG_ERROR, "Pollen_OpenMeteo", "Error HTTP: " + String(rc));
      Serial.print("Pollen fallback API error: ");
      Serial.println(rc);
      httpPollen.end();
      pollenFailCount++;
      return false;
    }
  }
}

// Returns true on success, false on failure
bool fetch_aare_data() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    aareFailCount++;
    return false;
  }

  WiFiClient client;
  String aareServerPath = "http://aareguru.existenz.ch/v2018/today?city=bern&app=li.richert.smartframe&version=0.0.1";
  HTTPClient httpAare;
  httpAare.begin(client, aareServerPath);
  int rc = httpAare.GET();

  if (rc == 200) {
    aareJsonBuffer = httpAare.getString();
    remote_log(LOG_INFO, "Aare", "Success HTTP 200");
    Serial.println(aareJsonBuffer);
    aareObject = JSON.parse(aareJsonBuffer);

    if (JSON.typeof(aareObject) == "undefined") {
      Serial.println("Parsing aare data failed!");
      httpAare.end();
      aareFailCount++;
      return false;
    }

    aareTemp = String((int)round((double)aareObject["aare"]));
    aareText = JSON.stringify(aareObject["text"]);
    aareText.replace("\"", "");

    Serial.print("String aare temp: ");
    Serial.println(aareTemp);
    Serial.print("Aare text: ");
    Serial.println(aareText);
    httpAare.end();
    aareFailCount = 0;
    return true;
  } else {
    remote_log(LOG_ERROR, "Aare", "Error HTTP: " + String(rc));
    Serial.print("Aare API error: ");
    Serial.println(rc);
    httpAare.end();
    aareFailCount++;
    return false;
  }
}

// --- Calendar ---

void fetch_useless_fact();  // forward declaration

bool fetch_calendar_data() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    calendarFailCount++;
    return false;
  }

  if (config.calendarApiUrl.length() == 0) {
    Serial.println("No calendar API URL configured, skipping");
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(client, config.calendarApiUrl);
  if (config.calendarBearerToken.length() > 0) {
    http.addHeader("Authorization", "Bearer " + config.calendarBearerToken);
  }

  int rc = http.GET();

  if (rc == 200) {
    String payload = http.getString();
    Serial.println(payload);
    JSONVar calObj = JSON.parse(payload);

    if (JSON.typeof(calObj) == "undefined") {
      Serial.println("Parsing calendar data failed!");
      http.end();
      calendarFailCount++;
      return false;
    }

    calendarTotalEvents = 0;

    // Parse today and tomorrow
    const char* keys[] = {"today", "tomorrow"};
    for (int d = 0; d < 2; d++) {
      calendarDays[d].eventCount = 0;
      calendarDays[d].label = "";

      if (JSON.typeof(calObj[keys[d]]) == "undefined") continue;

      JSONVar day = calObj[keys[d]];
      if (JSON.typeof(day["label"]) != "undefined") {
        calendarDays[d].label = (const char*)day["label"];
      }

      if (JSON.typeof(day["events"]) != "undefined") {
        int count = day["events"].length();
        if (count > 5) count = 5;  // max 5 events per day (renderers cap further)

        for (int i = 0; i < count; i++) {
          JSONVar ev = day["events"][i];
          calendarDays[d].events[i].summary = "";
          calendarDays[d].events[i].startTime = "";
          calendarDays[d].events[i].allDay = false;
          calendarDays[d].events[i].startMin = -1;
          calendarDays[d].events[i].endMin = -1;

          if (JSON.typeof(ev["summary"]) != "undefined") {
            calendarDays[d].events[i].summary = (const char*)ev["summary"];
          }

          if (JSON.typeof(ev["allDay"]) != "undefined") {
            calendarDays[d].events[i].allDay = (bool)ev["allDay"];
          }

          if (!calendarDays[d].events[i].allDay && JSON.typeof(ev["start"]) != "undefined") {
            // Extract HH:MM from ISO 8601 "2026-06-09T08:00:00"
            String start = (const char*)ev["start"];
            int tPos = start.indexOf('T');
            if (tPos >= 0 && start.length() >= tPos + 6) {
              calendarDays[d].events[i].startTime = start.substring(tPos + 1, tPos + 6);
            }
            calendarDays[d].events[i].startMin = isoTimeToMinutes(start);
          }

          if (!calendarDays[d].events[i].allDay && JSON.typeof(ev["end"]) != "undefined") {
            calendarDays[d].events[i].endMin = isoTimeToMinutes((const char*)ev["end"]);
          }

          calendarDays[d].eventCount++;
          calendarTotalEvents++;
        }
      }
    }

    Serial.print("Calendar: ");
    Serial.print(calendarTotalEvents);
    Serial.println(" events total");
    remote_log(LOG_INFO, "Calendar", "Success. Events: " + String(calendarTotalEvents));
    http.end();
    calendarFailCount = 0;

    return true;
  } else {
    remote_log(LOG_ERROR, "Calendar", "API Error HTTP: " + String(rc));
    Serial.print("Calendar API error: ");
    Serial.println(rc);
    http.end();
    calendarFailCount++;
    return false;
  }
}

void fetch_useless_fact() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected - cannot fetch fact");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(client, "https://uselessfacts.jsph.pl/api/v2/facts/today");

  int rc = http.GET();

  if (rc == 200) {
    String payload = http.getString();
    JSONVar factObj = JSON.parse(payload);

    if (JSON.typeof(factObj) != "undefined" && JSON.typeof(factObj["text"]) != "undefined") {
      uselessFact = (const char*)factObj["text"];
      Serial.print("Useless fact: ");
      Serial.println(uselessFact);
    }
  } else {
    Serial.print("Useless fact API error: ");
    Serial.println(rc);
  }

  http.end();
}

// Legacy wrapper for initial boot fetch (calls all three)
void fetch_weather_data() {
  fetch_openmeteo_data();
  fetch_pollen_data();
  fetch_aare_data();
}

#ifndef PANEL_579
#include "screen_42.h"   // 4.2" forecast layout (needs wx / calendarDays above)
#endif






// ---- E-ink refresh tracking ---------------------------------------------
// The controller does differential (partial) updates against whatever we last
// told it is on the panel ("OLD RAM"). If any other screen paints the panel
// without updating this copy, the next partial update computes the wrong
// transitions and the old content bleeds through. So every screen that paints
// must either refresh this buffer or invalidate it.
static uint8_t* epdPrevFrame = nullptr;
static bool epdPrevFrameValid = false;

// Force the next main-screen draw to do a full refresh. Call this from any
// other screen that paints the panel.
static void epdInvalidatePrevFrame() { epdPrevFrameValid = false; }

static void epdEnsurePrevFrame(int frameSize) {
  if (epdPrevFrame != nullptr) return;
#ifdef PANEL_579
  epdPrevFrame = (uint8_t*)ps_malloc(frameSize);
#else
  epdPrevFrame = (uint8_t*)malloc(frameSize);
#endif
  if (epdPrevFrame != nullptr) memset(epdPrevFrame, 0xFF, frameSize);
  epdPrevFrameValid = false;   // panel contents unknown until we paint
}

// Full refresh policy: at most this many partial updates in a row, and never
// longer than this between full refreshes. Partial updates are cheap and
// flicker-free but accumulate ghosting, so they need bounding on both axes.
#define EPD_MAX_PARTIAL_UPDATES 4
#define EPD_FULL_REFRESH_INTERVAL_MS (1000UL * 60 * 60)   // 1 hour
#define EPD_BIG_CHANGE_PERCENT 10

// Push an already-composed frame, choosing a full or partial update.
static void epdPushFrame(uint8_t* ImageBW, int frameSize, bool& forceFullRefresh) {
  static int partialCount = 0;
  static unsigned long lastFullMs = 0;
  unsigned long nowMs = millis();

  // How much of the frame actually changed?
  uint32_t changed = 0;
  if (epdPrevFrameValid && epdPrevFrame != nullptr) {
    for (int i = 0; i < frameSize; i++) {
      if (epdPrevFrame[i] != ImageBW[i]) changed++;
    }
  }
  bool bigChange = (changed * 100UL) >= ((uint32_t)frameSize * EPD_BIG_CHANGE_PERCENT);
  bool overdue   = (lastFullMs == 0) || (nowMs - lastFullMs >= EPD_FULL_REFRESH_INTERVAL_MS);

  bool doFull = forceFullRefresh
             || !epdPrevFrameValid            // panel shows something we did not draw
             || partialCount >= EPD_MAX_PARTIAL_UPDATES
             || bigChange
             || overdue;

  if (doFull) {
    Serial.printf("EPD: full refresh (forced=%d valid=%d partials=%d changed=%lu%% overdue=%d)\n",
                  (int)forceFullRefresh, (int)epdPrevFrameValid, partialCount,
                  (unsigned long)(frameSize ? (changed * 100UL / frameSize) : 0), (int)overdue);
    EPD_Init();
    EPD_Clear();
    EPD_Display_Fast(ImageBW);
    partialCount = 0;
    lastFullMs = nowMs;
  } else {
    EPD_Init_Fast(Fast_Seconds_1_5s);

    // Tell the controller what is currently on the panel.
#ifdef PANEL_579
    EPD_SetRAMMP();
    EPD_SetRAMMA();
    EPD_WR_REG(0x26);
    {
      uint32_t tempcol = 0, templine = 0;
      const uint16_t rowBytes = SOURCE_BYTES * 2;
      for (uint32_t i = 0; i < IC_BYTES; i++) {
        EPD_WR_DATA8(*(epdPrevFrame + templine * rowBytes + tempcol));
        templine++;
        if (templine >= GATE_BITS) { tempcol++; templine = 0; }
      }
    }
    EPD_SetRAMSP();
    EPD_SetRAMSA();
    EPD_WR_REG(0xA6);
    {
      uint32_t tempcol = SOURCE_BYTES, templine = 0;
      const uint16_t rowBytes = SOURCE_BYTES * 2;
      for (uint32_t i = 0; i < IC_BYTES; i++) {
        EPD_WR_DATA8(*(epdPrevFrame + templine * rowBytes + tempcol));
        templine++;
        if (templine >= GATE_BITS) { tempcol++; templine = 0; }
      }
    }
#else
    EPD_Address_Set(0, 0, EPD_W - 1, EPD_H - 1);
    EPD_SetCursor(0, 0);
    EPD_WR_REG(0x26);
    for (int i = 0; i < frameSize; i++) EPD_WR_DATA8(epdPrevFrame[i]);
#endif

    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
    partialCount++;
  }

  forceFullRefresh = false;
  if (epdPrevFrame != nullptr) memcpy(epdPrevFrame, ImageBW, frameSize);
  epdPrevFrameValid = true;
}

// ---- Status Screen ----
// otaStateText: 0=idle, 1=checking, 2=update available, 3=no update
void display_status_screen(uint8_t* ImageBW, int otaState, const char* otaVersion) {
  char buffer[64];

  EPD_Init();
  EPD_Clear();
  EPD_Init_Fast(Fast_Seconds_1_5s);
  Paint_NewImage(ImageBW, EPD_W, EPD_H, PANEL_ROTATION, WHITE);
  EPD_Full(WHITE);

  int midX = LAYOUT_W / 2;
  int y = 15;

  // Title
  const char* title = "Device Status";
  int titleW = EPD_GetUTF8TextWidth(title, 24);
  EPD_ShowStringUTF8(midX - titleW / 2, y, title, 24, BLACK);
  y += 35;

  // Horizontal line
  EPD_DrawLine(20, y, LAYOUT_W - 20, y, BLACK);
  y += 12;

  // WiFi SSID
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "WiFi: %s", WiFi.SSID().c_str());
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 22;

  // IP
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "IP: %s", WiFi.localIP().toString().c_str());
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 22;

  // mDNS hostname
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "mDNS: %s.local", config.mdnsHostname.c_str());
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 22;

  // Signal strength
  int rssi = WiFi.RSSI();
  const char* sigText;
  if (rssi > -50) sigText = "Excellent";
  else if (rssi > -60) sigText = "Good";
  else if (rssi > -70) sigText = "Fair";
  else sigText = "Weak";
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Signal: %s (%ddBm)", sigText, rssi);
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 22;

  // Firmware version
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Firmware: %s", FIRMWARE_VERSION);
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 22;

  // Uptime
  unsigned long uptimeSec = millis() / 1000;
  int days = uptimeSec / 86400;
  int hours = (uptimeSec % 86400) / 3600;
  int mins = (uptimeSec % 3600) / 60;
  memset(buffer, 0, sizeof(buffer));
  if (days > 0) {
    snprintf(buffer, sizeof(buffer), "Uptime: %dd %dh %dm", days, hours, mins);
  } else {
    snprintf(buffer, sizeof(buffer), "Uptime: %dh %dm", hours, mins);
  }
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 30;

  // Horizontal line
  EPD_DrawLine(20, y, LAYOUT_W - 20, y, BLACK);
  y += 12;

  // OTA section
  if (otaState == 0) {
    const char* hint = "Press OK to check for updates";
    int hintW = EPD_GetUTF8TextWidth(hint, 16);
    EPD_ShowStringUTF8(midX - hintW / 2, y, hint, 16, BLACK);
  } else if (otaState == 1) {
    const char* msg = "Checking for updates...";
    int msgW = EPD_GetUTF8TextWidth(msg, 16);
    EPD_ShowStringUTF8(midX - msgW / 2, y, msg, 16, BLACK);
  } else if (otaState == 2 && otaVersion != nullptr) {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Update available: %s", otaVersion);
    int msgW = EPD_GetUTF8TextWidth(buffer, 16);
    EPD_ShowStringUTF8(midX - msgW / 2, y, buffer, 16, BLACK);
    y += 20;
    const char* hint2 = "Press OK to install";
    int h2W = EPD_GetUTF8TextWidth(hint2, 16);
    EPD_ShowStringUTF8(midX - h2W / 2, y, hint2, 16, BLACK);
  } else if (otaState == 3) {
    const char* msg = "Firmware is up to date";
    int msgW = EPD_GetUTF8TextWidth(msg, 16);
    EPD_ShowStringUTF8(midX - msgW / 2, y, msg, 16, BLACK);
  }

  // Footer
  const char* footer = "MENU to go back";
  int footW = EPD_GetUTF8TextWidth(footer, 12);
  EPD_ShowStringUTF8(midX - footW / 2, EPD_H - 18, footer, 12, BLACK);

  EPD_Display_Fast(ImageBW);
  epdInvalidatePrevFrame();   // panel no longer shows the main screen
}

// ---- OTA Update Progress Screen ----
void display_ota_screen(uint8_t* ImageBW, const char* version, int percent) {
  static bool otaScreenInitialized = false;
  char buffer[64];

  int midX = LAYOUT_W / 2;
  int barX = midX - 120;
  int barY = 130;
  int barW = 240;
  int barH = 26;

  if (!otaScreenInitialized) {
    // First call: full clear and draw static elements
    EPD_Init();
    EPD_Clear();
    EPD_Init_Fast(Fast_Seconds_1_5s);
    Paint_NewImage(ImageBW, EPD_W, EPD_H, PANEL_ROTATION, WHITE);
    EPD_Full(WHITE);

    // Title
    const char* title = "Updating Firmware";
    int titleW = EPD_GetUTF8TextWidth(title, 24);
    EPD_ShowStringUTF8(midX - titleW / 2, 50, title, 24, BLACK);

    // Version line
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s -> %s", FIRMWARE_VERSION, version);
    int verW = EPD_GetUTF8TextWidth(buffer, 16);
    EPD_ShowStringUTF8(midX - verW / 2, 90, buffer, 16, BLACK);

    // Progress bar outline
    EPD_DrawRectangle(barX, barY, barX + barW, barY + barH, BLACK, 0);

    // Warning
    const char* warn = "Do not power off!";
    int warnW = EPD_GetUTF8TextWidth(warn, 12);
    EPD_ShowStringUTF8(midX - warnW / 2, 230, warn, 12, BLACK);

    // Fill progress (if > 0 on first call)
    if (percent > 0) {
      int fillW = (barW - 4) * percent / 100;
      EPD_DrawRectangle(barX + 2, barY + 2, barX + 2 + fillW, barY + barH - 2, BLACK, 1);
    }

    // Percentage text
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%d%%", percent);
    int pctW = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(midX - pctW / 2, 175, buffer, 24, BLACK);

    EPD_Display_Fast(ImageBW);
    otaScreenInitialized = true;
  } else {
    // Subsequent calls: only update progress bar fill and percentage
    EPD_ClearWindows(barX + 2, barY + 2, barX + barW - 2, barY + barH - 2, WHITE);
    EPD_ClearWindows(midX - 60, 175, midX + 60, 203, WHITE);

    // Redraw progress fill
    if (percent > 0) {
      int fillW = (barW - 4) * percent / 100;
      EPD_DrawRectangle(barX + 2, barY + 2, barX + 2 + fillW, barY + barH - 2, BLACK, 1);
    }

    // Redraw percentage text
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%d%%", percent);
    int pctW = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(midX - pctW / 2, 175, buffer, 24, BLACK);

    // Full partial update (dual-IC doesn't support windowed partial easily)
    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  }
  epdInvalidatePrevFrame();   // panel no longer shows the main screen
}

// ---- AP / Captive Portal Screen ----
void display_ap_screen(uint8_t* ImageBW, const char* ssid, const char* ip) {
  char buffer[64];

  EPD_Init();
  EPD_Clear();
  EPD_Init_Fast(Fast_Seconds_1_5s);
  Paint_NewImage(ImageBW, EPD_W, EPD_H, PANEL_ROTATION, WHITE);
  EPD_Full(WHITE);

  // Generate WiFi QR code: WIFI:T:nopass;S:<SSID>;;
  char qrData[128];
  snprintf(qrData, sizeof(qrData), "WIFI:T:nopass;S:%s;;", ssid);

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrData);

  // Render QR code on the left portion
  // QR version 3 = 29x29 modules. Scale to fit nicely in ~140px
  int qrSize = qrcode.size;  // 29
  int scale = 4;             // 29*4 = 116px
  int qrPixels = qrSize * scale;
  int qrX = 20;             // left margin
  int qrY = (EPD_H - qrPixels) / 2;  // vertically centered

  // Draw QR with quiet zone (white border already from EPD_Full)
  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // Draw a scale x scale black square
        EPD_DrawRectangle(
          qrX + x * scale,
          qrY + y * scale,
          qrX + x * scale + scale,
          qrY + y * scale + scale,
          BLACK, 1);
      }
    }
  }

  // Text instructions on the right side of left half
  int textX = qrX + qrPixels + 20;  // after QR + margin
  int textW = LAYOUT_W - textX - 10;
  int textMidX = textX + textW / 2;
  int y = 30;

  // Title
  const char* title = "WiFi Setup";
  int titleW = EPD_GetUTF8TextWidth(title, 24);
  EPD_ShowStringUTF8(textMidX - titleW / 2, y, title, 24, BLACK);
  y += 40;

  // Step 1
  const char* step1 = "1. Scan QR or join:";
  EPD_ShowStringUTF8(textX, y, step1, 16, BLACK);
  y += 22;

  // SSID
  int ssidW = EPD_GetUTF8TextWidth(ssid, 24);
  EPD_ShowStringUTF8(textMidX - ssidW / 2, y, ssid, 24, BLACK);
  y += 38;

  // Step 2
  const char* step2 = "2. Open browser:";
  EPD_ShowStringUTF8(textX, y, step2, 16, BLACK);
  y += 22;

  // IP
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "http://%s", ip);
  int ipW = EPD_GetUTF8TextWidth(buffer, 16);
  EPD_ShowStringUTF8(textMidX - ipW / 2, y, buffer, 16, BLACK);
  y += 32;

  // Step 3
  const char* step3 = "3. Select WiFi";
  EPD_ShowStringUTF8(textX, y, step3, 16, BLACK);

  EPD_Display_Fast(ImageBW);
  epdInvalidatePrevFrame();   // panel no longer shows the main screen
}

void display_main_screen(uint8_t* ImageBW, bool& forceFullRefresh) {
  static char buffer[64];
  const int frameSize = EPD_BUF_SIZE;

  epdEnsurePrevFrame(frameSize);

  // Compose the frame in RAM only; no panel I/O until epdPushFrame() decides
  // between a full and a partial update based on what actually changed.
  Paint_NewImage(ImageBW, EPD_W, EPD_H, PANEL_ROTATION, WHITE);
  EPD_Full(WHITE);

  // ---- Layout ----
#ifdef PANEL_579
  int leftShift = 10;  // shift left-half content left to balance with separator
  int midX = LAYOUT_W / 2 - leftShift;
  int topHeight = EPD_H - 60;

  // Font size for temperatures (78px Logisoso - numbers only)
  int tempFontSize = 78;

  // Center points for left and right columns within the left half
  int leftCenter = LAYOUT_W / 4 - 10 - leftShift;
  int rightCenter = LAYOUT_W * 3 / 4 - leftShift;

  // Top-left: Bern Temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", temperature.c_str());
  int tempWidth = EPD_GetUTF8TextWidth(buffer, tempFontSize);
  EPD_ShowStringUTF8(leftCenter - tempWidth / 2, 20, buffer, tempFontSize, BLACK);
  
  // Draw degree symbol smaller next to temperature
  int degX = leftCenter + tempWidth / 2 + 2;
  EPD_ShowStringUTF8(degX, 20, "o", 16, BLACK);
  
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Local");
  int labelWidth = EPD_GetUTF8TextWidth(buffer, 16);
  EPD_ShowStringUTF8(leftCenter - labelWidth / 2, 102, buffer, 16, BLACK);

  // Top-right: Aare Temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", aareTemp.c_str());
  int aareWidth = EPD_GetUTF8TextWidth(buffer, tempFontSize);
  EPD_ShowStringUTF8(rightCenter - aareWidth / 2, 20, buffer, tempFontSize, BLACK);
  
  // Draw degree symbol smaller next to temperature
  degX = rightCenter + aareWidth / 2 + 2;
  EPD_ShowStringUTF8(degX, 20, "o", 16, BLACK);
  
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Aare");
  labelWidth = EPD_GetUTF8TextWidth(buffer, 16);
  EPD_ShowStringUTF8(rightCenter - labelWidth / 2, 102, buffer, 16, BLACK);

  // Middle: AareGuru text or staleness warning, centered in left half
  String staleMsg = getStalenessMessage();
  const char* middleText = nullptr;
  char aareTextBuf[64];
  
  if (staleMsg.length() > 0) {
    strncpy(aareTextBuf, staleMsg.c_str(), sizeof(aareTextBuf) - 1);
    aareTextBuf[sizeof(aareTextBuf) - 1] = '\0';
    middleText = aareTextBuf;
  } else if (aareText.length() > 0) {
    strncpy(aareTextBuf, aareText.c_str(), sizeof(aareTextBuf) - 1);
    aareTextBuf[sizeof(aareTextBuf) - 1] = '\0';
    middleText = aareTextBuf;
  }

  if (middleText != nullptr) {
    int maxWidth = LAYOUT_W - 20;
    while (strlen(aareTextBuf) > 0 && EPD_GetUTF8TextWidth(aareTextBuf, 16) > maxWidth) {
      aareTextBuf[strlen(aareTextBuf) - 1] = '\0';
    }
    int aareTextWidth = EPD_GetUTF8TextWidth(aareTextBuf, 16);
    int aareTextY = 155;
    EPD_ShowStringUTF8(midX - aareTextWidth / 2, aareTextY, aareTextBuf, 16, BLACK);
  }

  // Bottom-left: Rain status > Pollen (priority order)
  int bottomY = topHeight + 5;
  int bottomLeftCenter = leftCenter + 5; // nudge right to visually align with temperature above
  if (rainStatus.length() > 0) {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", rainStatus.c_str());
    int rainWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(bottomLeftCenter - rainWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "weather");
    int rainLabelWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(bottomLeftCenter - rainLabelWidth / 2, bottomY + 26, buffer, 12, BLACK);
  } else {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", pollenLevel.c_str());
    int pollenWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(bottomLeftCenter - pollenWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "pollen");
    int pollenLabelWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(bottomLeftCenter - pollenLabelWidth / 2, bottomY + 26, buffer, 12, BLACK);
  }

  // Bottom-right (of left half): UV index or Weight
  if (bottomRightMode == 0) {
    const char* uvLabel;
    if (uvIndexMax >= 11) uvLabel = "extreme";
    else if (uvIndexMax >= 8) uvLabel = "very high";
    else if (uvIndexMax >= 6) uvLabel = "high";
    else if (uvIndexMax >= 3) uvLabel = "moderate";
    else uvLabel = "low";

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%d %s", uvIndexMax, uvLabel);
    int uvWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(rightCenter - uvWidth / 2, bottomY, buffer, 24, BLACK);

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "uv index");
    int uvLabelWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(rightCenter - uvLabelWidth / 2, bottomY + 26, buffer, 12, BLACK);
  } else {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s kg", weight.c_str());
    int weightWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(rightCenter - weightWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", weightTrend.c_str());
    int trendWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(rightCenter - trendWidth / 2, bottomY + 26, buffer, 12, BLACK);
  }

#ifdef PANEL_579
  // ---- Vertical separator between left and right halves ----
  {
    int sepX = 380;           // right edge of left half, before the IC gap
    int sepPadY = 30;         // top and bottom padding
    EPD_DrawLine(sepX, sepPadY, sepX, EPD_H - sepPadY, BLACK);
  }

  // ---- Right half: Calendar events or useless fact ----
  {
    int rhX = 410;                      // start after IC gap (396 + 8 + margin)
    int rhMaxX = EPD_W_VISIBLE - 18;    // 774, right margin (10px padding from edge)
    int rhW = rhMaxX - rhX;             // ~374 usable width
    int rhY = 20;                       // top margin (aligned with temperature numbers)

    int headerFontSize = 32;            // 18pt Helvetica for day labels
    int eventFontSize = 24;             // 14pt Helvetica for event text
    int headerLineHeight = 34;          // vertical advance after day header
    int headerPadding = 6;              // extra gap between header and first event
    int eventLineHeight = 30;           // vertical advance after each event
    int daySectionGap = 14;             // spacing between day sections

    if (calendarTotalEvents > 0) {
      // Render calendar events
      for (int d = 0; d < 2; d++) {
        if (calendarDays[d].eventCount == 0) continue;

        // Day header ("Today" / "Tomorrow")
        if (calendarDays[d].label.length() > 0) {
          memset(buffer, 0, sizeof(buffer));
          snprintf(buffer, sizeof(buffer), "%s", calendarDays[d].label.c_str());
          EPD_ShowStringUTF8(rhX, rhY, buffer, headerFontSize, BLACK);
          rhY += headerLineHeight + headerPadding;
        }

        // Events (capped: the right half has room for ~3 per day)
        int shown = calendarDays[d].eventCount;
        if (shown > 3) shown = 3;
        for (int i = 0; i < shown; i++) {
          CalendarEvent& ev = calendarDays[d].events[i];

          if (ev.allDay) {
            // All-day: render summary with indent matching timed events
            memset(buffer, 0, sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "All day  %s", ev.summary.c_str());
          } else {
            // Timed: "HH:MM  Summary"
            memset(buffer, 0, sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "%s  %s", ev.startTime.c_str(), ev.summary.c_str());
          }

          // Truncate if too wide
          while (strlen(buffer) > 0 && EPD_GetUTF8TextWidth(buffer, eventFontSize) > rhW) {
            buffer[strlen(buffer) - 1] = '\0';
          }

          EPD_ShowStringUTF8(rhX, rhY, buffer, eventFontSize, BLACK);
          rhY += eventLineHeight;
        }

        rhY += daySectionGap;  // spacing between days
      }
    }

    // Show useless fact if there's remaining vertical space
    // (either no events at all, or after a short event list)
    // Bottom-aligned: word-wrap into lines first, then render from bottom up
    {
      // Smaller font when used as filler below calendar events
      bool isFiller = (calendarTotalEvents > 0);
      int lineHeight = isFiller ? 22 : 26;
      int fontSize = isFiller ? 18 : 20;
      int bottomMargin = 25;
      int maxLines = 8;
      int labelFontSize = 12;
      int labelGap = 8;
      int labelHeight = labelFontSize + labelGap;

      // Need room for: label + gap + at least one line of fact text + bottom margin
      int minRequired = labelHeight + lineHeight + bottomMargin;

      if (uselessFact.length() > 0 && rhY + minRequired < EPD_H) {

        // First pass: word-wrap into line buffer
        char factBuf[512];
        strncpy(factBuf, uselessFact.c_str(), sizeof(factBuf) - 1);
        factBuf[sizeof(factBuf) - 1] = '\0';

        char lines[8][256];
        int lineCount = 0;

        char* remaining = factBuf;
        while (*remaining && lineCount < maxLines) {
          int len = strlen(remaining);
          if (len > (int)sizeof(lines[0]) - 1) len = sizeof(lines[0]) - 1;
          strncpy(lines[lineCount], remaining, len);
          lines[lineCount][len] = '\0';

          // Shrink until it fits the width
          while (strlen(lines[lineCount]) > 0 && EPD_GetUTF8TextWidth(lines[lineCount], fontSize) > rhW) {
            lines[lineCount][strlen(lines[lineCount]) - 1] = '\0';
          }

          int lineLen = strlen(lines[lineCount]);
          if (lineLen == 0) break;

          // If we truncated and the next char isn't a space/end, back up to last space
          if (lineLen < (int)strlen(remaining) && remaining[lineLen] != ' ') {
            int lastSpace = -1;
            for (int j = lineLen - 1; j >= 0; j--) {
              if (lines[lineCount][j] == ' ') { lastSpace = j; break; }
            }
            if (lastSpace > 0) {
              lines[lineCount][lastSpace] = '\0';
              lineLen = lastSpace;
            }
          }

          lineCount++;
          remaining += lineLen;
          while (*remaining == ' ') remaining++;
        }

        // Second pass: position the fact text (including label above)
        int factStartY;
        if (isFiller) {
          // Bottom-aligned when used as filler below calendar events
          factStartY = EPD_H - bottomMargin - (lineCount * lineHeight) - labelHeight - 5;
          if (factStartY < rhY + 10) factStartY = rhY + 10;
        } else {
          // Vertically centered on the right half when no events exist
          int totalFactHeight = labelHeight + (lineCount * lineHeight);
          factStartY = (EPD_H - totalFactHeight) / 2;
        }

        // Draw label
        EPD_ShowStringUTF8(rhX, factStartY, "Fun fact of the day", labelFontSize, BLACK);
        factStartY += labelHeight;

        for (int i = 0; i < lineCount; i++) {
          int drawY = factStartY + (i * lineHeight);
          if (drawY + lineHeight > EPD_H - bottomMargin) break;  // safety
          EPD_ShowStringUTF8(rhX, drawY, lines[i], fontSize, BLACK);
        }
      }
    }
  }
#endif  // right half (5.79")
#else
  // ---- 4.2": dedicated forecast + calendar layout ----
  (void)buffer;
  draw_layout_42();
#endif

  epdPushFrame(ImageBW, frameSize, forceFullRefresh);
}

#endif
