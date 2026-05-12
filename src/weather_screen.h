#ifndef WEATHER_SCREEN_H
#define WEATHER_SCREEN_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "credentials.h"

String temperature;
String aareTemp;
String pollenLevel;

String openWeatherMapApiKey = OPENWEATHER_API_KEY;
String cityId = "2661552";

String jsonBuffer;
JSONVar myObject;
String aareJsonBuffer;
JSONVar aareObject;
String pollenJsonBuffer;
JSONVar pollenObject;

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

void fetch_weather_data(int& httpResponseCode)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    return;
  }

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

    double tempValue = (double)myObject["main"]["temp"];
    temperature = String((int)tempValue);

    Serial.print("String Temperature: ");
    Serial.println(temperature);
  } else {
    Serial.print("Weather API error: ");
    Serial.println(httpResponseCode);
    http.end();
    return;
  }

  http.end();

  String pollenServerPath = "http://api.openweathermap.org/data/2.5/air_pollution?lat=46.9480&lon=7.4474&appid=" + openWeatherMapApiKey;
  HTTPClient httpPollen;
  httpPollen.begin(client, pollenServerPath);
  httpResponseCode = httpPollen.GET();

  if (httpResponseCode == 200) {
    pollenJsonBuffer = httpPollen.getString();
    Serial.println(pollenJsonBuffer);
    pollenObject = JSON.parse(pollenJsonBuffer);

    if (JSON.typeof(pollenObject) == "undefined") {
      Serial.println("Parsing pollen data failed!");
      httpPollen.end();
      pollenLevel = "n/a";
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

    Serial.print("String aare temp: ");
    Serial.println(aareTemp);
  } else {
    Serial.print("Aare API error: ");
    Serial.println(httpResponseCode);
  }

  httpAare.end();
}

void display_weather_screen(uint8_t* ImageBW, bool& forceFullRefresh)
{
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

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s°", temperature.c_str());
  EPD_ShowStringUTF8(120, 80, buffer, 72, BLACK);

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Pollen %s", pollenLevel.c_str());
  EPD_ShowStringUTF8(120, 170, buffer, 24, BLACK);

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Aare %s°", aareTemp.c_str());
  EPD_ShowStringUTF8(120, 220, buffer, 24, BLACK);

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

#endif
