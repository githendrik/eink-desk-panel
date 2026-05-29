#ifndef REMOTE_LOG_H
#define REMOTE_LOG_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config_manager.h"

extern ConfigManager config;

// Log levels: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=OFF
enum LogLevel {
  LOG_DEBUG = 0,
  LOG_INFO = 1,
  LOG_WARN = 2,
  LOG_ERROR = 3,
  LOG_OFF = 4
};

static const char* LOG_LEVEL_NAMES[] = {"DEBUG", "INFO", "WARN", "ERROR", "OFF"};

// Rate limiting: don't send more than 1 message per 5 seconds
static unsigned long lastRemoteLogTime = 0;
const unsigned long REMOTE_LOG_MIN_INTERVAL = 5000;

void remote_log(LogLevel level, const String& module, const String& message) {
  // Always log locally
  Serial.print("[");
  Serial.print(LOG_LEVEL_NAMES[level]);
  Serial.print("] [");
  Serial.print(module);
  Serial.print("] ");
  Serial.println(message);

  // Skip remote if level is below configured threshold
  if (level < config.remoteLogLevel) {
    return;
  }

  // If no URL configured or WiFi down, skip
  if (config.discordWebhookUrl.length() == 0 || WiFi.status() != WL_CONNECTED) {
    return;
  }

  // Rate limit to avoid blocking the main loop too often
  if (millis() - lastRemoteLogTime < REMOTE_LOG_MIN_INTERVAL) {
    return;
  }

  // Discord webhooks require HTTPS
  WiFiClientSecure client;
  client.setInsecure();  // Skip cert verification (constrained device)

  HTTPClient http;
  http.setTimeout(2000);
  http.begin(client, config.discordWebhookUrl);
  http.addHeader("Content-Type", "application/json");

  // Escape JSON-special characters
  String escapedMsg = message;
  escapedMsg.replace("\\", "\\\\");
  escapedMsg.replace("\"", "\\\"");
  escapedMsg.replace("\n", "\\n");
  escapedMsg.replace("\r", "\\r");

  String escapedModule = module;
  escapedModule.replace("\\", "\\\\");
  escapedModule.replace("\"", "\\\"");

  String jsonPayload = "{\"content\":\"[" + String(LOG_LEVEL_NAMES[level]) + "] [" + escapedModule + "] " + escapedMsg + "\"}";

  int rc = http.POST(jsonPayload);
  if (rc < 0) {
    Serial.print("Remote log failed: ");
    Serial.println(http.errorToString(rc));
  } else if (rc == 429) {
    Serial.println("Remote log rate-limited by Discord");
  }
  http.end();

  lastRemoteLogTime = millis();
}

#endif