#ifndef WEATHER_SCREEN_H
#define WEATHER_SCREEN_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <time.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "config_manager.h"

extern ConfigManager config;

String temperature;
String aareTemp;
String aareText;
String pollenLevel;
String rainStatus;  // empty = no rain, otherwise "Rain in 3h/2h/1h", "Rain imminent", or "Raining"
int uvIndexMax = 0; // Daily max UV index from Open-Meteo
String weight = "--.-";
String weightTrend = "n/a";

String stravaActivity = "";      // e.g. "Run 5.2km" or "Ride 32km"
String stravaActivityDate = "";  // e.g. "2h ago" or "3d ago"

// Bottom-right display mode: 0 = strava (default for now), 1 = weight
int bottomRightMode = 1;

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
    Serial.println("Token expired, refreshing...");
    fetch_withings_token(httpResponseCode);
    if (httpResponseCode == 200) {
      Serial.println("Token refreshed, retrying weight fetch...");
      http.end();
      fetch_weight_data(httpResponseCode, false);
      return;
    }
  } else {
    Serial.print("Withings API error: ");
    Serial.println(httpResponseCode);
    weight = "--.-";
    weightTrend = "err";
  }

  http.end();
}

// --- Strava ---

void fetch_strava_token(int& httpResponseCode) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  
  http.begin(client, "https://www.strava.com/oauth/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "client_id=" + config.stravaClientId + 
                    "&client_secret=" + config.stravaClientSecret +
                    "&grant_type=refresh_token" +
                    "&refresh_token=" + config.stravaRefreshToken;
  
  httpResponseCode = http.POST(postData);
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("Strava token response:");
    Serial.println(payload);
    
    JSONVar tokenObject = JSON.parse(payload);
    if (JSON.typeof(tokenObject) != "undefined") {
      if (JSON.typeof(tokenObject["access_token"]) != "undefined") {
        String token = JSON.stringify(tokenObject["access_token"]);
        token.replace("\"", "");
        config.stravaAccessToken = token;
      }
      if (JSON.typeof(tokenObject["refresh_token"]) != "undefined") {
        String refresh = JSON.stringify(tokenObject["refresh_token"]);
        refresh.replace("\"", "");
        config.stravaRefreshToken = refresh;
      }
      config.saveStrava();
      Serial.println("Strava token updated successfully");
    }
  } else {
    Serial.print("Failed to refresh Strava token. HTTP code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

String getActivityShortType(const String& type) {
  if (type == "Run") return "Run";
  if (type == "Ride") return "Ride";
  if (type == "Swim") return "Swim";
  if (type == "Walk") return "Walk";
  if (type == "Hike") return "Hike";
  if (type == "MountainBikeRide") return "MTB";
  if (type == "GravelRide") return "Gravel";
  if (type == "TrailRun") return "Trail";
  if (type == "VirtualRide") return "Zwift";
  if (type == "VirtualRun") return "VRun";
  if (type == "WeightTraining") return "Gym";
  if (type == "Yoga") return "Yoga";
  return type.substring(0, 6);
}

void fetch_strava_data(int& httpResponseCode, bool retry = true) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  
  http.begin(client, "https://www.strava.com/api/v3/athlete/activities?per_page=1&page=1");
  http.addHeader("Authorization", "Bearer " + config.stravaAccessToken);
  
  httpResponseCode = http.GET();
  Serial.print("Strava HTTP code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode == 200) {
    String stravaBuffer = http.getString();
    Serial.println(stravaBuffer);
    JSONVar activities = JSON.parse(stravaBuffer);
    
    if (JSON.typeof(activities) != "undefined" && activities.length() > 0) {
      String type = JSON.stringify(activities[0]["type"]);
      type.replace("\"", "");
      String shortType = getActivityShortType(type);
      
      double distance = (double)activities[0]["distance"];
      double distKm = distance / 1000.0;
      
      if (distKm >= 1.0) {
        stravaActivity = shortType + " " + String(distKm, 1) + "km";
      } else {
        stravaActivity = shortType + " " + String((int)distance) + "m";
      }
      
      // Show date of activity
      String startDate = JSON.stringify(activities[0]["start_date_local"]);
      startDate.replace("\"", "");
      // ISO 8601: "2026-05-12T07:30:00Z" -> "12 May"
      if (startDate.length() >= 10) {
        int month = startDate.substring(5, 7).toInt();
        int day = startDate.substring(8, 10).toInt();
        const char* months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        if (month >= 1 && month <= 12) {
          stravaActivityDate = String(day) + " " + months[month];
        } else {
          stravaActivityDate = startDate.substring(0, 10);
        }
      }
      
      Serial.print("Strava activity: ");
      Serial.print(stravaActivity);
      Serial.print(" (");
      Serial.print(stravaActivityDate);
      Serial.println(")");
    } else {
      stravaActivity = "No activity";
      stravaActivityDate = "";
    }
  } else if (httpResponseCode == 401 && retry) {
    Serial.println("Strava token expired, refreshing...");
    http.end();
    fetch_strava_token(httpResponseCode);
    if (httpResponseCode == 200) {
      fetch_strava_data(httpResponseCode, false);
      return;
    } else {
      stravaActivity = "auth err";
      stravaActivityDate = "";
    }
  } else {
    Serial.print("Strava API error: ");
    Serial.println(httpResponseCode);
    stravaActivity = "err";
    stravaActivityDate = "";
  }
  
  http.end();
}

void fetch_weather_data(int& httpResponseCode) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    return;
  }

  bool currentlyRaining = false;

  // Open-Meteo: weather + hourly forecast + daily UV in one call
  // Herrenschwanden coordinates: 46.9725, 7.4528
  String serverPath = "http://api.open-meteo.com/v1/forecast?latitude=46.9725&longitude=7.4528"
    "&current=temperature_2m,weather_code"
    "&hourly=weather_code"
    "&daily=uv_index_max"
    "&forecast_hours=8"
    "&timezone=Europe/Zurich";
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverPath);
  httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    jsonBuffer = http.getString();
    Serial.println(jsonBuffer);
    myObject = JSON.parse(jsonBuffer);

    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      http.end();
      return;
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
      time_t now = time(NULL);

      // Parse the first hourly timestamp to calculate offsets
      String firstTimeStr = (const char*)myObject["hourly"]["time"][0];

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

    Serial.print("Temperature: ");
    Serial.println(temperature);
  } else {
    Serial.print("Weather API error: ");
    Serial.println(httpResponseCode);
    http.end();
    return;
  }

  http.end();

  // Google Pollen API (specifically GRASS)
  if (config.googlePollenApiKey.length() > 0) {
    String pollenServerPath = "https://pollen.googleapis.com/v1/forecast:lookup?key=" + config.googlePollenApiKey + "&location.latitude=46.9725&location.longitude=7.4528&days=1";
    
    // Google API requires HTTPS. Using setInsecure to bypass cert checks for simplicity constraint.
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    
    HTTPClient httpPollen;
    httpPollen.begin(secureClient, pollenServerPath);
    httpResponseCode = httpPollen.GET();

    if (httpResponseCode == 200) {
      pollenJsonBuffer = httpPollen.getString();
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
    } else {
      Serial.print("Pollen API error: ");
      Serial.println(httpResponseCode);
      pollenLevel = "n/a";
    }
    httpPollen.end();
  } else {
    // Open-Meteo Fallback
    String pollenServerPath = "http://air-quality-api.open-meteo.com/v1/air-quality?latitude=46.9725&longitude=7.4528&current=pm2_5";
    HTTPClient httpPollen;
    httpPollen.begin(client, pollenServerPath);
    httpResponseCode = httpPollen.GET();

    if (httpResponseCode == 200) {
      pollenJsonBuffer = httpPollen.getString();
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
    }
    httpPollen.end();
  }

  String aareServerPath = "http://aareguru.existenz.ch/v2018/today?city=bern&app=li.richert.smartframe&version=0.0.1";
  HTTPClient httpAare;
  httpAare.begin(client, aareServerPath);
  httpResponseCode = httpAare.GET();

  if (httpResponseCode == 200) {
    aareJsonBuffer = httpAare.getString();
    Serial.println(aareJsonBuffer);
    aareObject = JSON.parse(aareJsonBuffer);

    if (JSON.typeof(aareObject) == "undefined") {
      Serial.println("Parsing aare data failed!");
      httpAare.end();
      return;
    }

    aareTemp = String((int)round((double)aareObject["aare"]));
    aareText = JSON.stringify(aareObject["text"]);
    aareText.replace("\"", "");

    Serial.print("String aare temp: ");
    Serial.println(aareTemp);
    Serial.print("Aare text: ");
    Serial.println(aareText);
  } else {
    Serial.print("Aare API error: ");
    Serial.println(httpResponseCode);
  }

  httpAare.end();

  fetch_weight_data(httpResponseCode);
}





// ---- Status Screen ----
// otaStateText: 0=idle, 1=checking, 2=update available, 3=no update
void display_status_screen(uint8_t* ImageBW, int otaState, const char* otaVersion) {
  char buffer[64];

  EPD_Init_Fast(Fast_Seconds_1_5s);
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  int midX = EPD_W / 2;
  int y = 20;

  // Title
  const char* title = "Device Status";
  int titleW = EPD_GetUTF8TextWidth(title, 24);
  EPD_ShowStringUTF8(midX - titleW / 2, y, title, 24, BLACK);
  y += 40;

  // Horizontal line
  EPD_DrawLine(20, y, EPD_W - 20, y, BLACK);
  y += 15;

  // WiFi SSID
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "WiFi: %s", WiFi.SSID().c_str());
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 25;

  // IP
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "IP: %s", WiFi.localIP().toString().c_str());
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 25;

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
  y += 25;

  // Firmware version
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Firmware: %s", FIRMWARE_VERSION);
  EPD_ShowStringUTF8(20, y, buffer, 16, BLACK);
  y += 25;

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
  y += 35;

  // Horizontal line
  EPD_DrawLine(20, y, EPD_W - 20, y, BLACK);
  y += 15;

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
    y += 22;
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
  EPD_ShowStringUTF8(midX - footW / 2, EPD_H - 20, footer, 12, BLACK);

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

// ---- OTA Update Progress Screen ----
void display_ota_screen(uint8_t* ImageBW, const char* version, int percent) {
  char buffer[64];

  EPD_Init_Fast(Fast_Seconds_1_5s);
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  int midX = EPD_W / 2;

  // Title
  const char* title = "Updating Firmware";
  int titleW = EPD_GetUTF8TextWidth(title, 24);
  EPD_ShowStringUTF8(midX - titleW / 2, 60, title, 24, BLACK);

  // Version line
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s -> %s", FIRMWARE_VERSION, version);
  int verW = EPD_GetUTF8TextWidth(buffer, 16);
  EPD_ShowStringUTF8(midX - verW / 2, 100, buffer, 16, BLACK);

  // Progress bar outline (280x30, centered)
  int barX = midX - 140;
  int barY = 150;
  int barW = 280;
  int barH = 30;
  EPD_DrawRectangle(barX, barY, barX + barW, barY + barH, BLACK, 0);  // outline

  // Fill progress
  if (percent > 0) {
    int fillW = (barW - 4) * percent / 100;
    EPD_DrawRectangle(barX + 2, barY + 2, barX + 2 + fillW, barY + barH - 2, BLACK, 1);  // filled
  }

  // Percentage text
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%d%%", percent);
  int pctW = EPD_GetUTF8TextWidth(buffer, 24);
  EPD_ShowStringUTF8(midX - pctW / 2, 200, buffer, 24, BLACK);

  // Warning
  const char* warn = "Do not power off!";
  int warnW = EPD_GetUTF8TextWidth(warn, 12);
  EPD_ShowStringUTF8(midX - warnW / 2, 250, warn, 12, BLACK);

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

// ---- AP / Captive Portal Screen ----
void display_ap_screen(uint8_t* ImageBW, const char* ssid, const char* ip) {
  char buffer[64];

  EPD_Init();
  EPD_Clear();
  EPD_Init_Fast(Fast_Seconds_1_5s);
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  int midX = EPD_W / 2;

  // Title
  const char* title = "WiFi Setup";
  int titleW = EPD_GetUTF8TextWidth(title, 24);
  EPD_ShowStringUTF8(midX - titleW / 2, 40, title, 24, BLACK);

  // Step 1
  const char* step1 = "1. Connect to WiFi:";
  int s1W = EPD_GetUTF8TextWidth(step1, 16);
  EPD_ShowStringUTF8(midX - s1W / 2, 90, step1, 16, BLACK);

  // SSID (large)
  int ssidW = EPD_GetUTF8TextWidth(ssid, 24);
  EPD_ShowStringUTF8(midX - ssidW / 2, 115, ssid, 24, BLACK);

  // Step 2
  const char* step2 = "2. Open browser:";
  int s2W = EPD_GetUTF8TextWidth(step2, 16);
  EPD_ShowStringUTF8(midX - s2W / 2, 160, step2, 16, BLACK);

  // IP (large)
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "http://%s", ip);
  int ipW = EPD_GetUTF8TextWidth(buffer, 24);
  EPD_ShowStringUTF8(midX - ipW / 2, 185, buffer, 24, BLACK);

  // Step 3
  const char* step3 = "3. Select your WiFi network";
  int s3W = EPD_GetUTF8TextWidth(step3, 16);
  EPD_ShowStringUTF8(midX - s3W / 2, 230, step3, 16, BLACK);

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

void display_main_screen(uint8_t* ImageBW, bool& forceFullRefresh) {
  static char buffer[64];

  if (forceFullRefresh) {
    EPD_Init();
    EPD_Clear();
    forceFullRefresh = false;
  }

  EPD_Init_Fast(Fast_Seconds_1_5s);
  
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  int midX = EPD_W / 2;
  int topHeight = EPD_H - 70;  // Give temps 70%+ of screen

  // Font size for temperatures (78px Logisoso - numbers only)
  int tempFontSize = 78;

  // Center points for left and right columns
  int leftCenter = midX / 2 - 10;
  int rightCenter = midX + midX / 2 - 10;

  // Top-left: Bern Temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", temperature.c_str());
  int tempWidth = EPD_GetUTF8TextWidth(buffer, tempFontSize);
  EPD_ShowStringUTF8(leftCenter - tempWidth / 2, 30, buffer, tempFontSize, BLACK);
  
  // Draw degree symbol smaller next to temperature
  int degX = leftCenter + tempWidth / 2 + 2;
  EPD_ShowStringUTF8(degX, 30, "o", 16, BLACK);
  
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Local");
  int labelWidth = EPD_GetUTF8TextWidth(buffer, 16);
  EPD_ShowStringUTF8(leftCenter - labelWidth / 2, 115, buffer, 16, BLACK);

  // Top-right: Aare Temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", aareTemp.c_str());
  int aareWidth = EPD_GetUTF8TextWidth(buffer, tempFontSize);
  EPD_ShowStringUTF8(rightCenter - aareWidth / 2, 30, buffer, tempFontSize, BLACK);
  
  // Draw degree symbol smaller next to temperature
  degX = rightCenter + aareWidth / 2 + 2;
  EPD_ShowStringUTF8(degX, 30, "o", 16, BLACK);
  
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Aare");
  labelWidth = EPD_GetUTF8TextWidth(buffer, 16);
  EPD_ShowStringUTF8(rightCenter - labelWidth / 2, 115, buffer, 16, BLACK);

  // Middle: AareGuru text, centered
  if (aareText.length() > 0) {
    // Truncate to fit display width with margin
    int maxWidth = EPD_W - 40;
    char aareTextBuf[64];
    strncpy(aareTextBuf, aareText.c_str(), sizeof(aareTextBuf) - 1);
    aareTextBuf[sizeof(aareTextBuf) - 1] = '\0';
    // Trim until it fits
    while (strlen(aareTextBuf) > 0 && EPD_GetUTF8TextWidth(aareTextBuf, 16) > maxWidth) {
      aareTextBuf[strlen(aareTextBuf) - 1] = '\0';
    }
    int aareTextWidth = EPD_GetUTF8TextWidth(aareTextBuf, 16);
    EPD_ShowStringUTF8(midX - aareTextWidth / 2, 170, aareTextBuf, 16, BLACK);
  }

  // Bottom-left: Rain status > UV warning (>=6) > Pollen (priority order)
  int bottomY = topHeight + 10;
  if (rainStatus.length() > 0) {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", rainStatus.c_str());
    int rainWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(leftCenter - rainWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Weather");
    int rainLabelWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(leftCenter - rainLabelWidth / 2, bottomY + 28, buffer, 12, BLACK);
  } else if (uvIndexMax >= 8) {
    // UV warning: 8-10 very high, 11+ extreme
    const char* uvLabel;
    if (uvIndexMax >= 11) uvLabel = "extreme";
    else uvLabel = "very high";

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", uvLabel);
    int uvWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(leftCenter - uvWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "uv index");
    int uvLabelWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(leftCenter - uvLabelWidth / 2, bottomY + 28, buffer, 12, BLACK);
  } else {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", pollenLevel.c_str());
    int pollenWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(leftCenter - pollenWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "pollen");
    int pollenLabelWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(leftCenter - pollenLabelWidth / 2, bottomY + 28, buffer, 12, BLACK);
  }

  // Bottom-right: Strava or Weight (toggled by rocker switch)
  if (bottomRightMode == 0 && stravaActivity.length() > 0) {
    // Strava mode
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", stravaActivity.c_str());
    int stravaWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(rightCenter - stravaWidth / 2, bottomY, buffer, 24, BLACK);
    
    if (stravaActivityDate.length() > 0) {
      memset(buffer, 0, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), "%s", stravaActivityDate.c_str());
      int dateWidth = EPD_GetUTF8TextWidth(buffer, 12);
      EPD_ShowStringUTF8(rightCenter - dateWidth / 2, bottomY + 28, buffer, 12, BLACK);
    }
  } else {
    // Weight mode
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s kg", weight.c_str());
    int weightWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(rightCenter - weightWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", weightTrend.c_str());
    int trendWidth = EPD_GetUTF8TextWidth(buffer, 12);
    EPD_ShowStringUTF8(rightCenter - trendWidth / 2, bottomY + 28, buffer, 12, BLACK);
  }

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

#endif
