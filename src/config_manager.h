#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>

class ConfigManager {
public:
  // WiFi
  String wifiSsid;
  String wifiPassword;

  // OpenWeather
  String openWeatherApiKey;
  String cityId;

  // Withings
  String withingsClientId;
  String withingsClientSecret;
  String withingsAccessToken;
  String withingsRefreshToken;
  String withingsUserId;

  // Strava
  String stravaClientId;
  String stravaClientSecret;
  String stravaAccessToken;
  String stravaRefreshToken;

  // Firmware
  String firmwareVersion;

  ConfigManager() : cityId("2661552"), firmwareVersion("0.0.0") {}

  // Load all config from NVS
  void loadAll() {
    Preferences prefs;

    prefs.begin("wifi", true);
    wifiSsid = prefs.getString("ssid", "");
    wifiPassword = prefs.getString("password", "");
    prefs.end();

    prefs.begin("openweather", true);
    openWeatherApiKey = prefs.getString("api_key", "");
    cityId = prefs.getString("city_id", "2661552");
    prefs.end();

    prefs.begin("withings", true);
    withingsClientId = prefs.getString("client_id", "");
    withingsClientSecret = prefs.getString("client_sec", "");
    withingsAccessToken = prefs.getString("access_token", "");
    withingsRefreshToken = prefs.getString("refresh_token", "");
    withingsUserId = prefs.getString("user_id", "");
    prefs.end();

    prefs.begin("strava", true);
    stravaClientId = prefs.getString("client_id", "");
    stravaClientSecret = prefs.getString("client_sec", "");
    stravaAccessToken = prefs.getString("access_token", "");
    stravaRefreshToken = prefs.getString("refresh_token", "");
    prefs.end();

    prefs.begin("firmware", true);
    firmwareVersion = prefs.getString("version", "0.0.0");
    prefs.end();

    Serial.println("ConfigManager: loaded all config from NVS");
  }

  // Save all config to NVS
  void saveAll() {
    saveWifi();
    saveOpenWeather();
    saveWithings();
    saveStrava();
    saveFirmwareVersion();
  }

  void saveWifi() {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", wifiSsid);
    prefs.putString("password", wifiPassword);
    prefs.end();
  }

  void saveOpenWeather() {
    Preferences prefs;
    prefs.begin("openweather", false);
    prefs.putString("api_key", openWeatherApiKey);
    prefs.putString("city_id", cityId);
    prefs.end();
  }

  void saveWithings() {
    Preferences prefs;
    prefs.begin("withings", false);
    prefs.putString("client_id", withingsClientId);
    prefs.putString("client_sec", withingsClientSecret);
    prefs.putString("access_token", withingsAccessToken);
    prefs.putString("refresh_token", withingsRefreshToken);
    prefs.putString("user_id", withingsUserId);
    prefs.end();
  }

  void saveStrava() {
    Preferences prefs;
    prefs.begin("strava", false);
    prefs.putString("client_id", stravaClientId);
    prefs.putString("client_sec", stravaClientSecret);
    prefs.putString("access_token", stravaAccessToken);
    prefs.putString("refresh_token", stravaRefreshToken);
    prefs.end();
  }

  void saveFirmwareVersion() {
    Preferences prefs;
    prefs.begin("firmware", false);
    prefs.putString("version", firmwareVersion);
    prefs.end();
  }

  // Clear all NVS data (for factory reset)
  void clearAll() {
    Preferences prefs;
    const char* namespaces[] = {"wifi", "openweather", "withings", "strava", "firmware"};
    for (auto ns : namespaces) {
      prefs.begin(ns, false);
      prefs.clear();
      prefs.end();
    }
    Serial.println("ConfigManager: cleared all NVS data");
  }

  // Clear WiFi only (for AP mode reset)
  void clearWifi() {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();
    wifiSsid = "";
    wifiPassword = "";
  }
};

#endif
