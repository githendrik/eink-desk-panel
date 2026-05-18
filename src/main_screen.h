#ifndef WEATHER_SCREEN_H
#define WEATHER_SCREEN_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <time.h>
#include <Preferences.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "credentials.h"

Preferences prefs;

String temperature;
String aareTemp;
String aareText;
String pollenLevel;
String rainStatus;  // empty = no rain, otherwise "Rain in 3h/2h/1h", "Rain imminent", or "Raining"
String weight = "--.-";
String weightTrend = "n/a";

String stravaActivity = "";      // e.g. "Run 5.2km" or "Ride 32km"
String stravaActivityDate = "";  // e.g. "2h ago" or "3d ago"

// Bottom-right display mode: 0 = strava (default for now), 1 = weight
int bottomRightMode = 1;

String openWeatherMapApiKey = OPENWEATHER_API_KEY;
String cityId = "2661552";

String withingsClientId = WITHINGS_CLIENT_ID;
String withingsClientSecret = WITHINGS_CLIENT_SECRET;
String withingsAccessToken = WITHINGS_ACCESS_TOKEN;
String withingsRefreshToken = WITHINGS_REFRESH_TOKEN;
String withingsUserId = WITHINGS_USER_ID;

String stravaClientId = STRAVA_CLIENT_ID;
String stravaClientSecret = STRAVA_CLIENT_SECRET;
String stravaAccessToken = STRAVA_ACCESS_TOKEN;
String stravaRefreshToken = STRAVA_REFRESH_TOKEN;

void loadWithingsTokens() {
  prefs.begin("withings", true);  // read-only
  String storedAccess = prefs.getString("access_token", "");
  String storedRefresh = prefs.getString("refresh_token", "");
  prefs.end();
  
  if (storedAccess.length() > 0) {
    withingsAccessToken = storedAccess;
    Serial.println("Loaded access token from NVS");
  }
  if (storedRefresh.length() > 0) {
    withingsRefreshToken = storedRefresh;
    Serial.println("Loaded refresh token from NVS");
  }
}

void saveWithingsTokens() {
  prefs.begin("withings", false);  // read-write
  prefs.putString("access_token", withingsAccessToken);
  prefs.putString("refresh_token", withingsRefreshToken);
  prefs.end();
  Serial.println("Saved tokens to NVS");
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
  
  String postData = "action=requesttoken&grant_type=refresh_token&refresh_token=" + withingsRefreshToken + 
                    "&client_id=" + withingsClientId + 
                    "&client_secret=" + withingsClientSecret;
  
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
        withingsAccessToken = token;
        if (newRefresh.length() > 0) {
          newRefresh.replace("\"", "");
          withingsRefreshToken = newRefresh;
        }
        saveWithingsTokens();
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

  if (withingsAccessToken.length() == 0) {
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
  http.addHeader("Authorization", "Bearer " + withingsAccessToken);
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

void loadStravaTokens() {
  prefs.begin("strava", true);
  String storedAccess = prefs.getString("access_token", "");
  String storedRefresh = prefs.getString("refresh_token", "");
  prefs.end();
  
  if (storedAccess.length() > 0) {
    stravaAccessToken = storedAccess;
    Serial.println("Loaded Strava access token from NVS");
  }
  if (storedRefresh.length() > 0) {
    stravaRefreshToken = storedRefresh;
    Serial.println("Loaded Strava refresh token from NVS");
  }
}

void saveStravaTokens() {
  prefs.begin("strava", false);
  prefs.putString("access_token", stravaAccessToken);
  prefs.putString("refresh_token", stravaRefreshToken);
  prefs.end();
  Serial.println("Saved Strava tokens to NVS");
}

void fetch_strava_token(int& httpResponseCode) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  
  http.begin(client, "https://www.strava.com/oauth/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "client_id=" + stravaClientId + 
                    "&client_secret=" + stravaClientSecret +
                    "&grant_type=refresh_token" +
                    "&refresh_token=" + stravaRefreshToken;
  
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
        stravaAccessToken = token;
      }
      if (JSON.typeof(tokenObject["refresh_token"]) != "undefined") {
        String refresh = JSON.stringify(tokenObject["refresh_token"]);
        refresh.replace("\"", "");
        stravaRefreshToken = refresh;
      }
      saveStravaTokens();
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
  http.addHeader("Authorization", "Bearer " + stravaAccessToken);
  
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
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?id=" + cityId + "&APPID=" + openWeatherMapApiKey + "&units=metric";
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

    temperature = String((int)round((double)myObject["main"]["temp"]));

    // Check if currently raining (weather IDs 2xx=thunderstorm, 3xx=drizzle, 5xx=rain)
    int weatherId = (int)round((double)myObject["weather"][0]["id"]);
    currentlyRaining = (weatherId >= 200 && weatherId < 600);
    if (currentlyRaining) {
      rainStatus = "Raining";
      Serial.println("Currently raining");
    } else {
      rainStatus = "";
    }

    Serial.print("String Temperature: ");
    Serial.println(temperature);
  } else {
    Serial.print("Weather API error: ");
    Serial.println(httpResponseCode);
    http.end();
    return;
  }

  http.end();

  // Check forecast for rain ending (if currently raining) or upcoming rain
  String forecastPath = "http://api.openweathermap.org/data/2.5/forecast?id=" + cityId + "&APPID=" + openWeatherMapApiKey + "&units=metric&cnt=8";
  HTTPClient httpForecast;
  httpForecast.begin(client, forecastPath);
  httpResponseCode = httpForecast.GET();

  if (httpResponseCode == 200) {
    String forecastBuffer = httpForecast.getString();
    JSONVar forecastObject = JSON.parse(forecastBuffer);

    if (JSON.typeof(forecastObject) != "undefined" && JSON.typeof(forecastObject["list"]) != "undefined") {
      int listCount = forecastObject["list"].length();
      time_t now = time(NULL);

      for (int i = 0; i < listCount; i++) {
        int fId = (int)round((double)forecastObject["list"][i]["weather"][0]["id"]);
        bool isRain = fId >= 200 && fId < 600;

        if (currentlyRaining) {
          if (!isRain) {
            time_t forecastDt = (time_t)(double)forecastObject["list"][i]["dt"];
            int hoursAway = (forecastDt - now) / 3600;
            if (hoursAway <= 3) {
              if (hoursAway >= 2) {
                rainStatus = "Rain ends in 3h";
              } else if (hoursAway >= 1) {
                rainStatus = "Rain ends in 2h";
              } else {
                rainStatus = "Rain end soon";
              }
              Serial.print("Rain ending: ");
              Serial.println(rainStatus);
            }
            break;
          }
        } else {
          if (isRain) {
            time_t forecastDt = (time_t)(double)forecastObject["list"][i]["dt"];
            int hoursAway = (forecastDt - now) / 3600;
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
  }
  httpForecast.end();

  String pollenServerPath = "http://api.openweathermap.org/data/2.5/air_pollution?lat=46.9480&lon=7.4474&appid=" + openWeatherMapApiKey;
  HTTPClient httpPollen;
  httpPollen.begin(client, pollenServerPath);
  httpResponseCode = httpPollen.GET();

  if (httpResponseCode == 200) {
    pollenJsonBuffer = httpPollen.getString();
    pollenObject = JSON.parse(pollenJsonBuffer);

    if (JSON.typeof(pollenObject) == "undefined") {
      httpPollen.end();
      return;
    }

    double pm25Value = (double)pollenObject["list"][0]["components"]["pm2_5"];
    int pm25 = (int)pm25Value;
    
    if (pm25 < 20) pollenLevel = "low";
    else if (pm25 < 40) pollenLevel = "moderate";
    else if (pm25 < 60) pollenLevel = "high";
    else pollenLevel = "very high";

    Serial.print("Pollen level: ");
    Serial.println(pollenLevel);
  } else {
    Serial.print("Pollen API error: ");
    Serial.println(httpResponseCode);
    pollenLevel = "n/a";
  }

  httpPollen.end();

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
  snprintf(buffer, sizeof(buffer), "Bern");
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

  // Bottom-left: Rain status (if raining/forecast) or Pollen
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
  } else {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", pollenLevel.c_str());
    int pollenWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(leftCenter - pollenWidth / 2, bottomY, buffer, 24, BLACK);
    
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Pollen");
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
