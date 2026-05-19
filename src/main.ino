#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <esp_ota_ops.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "pic.h"
#include "config_manager.h"

#include "main_screen.h"
#include "ota_update.h"
#include "web_dashboard.h"

ConfigManager config;

uint8_t ImageBW[15000];

int httpResponseCode;
bool forceFullRefresh = true;

// Rocker switch
#define PRV_KEY 6
#define NEXT_KEY 4
#define MENU_KEY 1
#define OK_KEY 5

volatile bool prvButtonPressed = false;
volatile bool nextButtonPressed = false;
volatile bool menuButtonPressed = false;
volatile bool okButtonPressed = false;
volatile unsigned long lastPrvPress = 0;
volatile unsigned long lastNextPress = 0;
volatile unsigned long lastMenuPress = 0;
volatile unsigned long lastOkPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// Screen state: 0 = main, 1 = status
int currentScreen = 0;
// Status screen OTA state: 0 = idle, 1 = checking, 2 = update available, 3 = no update
int statusOtaState = 0;

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

void IRAM_ATTR handleMenuButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastMenuPress > DEBOUNCE_DELAY) {
    menuButtonPressed = true;
    lastMenuPress = currentTime;
  }
}

void IRAM_ATTR handleOkButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastOkPress > DEBOUNCE_DELAY) {
    okButtonPressed = true;
    lastOkPress = currentTime;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Rocker switch buttons
  pinMode(PRV_KEY, INPUT_PULLUP);
  pinMode(NEXT_KEY, INPUT_PULLUP);
  pinMode(MENU_KEY, INPUT_PULLUP);
  pinMode(OK_KEY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PRV_KEY), handlePrvButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(NEXT_KEY), handleNextButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(MENU_KEY), handleMenuButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(OK_KEY), handleOkButton, FALLING);

  EPD_GPIOInit();
  EPD_Init();
  EPD_Clear();
  delay(100);

  // WiFiManager: tries saved credentials, falls back to AP "EinkPanel"
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);  // 3 min portal timeout, then retry
  
  // Show AP mode info on e-ink display before starting portal
  wm.setAPCallback([](WiFiManager *mgr) {
    Serial.println("Entered AP mode: EinkPanel");
    Serial.print("Portal IP: ");
    Serial.println(WiFi.softAPIP());
    display_ap_screen(ImageBW, "EinkPanel", WiFi.softAPIP().toString().c_str());
  });

  if (!wm.autoConnect("EinkPanel")) {
    Serial.println("WiFi connection failed, restarting...");
    ESP.restart();
  }

  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  config.loadAll();

  setupWebDashboard();

  configTime(0, 3600, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time...");
  time_t now = time(NULL);
  unsigned long ntpStart = millis();
  while (now < 1000000000 && millis() - ntpStart < 15000) {
    delay(100);
    now = time(NULL);
  }
  Serial.print("Time set: ");
  Serial.println(ctime(&now));

  // Mark OTA app as valid after WiFi + NTP succeed (rollback safety)
  esp_ota_mark_app_valid_cancel_rollback();
  Serial.println("App marked as valid (rollback cancelled)");

  // Check for OTA update on boot
  Serial.println("Checking for OTA update...");
  otaSetProgressCallback([](int percent) {
    display_ota_screen(ImageBW, lastUpdateInfo.version.c_str(), percent);
  });
  lastUpdateInfo = otaCheckForUpdate();
  if (lastUpdateInfo.available) {
    display_ota_screen(ImageBW, lastUpdateInfo.version.c_str(), 0);
    if (otaApplyUpdate(lastUpdateInfo)) {
      Serial.println("OTA update applied, rebooting...");
      delay(1000);
      ESP.restart();
    }
  }

  fetch_weather_data(httpResponseCode);
  fetch_weight_data(httpResponseCode);
  fetch_strava_data(httpResponseCode);

  display_main_screen(ImageBW, forceFullRefresh);
}

void loop() {
  static unsigned long lastWeatherFetch = 0;
  static unsigned long lastWeightFetch = 0;
  static unsigned long lastStravaFetch = 0;

  // Dashboard-triggered OTA check
  if (otaCheckTriggered) {
    otaCheckTriggered = false;
    Serial.println("OTA check triggered from dashboard");
    lastUpdateInfo = otaCheckForUpdate();
    updateCheckDone = true;
  }

  // Dashboard-triggered OTA update
  if (otaTriggered) {
    otaTriggered = false;
    Serial.println("OTA triggered from dashboard");
    lastUpdateInfo = otaCheckForUpdate();
    if (lastUpdateInfo.available) {
      display_ota_screen(ImageBW, lastUpdateInfo.version.c_str(), 0);
      if (otaApplyUpdate(lastUpdateInfo)) {
        Serial.println("OTA update applied, rebooting...");
        delay(1000);
        ESP.restart();
      }
    }
  }

  // MENU button: toggle between main and status screen
  if (menuButtonPressed) {
    menuButtonPressed = false;
    if (currentScreen == 0) {
      currentScreen = 1;
      statusOtaState = 0;
      Serial.println("Switching to status screen");
      display_status_screen(ImageBW, statusOtaState, nullptr);
    } else {
      currentScreen = 0;
      Serial.println("Switching to main screen");
      forceFullRefresh = false;
      display_main_screen(ImageBW, forceFullRefresh);
    }
  }

  // OK button: on status screen, check for update or apply update
  if (okButtonPressed) {
    okButtonPressed = false;
    if (currentScreen == 1) {
      if (statusOtaState == 0 || statusOtaState == 3) {
        // Start OTA check
        statusOtaState = 1;
        display_status_screen(ImageBW, statusOtaState, nullptr);
        Serial.println("OTA check from status screen...");
        lastUpdateInfo = otaCheckForUpdate();
        updateCheckDone = true;
        if (lastUpdateInfo.available) {
          statusOtaState = 2;
          display_status_screen(ImageBW, statusOtaState, lastUpdateInfo.version.c_str());
        } else {
          statusOtaState = 3;
          display_status_screen(ImageBW, statusOtaState, nullptr);
        }
      } else if (statusOtaState == 2) {
        // Apply update
        Serial.println("OTA apply from status screen...");
        display_ota_screen(ImageBW, lastUpdateInfo.version.c_str(), 0);
        if (otaApplyUpdate(lastUpdateInfo)) {
          Serial.println("OTA update applied, rebooting...");
          delay(1000);
          ESP.restart();
        } else {
          // Update failed — go back to status
          statusOtaState = 0;
          display_status_screen(ImageBW, statusOtaState, nullptr);
        }
      }
    }
  }

  // Rocker switch: toggle bottom-right between Strava (0) and Weight (1)
  if (prvButtonPressed || nextButtonPressed) {
    prvButtonPressed = false;
    nextButtonPressed = false;
    if (currentScreen == 1) {
      // On status screen, rocker goes back to main
      currentScreen = 0;
      forceFullRefresh = false;
      display_main_screen(ImageBW, forceFullRefresh);
    } else {
      bottomRightMode = (bottomRightMode == 0) ? 1 : 0;
      Serial.print("Bottom-right mode: ");
      Serial.println(bottomRightMode == 0 ? "Strava" : "Weight");
      forceFullRefresh = false;
      display_main_screen(ImageBW, forceFullRefresh);
    }
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
