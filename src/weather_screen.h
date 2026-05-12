#ifndef WEATHER_SCREEN_H
#define WEATHER_SCREEN_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <TimeLib.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "credentials.h"

// Weather and Aare data variables
String weather;
String temperature;
String temperatureMin;
String temperatureMax;
String city_js;
String aareTemp;
String aareText;
String aareTime;

// OpenWeatherMap API key
String openWeatherMapApiKey = OPENWEATHER_API_KEY;

// City name and code to query
String city = "Bern";
String countryCode = "2661552";

// JSON buffers
String jsonBuffer;
JSONVar myObject;
String aareJsonBuffer;
JSONVar aareObject;

// Helper function to format timestamp
String getDate(String ts) {
  time_t t = (time_t) ts.toInt();
  char buf[25];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", year(t), month(t), day(t), hour(t), minute(t), second(t));
  return String(buf);
}

// Fetch weather and aare data
void fetch_weather_data(int& httpResponseCode, String (*httpGETRequest)(const char*))
{
  // Check if WiFi is connected successfully
  if (WiFi.status() == WL_CONNECTED) {
    // Build the OpenWeatherMap API request URL
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";

    // Try to get a successful HTTP response (non-blocking - single attempt)
    // Send an HTTP GET request and get JSON data
    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer); // Print the retrieved JSON data
    myObject = JSON.parse(jsonBuffer); // Parse the JSON data

    // Check if the JSON data was parsed successfully
    if (JSON.typeof(myObject) == "undefined" || httpResponseCode != 200) {
      Serial.println("Parsing input failed or bad HTTP response!"); // If parsing fails, print error message and exit
      return;
    }

    // Extract weather information from the JSON data
    String w = myObject["weather"][0]["main"];
    weather = w;
    temperature = String((int)round((double)myObject["main"]["temp"])); // Get the temperature rounded to nearest int
    temperatureMin = String((int)round((double)myObject["main"]["temp_min"])); // Get the min temperature rounded to nearest int
    temperatureMax = String((int)round((double)myObject["main"]["temp_max"])); // Get the max temperature rounded to nearest int

    String city_name = myObject["name"];
    city_js = city_name;

    // Print the extracted weather information
    Serial.print("String weather: ");
    Serial.println(weather);
    Serial.print("String Temperature: ");
    Serial.println(temperature);
    Serial.print("String city_js: ");
    Serial.println(city_js);

    httpResponseCode = 0;
    String aareServerPath = "http://aareguru.existenz.ch/v2018/today?city=bern&app=li.richert.smartframe&version=0.0.1";

    // Try to get aare data (non-blocking - single attempt)
    // Send an HTTP GET request and get JSON data
    aareJsonBuffer = httpGETRequest(aareServerPath.c_str());
    Serial.println(aareJsonBuffer); // Print the retrieved JSON data
    aareObject = JSON.parse(aareJsonBuffer); // Parse the JSON data

    // Check if the JSON data was parsed successfully
    if (JSON.typeof(aareObject) == "undefined" || httpResponseCode != 200) {
      Serial.println("Parsing aare data failed or bad HTTP response!"); // If parsing fails, print error message and exit
      return;
    }

    // Extract aare information from the JSON data
    aareTemp = String((int)round((double)aareObject["aare"])); // Get the aare temperature rounded to nearest int
    String aareTimeTs = JSON.stringify(aareObject["time"]);
    aareTime = getDate(aareTimeTs);

    String text = aareObject["text"];
    aareText = text;

    Serial.print("String aare temp: ");
    Serial.println(aareTemp);
    Serial.print("String aare text: ");
    Serial.println(aareText);
    Serial.print("String aare time: ");
    Serial.println(aareTime);
  }
  else {
    Serial.println("WiFi Disconnected"); // If WiFi is not connected, print error message
  }
}

// Helper function to get weather icon index
int getWeatherIcon(String weatherCondition) {
  weatherCondition.toLowerCase();
  if (weatherCondition.indexOf("mist") != -1 || weatherCondition.indexOf("fog") != -1) {
    return 0; // Mist
  } else if (weatherCondition.indexOf("cloud") != -1) {
    return 1; // Cloudy
  } else if (weatherCondition.indexOf("thunder") != -1) {
    return 2; // Thunderstorm
  } else if (weatherCondition.indexOf("clear") != -1) {
    return 3; // Clear sky
  } else if (weatherCondition.indexOf("snow") != -1) {
    return 4; // Snow
  } else if (weatherCondition.indexOf("rain") != -1 || weatherCondition.indexOf("drizzle") != -1) {
    return 5; // Rain
  }
  return 3; // Default to clear
}

// Display weather forecast information (Screen 0)
void display_weather_screen(uint8_t* ImageBW, bool& forceFullRefresh)
{
  // Create character arrays to store information
  static char buffer[64];

  // Do a full refresh if switching screens
  if (forceFullRefresh) {
    EPD_Init();
    EPD_Clear();
    // Removed blocking delay - EPD_Clear() already waits for completion
    forceFullRefresh = false;
  }

  // Initialize fast refresh mode
  EPD_Init_Fast(Fast_Seconds_1_5s);
  
  // Create a white canvas and clear the display
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE); // Fill display with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Clear to white

  // Get weather icon index
  int weatherIcon = getWeatherIcon(weather);
  
  // Display weather icon on the left (184x208 pixels)
  EPD_ShowPicture(20, 20, 184, 208, Weather_Num[weatherIcon], WHITE);

  // Display temperature data on the right - minimalist style
  // Main temperature (large)
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s°", temperature.c_str());
  EPD_ShowStringUTF8(230, 60, buffer, 48, BLACK);

  // Min/Max temperatures (smaller, below main temp)
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s° / %s°", temperatureMin.c_str(), temperatureMax.c_str());
  EPD_ShowStringUTF8(230, 120, buffer, 16, BLACK);

  // Weather condition text
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", weather.c_str());
  EPD_ShowStringUTF8(230, 150, buffer, 16, BLACK);

  // Aare section (bottom of screen)
  // Draw a subtle divider line
  EPD_DrawLine(20, 240, 380, 240, BLACK);

  // Aare title and temp (left side)
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Aare  %s°", aareTemp.c_str());
  EPD_ShowStringUTF8(30, 255, buffer, 16, BLACK);
  
  // Aare condition text (right-aligned, truncate if too long)
  memset(buffer, 0, sizeof(buffer));
  String truncatedText = aareText;
  if (truncatedText.length() > 27) {
    truncatedText = truncatedText.substring(0, 27) + "...";
  }
  snprintf(buffer, sizeof(buffer), "%s", truncatedText.c_str());
  int textWidth = EPD_GetUTF8TextWidth(buffer, 16);
  int rightMargin = 30;
  int xPos = EPD_W - rightMargin - textWidth;
  EPD_ShowStringUTF8(xPos, 255, buffer, 16, BLACK);

  // Draw screen indicators
  extern void drawScreenIndicators(int activeScreen);
  drawScreenIndicators(0);

  // Update the e-ink display content with a single refresh
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

#endif
