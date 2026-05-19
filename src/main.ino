#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "pic.h"
#include "credentials.h"
#include "config_manager.h"

#include "main_screen.h"
#include "web_dashboard.h"

ConfigManager config;

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

  // WiFiManager: tries saved credentials, falls back to AP "EinkPanel"
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);  // 3 min portal timeout, then retry
  
  // Show AP mode info on e-ink display before starting portal
  wm.setAPCallback([](WiFiManager *mgr) {
    Serial.println("Entered AP mode: EinkPanel");
    Serial.print("Portal IP: ");
    Serial.println(WiFi.softAPIP());
    // TODO (Phase C2): Draw AP mode screen on e-ink
  });

  if (!wm.autoConnect("EinkPanel")) {
    Serial.println("WiFi connection failed, restarting...");
    ESP.restart();
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  config.loadAll();

  // Seed NVS from credentials.h for any empty fields
  // This can be removed once all tokens are managed via the web dashboard
  bool owChanged = false;
  if (config.openWeatherApiKey.length() == 0) { config.openWeatherApiKey = OPENWEATHER_API_KEY; owChanged = true; }
  if (owChanged) { config.saveOpenWeather(); Serial.println("Seeded OpenWeather from credentials.h"); }

  bool wChanged = false;
  if (config.withingsClientId.length() == 0) { config.withingsClientId = WITHINGS_CLIENT_ID; wChanged = true; }
  if (config.withingsClientSecret.length() == 0) { config.withingsClientSecret = WITHINGS_CLIENT_SECRET; wChanged = true; }
  if (config.withingsAccessToken.length() == 0) { config.withingsAccessToken = WITHINGS_ACCESS_TOKEN; wChanged = true; }
  if (config.withingsRefreshToken.length() == 0) { config.withingsRefreshToken = WITHINGS_REFRESH_TOKEN; wChanged = true; }
  if (config.withingsUserId.length() == 0) { config.withingsUserId = WITHINGS_USER_ID; wChanged = true; }
  if (wChanged) { config.saveWithings(); Serial.println("Seeded Withings from credentials.h"); }

  bool sChanged = false;
  if (config.stravaClientId.length() == 0) { config.stravaClientId = STRAVA_CLIENT_ID; sChanged = true; }
  if (config.stravaClientSecret.length() == 0) { config.stravaClientSecret = STRAVA_CLIENT_SECRET; sChanged = true; }
  if (config.stravaAccessToken.length() == 0) { config.stravaAccessToken = STRAVA_ACCESS_TOKEN; sChanged = true; }
  if (config.stravaRefreshToken.length() == 0) { config.stravaRefreshToken = STRAVA_REFRESH_TOKEN; sChanged = true; }
  if (sChanged) { config.saveStrava(); Serial.println("Seeded Strava from credentials.h"); }

  setupWebDashboard();

  configTime(0, 3600, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time...");
  time_t now = time(NULL);
  while (now < 1000000000) {
    delay(100);
    now = time(NULL);
  }
  Serial.print("Time set: ");
  Serial.println(ctime(&now));

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
