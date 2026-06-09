#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>

class ConfigManager {
public:
  // Google Pollen
  String googlePollenApiKey;

  // WiFi
  String wifiSsid;
  String wifiPassword;

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

  // Calendar
  String calendarApiUrl;
  String calendarBearerToken;

  // Firmware
  String firmwareVersion;

  // Misc
  String discordWebhookUrl;
  int remoteLogLevel;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=OFF
  String mdnsHostname;  // mDNS hostname (without .local)

  ConfigManager() : firmwareVersion("0.0.0"), remoteLogLevel(3), mdnsHostname("eink-panel") {}  // Default: ERROR only, hostname eink-panel

  // Load all config from NVS
  void loadAll() {
    Preferences prefs;

    prefs.begin("wifi", true);
    wifiSsid = prefs.getString("ssid", "");
    wifiPassword = prefs.getString("password", "");
    prefs.end();

    prefs.begin("withings", true);
    withingsClientId = prefs.getString("client_id", "");
    withingsClientSecret = prefs.getString("client_sec", "");
    withingsAccessToken = prefs.getString("access_token", "");
    withingsRefreshToken = prefs.getString("refresh_token", "");
    withingsUserId = prefs.getString("user_id", "");
    prefs.end();

    prefs.begin("google", true);
    googlePollenApiKey = prefs.getString("pollen_key", "");
    prefs.end();

    prefs.begin("strava", true);
    stravaClientId = prefs.getString("client_id", "");
    stravaClientSecret = prefs.getString("client_sec", "");
    stravaAccessToken = prefs.getString("access_token", "");
    stravaRefreshToken = prefs.getString("refresh_token", "");
    prefs.end();

    prefs.begin("calendar", true);
    calendarApiUrl = prefs.getString("api_url", "");
    calendarBearerToken = prefs.getString("bearer", "");
    prefs.end();

    prefs.begin("misc", true);
    discordWebhookUrl = prefs.getString("webhook", "");
    remoteLogLevel = prefs.getInt("log_level", 3);  // Default: ERROR
    mdnsHostname = prefs.getString("mdns_host", "eink-panel");
    prefs.end();

    prefs.begin("firmware", true);
    firmwareVersion = prefs.getString("version", "0.0.0");
    prefs.end();

    Serial.println("ConfigManager: loaded all config from NVS");
  }

  // Save all config to NVS
  void saveAll() {
    saveWifi();
    saveWithings();
    saveGooglePollen();
    saveStrava();
    saveCalendar();
    saveMisc();
    saveFirmwareVersion();
  }

  void saveWifi() {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", wifiSsid);
    prefs.putString("password", wifiPassword);
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
  
  void saveGooglePollen() {
    Preferences prefs;
    prefs.begin("google", false);
    prefs.putString("pollen_key", googlePollenApiKey);
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

  void saveCalendar() {
    Preferences prefs;
    prefs.begin("calendar", false);
    prefs.putString("api_url", calendarApiUrl);
    prefs.putString("bearer", calendarBearerToken);
    prefs.end();
  }

  void saveMisc() {
    Preferences prefs;
    prefs.begin("misc", false);
    prefs.putString("webhook", discordWebhookUrl);
    prefs.putInt("log_level", remoteLogLevel);
    prefs.putString("mdns_host", mdnsHostname);
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
    const char* namespaces[] = {"wifi", "withings", "google", "strava", "calendar", "firmware", "misc"};
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
