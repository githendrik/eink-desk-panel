#include <WiFi.h>
#include <HTTPClient.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "pic.h"
#include "credentials.h"

#include "main_screen.h"

uint8_t ImageBW[15000];

int httpResponseCode;
bool forceFullRefresh = true;

void setup() {
  Serial.begin(115200);

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  EPD_GPIOInit();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  configTime(0, 3600, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time...");
  time_t now = time(NULL);
  while (now < 1000000000) {
    delay(100);
    now = time(NULL);
  }
  Serial.print("Time set: ");
  Serial.println(ctime(&now));

  loadWithingsTokens();

  fetch_weather_data(httpResponseCode);
  fetch_weight_data(httpResponseCode);

  display_main_screen(ImageBW, forceFullRefresh);
}

void loop() {
  static unsigned long lastWeatherFetch = 0;
  static unsigned long lastWeightFetch = 0;

  if (millis() - lastWeatherFetch >= 1000*60*60) {
    Serial.println("Fetching weather data...");
    fetch_weather_data(httpResponseCode);
    lastWeatherFetch = millis();
    forceFullRefresh = false;
    display_main_screen(ImageBW, forceFullRefresh);
  }

  if (millis() - lastWeightFetch >= 1000*60*60*6) {
    Serial.println("Fetching weight data...");
    fetch_weight_data(httpResponseCode);
    lastWeightFetch = millis();
    forceFullRefresh = false;
    display_main_screen(ImageBW, forceFullRefresh);
  }

  delay(10);
}
