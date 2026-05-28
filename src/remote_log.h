#ifndef REMOTE_LOG_H
#define REMOTE_LOG_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "config_manager.h"

extern ConfigManager config;

void remote_log(const String& module, const String& message) {
  // Always log locally
  Serial.print("[");
  Serial.print(module);
  Serial.print("] ");
  Serial.println(message);

  // If a URL is configured, push to remote
  if (config.discordWebhookUrl.length() > 0 && WiFi.status() == WL_CONNECTED) {
    // We launch this 'fire and forget'
    HTTPClient http;
    // Set a very short timeout so it doesn't block the UI refresh
    http.setTimeout(1000); 
    
    // Quick basic auth / bearer auth could be added later if needed.
    // For Discord webhooks, unauthenticated POST with JSON payload works.
    http.begin(config.discordWebhookUrl);
    http.addHeader("Content-Type", "application/json");

    // Discord expects {"content": "..."} format in JSON
    // We escape double quotes to handle valid JSON
    String escapedMsg = message;
    escapedMsg.replace("\"", "\\\"");
    String jsonPayload = "{\"content\": \"[" + module + "] " + escapedMsg + "\"}";

    http.POST(jsonPayload);
    http.end();
  }
}

#endif