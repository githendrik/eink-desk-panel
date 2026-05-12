#include <WiFi.h>
#include <HTTPClient.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "pic.h"
#include "credentials.h"

#include "weather_screen.h"

uint8_t ImageBW[15000];

int httpResponseCode;

void setup() {
  Serial.begin(115200);

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  EPD_GPIOInit();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  fetch_weather_data(httpResponseCode);

  display_weather_screen(ImageBW, true);
}

void loop() {
  static unsigned long lastWeatherFetch = 0;

  if (millis() - lastWeatherFetch >= 1000*60*60) {
    Serial.println("Fetching weather data...");
    fetch_weather_data(httpResponseCode);
    lastWeatherFetch = millis();
    display_weather_screen(ImageBW, false);
  }

  delay(10);
}
