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

// Rocker switch
#define PRV_KEY 6
#define NEXT_KEY 4

volatile bool prvButtonPressed = false;
volatile bool nextButtonPressed = false;
volatile unsigned long lastPrvPress = 0;
volatile unsigned long lastNextPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;

void IRAM_ATTR handlePrvButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastPrvPress > DEBOUNCE_DELAY) {
    prvButtonPressed = true;
    lastPrvPress = currentTime;
  }
}

void IRAM_ATTR handleNextButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastNextPress > DEBOUNCE_DELAY) {
    nextButtonPressed = true;
    lastNextPress = currentTime;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Rocker switch buttons
  pinMode(PRV_KEY, INPUT_PULLUP);
  pinMode(NEXT_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PRV_KEY), handlePrvButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(NEXT_KEY), handleNextButton, FALLING);

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
  loadStravaTokens();

  fetch_weather_data(httpResponseCode);
  fetch_weight_data(httpResponseCode);
  fetch_strava_data(httpResponseCode);

  display_main_screen(ImageBW, forceFullRefresh);
}

void loop() {
  static unsigned long lastWeatherFetch = 0;
  static unsigned long lastWeightFetch = 0;
  static unsigned long lastStravaFetch = 0;

  // Rocker switch: toggle bottom-right between Strava (0) and Weight (1)
  if (prvButtonPressed || nextButtonPressed) {
    prvButtonPressed = false;
    nextButtonPressed = false;
    bottomRightMode = (bottomRightMode == 0) ? 1 : 0;
    Serial.print("Bottom-right mode: ");
    Serial.println(bottomRightMode == 0 ? "Strava" : "Weight");
    forceFullRefresh = false;
    display_main_screen(ImageBW, forceFullRefresh);
  }

  if (millis() - lastWeatherFetch >= 1000UL*60*15) {
    Serial.println("Fetching weather data...");
    fetch_weather_data(httpResponseCode);
    lastWeatherFetch = millis();
    forceFullRefresh = false;
    display_main_screen(ImageBW, forceFullRefresh);
  }

  if (millis() - lastWeightFetch >= 1000UL*60*60*6) {
    Serial.println("Fetching weight data...");
    fetch_weight_data(httpResponseCode);
    lastWeightFetch = millis();
    forceFullRefresh = false;
    display_main_screen(ImageBW, forceFullRefresh);
  }

  if (millis() - lastStravaFetch >= 1000UL*60*60) {
    Serial.println("Fetching Strava data...");
    fetch_strava_data(httpResponseCode);
    lastStravaFetch = millis();
    forceFullRefresh = false;
    display_main_screen(ImageBW, forceFullRefresh);
  }

  delay(10);
}
