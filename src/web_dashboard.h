#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "config_manager.h"
#include "ota_update.h"

AsyncWebServer server(80);
extern ConfigManager config;

// Flags for async OTA trigger from dashboard
volatile bool otaTriggered = false;
volatile bool otaCheckTriggered = false;
OTAUpdateInfo lastUpdateInfo = {false, "", ""};
bool updateCheckDone = false;

// Firmware version baked in at build time
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.1.0"
#endif

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>E-Ink Panel</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,system-ui,sans-serif;background:#f5f5f5;color:#333;padding:16px;max-width:480px;margin:0 auto}
h1{font-size:1.3em;margin-bottom:12px}
h2{font-size:1.1em;margin:16px 0 8px;border-bottom:1px solid #ddd;padding-bottom:4px}
label{display:block;font-size:.85em;margin-top:8px;color:#555}
input{width:100%;padding:8px;margin-top:2px;border:1px solid #ccc;border-radius:4px;font-size:.9em}
.btn{display:inline-block;padding:10px 16px;margin:8px 4px 0 0;border:none;border-radius:4px;font-size:.9em;cursor:pointer;color:#fff}
.btn-save{background:#2563eb}
.btn-reset{background:#dc2626}
.btn-status{background:#059669}
.status{background:#fff;border:1px solid #ddd;border-radius:4px;padding:12px;margin-top:12px;font-size:.85em;display:none}
.msg{padding:8px;margin:8px 0;border-radius:4px;font-size:.85em;display:none}
.msg-ok{background:#d1fae5;color:#065f46}
.msg-err{background:#fee2e2;color:#991b1b}
</style>
</head>
<body>
<h1>E-Ink Desk Panel</h1>
<div id="msg" class="msg"></div>

<h2>Withings</h2>
<label>Client ID<input type="text" id="w_cid"></label>
<label>Client Secret<input type="password" id="w_csec"></label>
<label>Access Token<input type="text" id="w_at"></label>
<label>Refresh Token<input type="text" id="w_rt"></label>
<label>User ID<input type="text" id="w_uid"></label>

<h2>Strava</h2>
<label>Client ID<input type="text" id="s_cid"></label>
<label>Client Secret<input type="password" id="s_csec"></label>
<label>Access Token<input type="text" id="s_at"></label>
<label>Refresh Token<input type="text" id="s_rt"></label>

<h2>Google Pollen</h2>
<label>API Key<input type="password" id="g_pollen"></label>

<h2>System Logging</h2>
<label>Discord Webhook URL<input type="password" id="d_webhook"></label>

<div style="margin-top:16px">
<button class="btn btn-save" onclick="save()">Save Settings</button>
<button class="btn btn-status" onclick="status()">Status</button>
<button class="btn btn-save" onclick="checkUpdate()">Check for Updates</button>
<button class="btn btn-reset" onclick="reset()">Reset WiFi</button>
</div>

<div id="statusBox" class="status"></div>
<div id="updateBox" class="status"></div>

<script>
function msg(txt,ok){
  var m=document.getElementById('msg');
  m.textContent=txt;m.className='msg '+(ok?'msg-ok':'msg-err');m.style.display='block';
  setTimeout(function(){m.style.display='none'},4000);
}
function save(){
  var d={
    w_cid:document.getElementById('w_cid').value,
    w_csec:document.getElementById('w_csec').value,
    w_at:document.getElementById('w_at').value,
    w_rt:document.getElementById('w_rt').value,
    w_uid:document.getElementById('w_uid').value,
    s_cid:document.getElementById('s_cid').value,
    s_csec:document.getElementById('s_csec').value,
    s_at:document.getElementById('s_at').value,
    s_rt:document.getElementById('s_rt').value,
    g_pollen:document.getElementById('g_pollen').value,
    d_webhook:document.getElementById('d_webhook').value
  };
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})
  .then(function(r){return r.json()})
  .then(function(j){msg(j.message||'Saved',j.status==='ok')})
  .catch(function(){msg('Error saving',false)});
}
function status(){
  var box=document.getElementById('statusBox');
  box.style.display='block';box.textContent='Loading...';
  fetch('/status').then(function(r){return r.json()}).then(function(j){
    box.innerHTML='<b>Firmware:</b> '+j.firmware+'<br><b>IP:</b> '+j.ip+'<br><b>WiFi:</b> '+j.ssid+' ('+j.rssi+' dBm)<br><b>Uptime:</b> '+j.uptime+'s<br><b>Free heap:</b> '+j.free_heap+' bytes';
  }).catch(function(){box.textContent='Error fetching status'});
}
function reset(){
  if(!confirm('Reset WiFi credentials? Device will restart in AP mode.'))return;
  fetch('/reset',{method:'POST'}).then(function(){msg('Resetting...',true)}).catch(function(){msg('Error',false)});
}
function checkUpdate(){
  var box=document.getElementById('updateBox');
  box.style.display='block';box.textContent='Checking for updates...';
  fetch('/check-update').then(function(r){return r.json()}).then(function(j){
    if(j.checking){
      setTimeout(function(){
        fetch('/check-update').then(function(r){return r.json()}).then(function(j2){
          if(j2.available){
            box.innerHTML='<b>Update available:</b> '+j2.version+'<br><button class="btn btn-save" onclick="applyUpdate()">Update Now</button>';
          } else {
            box.textContent='Already up to date ('+j2.current+')';
          }
        }).catch(function(){box.textContent='Error fetching result'});
      },5000);
    } else if(j.available){
      box.innerHTML='<b>Update available:</b> '+j.version+'<br><button class="btn btn-save" onclick="applyUpdate()">Update Now</button>';
    } else {
      box.textContent='Already up to date ('+j.current+')';
    }
  }).catch(function(){box.textContent='Error checking for updates'});
}
function applyUpdate(){
  var box=document.getElementById('updateBox');
  box.textContent='Downloading and applying update... Do not unplug the device.';
  fetch('/apply-update',{method:'POST'}).then(function(r){return r.json()}).then(function(j){
    if(j.status==='started'){box.textContent='Update started. Device will reboot when complete.';}
    else{box.textContent='Error: '+j.message;}
  }).catch(function(){box.textContent='Error triggering update'});
}
// Load current values on page load
fetch('/status').then(function(r){return r.json()}).then(function(j){
  if(j.w_cid)document.getElementById('w_cid').value=j.w_cid;
  if(j.w_uid)document.getElementById('w_uid').value=j.w_uid;
  if(j.s_cid)document.getElementById('s_cid').value=j.s_cid;
  if(j.g_pollen)document.getElementById('g_pollen').value=j.g_pollen;
  if(j.d_webhook)document.getElementById('d_webhook').value=j.d_webhook;
}).catch(function(){});
</script>
</body>
</html>
)rawliteral";

void setupWebDashboard() {
  // Serve dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", DASHBOARD_HTML);
  });

  // Save settings
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // handled in body handler
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Simple JSON parsing — we use Arduino_JSON
    String body = String((char*)data).substring(0, len);
    JSONVar obj = JSON.parse(body);
    
    if (JSON.typeof(obj) == "undefined") {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    // Withings
    if (JSON.typeof(obj["w_cid"]) != "undefined") {
      String val = (const char*)obj["w_cid"];
      if (val.length() > 0) config.withingsClientId = val;
    }
    if (JSON.typeof(obj["w_csec"]) != "undefined") {
      String val = (const char*)obj["w_csec"];
      if (val.length() > 0) config.withingsClientSecret = val;
    }
    if (JSON.typeof(obj["w_at"]) != "undefined") {
      String val = (const char*)obj["w_at"];
      if (val.length() > 0) config.withingsAccessToken = val;
    }
    if (JSON.typeof(obj["w_rt"]) != "undefined") {
      String val = (const char*)obj["w_rt"];
      if (val.length() > 0) config.withingsRefreshToken = val;
    }
    if (JSON.typeof(obj["w_uid"]) != "undefined") {
      String val = (const char*)obj["w_uid"];
      if (val.length() > 0) config.withingsUserId = val;
    }

    // Google
     if (JSON.typeof(obj["g_pollen"]) != "undefined") {
       String val = (const char*)obj["g_pollen"];
       if (val.length() > 0) config.googlePollenApiKey = val;
     }

    // Strava
    if (JSON.typeof(obj["s_cid"]) != "undefined") {
      String val = (const char*)obj["s_cid"];
      if (val.length() > 0) config.stravaClientId = val;
    }
    if (JSON.typeof(obj["s_csec"]) != "undefined") {
      String val = (const char*)obj["s_csec"];
      if (val.length() > 0) config.stravaClientSecret = val;
    }
    if (JSON.typeof(obj["s_at"]) != "undefined") {
      String val = (const char*)obj["s_at"];
      if (val.length() > 0) config.stravaAccessToken = val;
    }
    if (JSON.typeof(obj["s_rt"]) != "undefined") {
      String val = (const char*)obj["s_rt"];
      if (val.length() > 0) config.stravaRefreshToken = val;
    }

    // System Logging
    if (JSON.typeof(obj["d_webhook"]) != "undefined") {
      String val = (const char*)obj["d_webhook"];
      if (val.length() > 0) config.discordWebhookUrl = val;
    }

    config.saveAll();
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Settings saved. Restart to apply.\"}");
  });

  // Status endpoint
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    // Non-secret config values for form pre-fill
    json += "\"w_cid\":\"" + config.withingsClientId + "\",";
    json += "\"w_uid\":\"" + config.withingsUserId + "\",";
    json += "\"s_cid\":\"" + config.stravaClientId + "\",";
    json += "\"g_pollen\":\"" + config.googlePollenApiKey + "\",";
    json += "\"d_webhook\":\"" + config.discordWebhookUrl + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  // Reset WiFi
  server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Resetting WiFi...\"}");
    delay(500);
    WiFiManager wm;
    wm.resetSettings();
    ESP.restart();
  });

  // Check for OTA update (deferred to main loop)
  server.on("/check-update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (updateCheckDone) {
      // Return cached result
      String json = "{";
      json += "\"available\":" + String(lastUpdateInfo.available ? "true" : "false") + ",";
      json += "\"current\":\"" + String(FIRMWARE_VERSION) + "\"";
      if (lastUpdateInfo.available) {
        json += ",\"version\":\"" + lastUpdateInfo.version + "\"";
      }
      json += "}";
      updateCheckDone = false;
      request->send(200, "application/json", json);
    } else {
      otaCheckTriggered = true;
      request->send(200, "application/json", "{\"checking\":true,\"current\":\"" + String(FIRMWARE_VERSION) + "\"}");
    }
  });

  // Trigger OTA update (runs in main loop via flag)
  server.on("/apply-update", HTTP_POST, [](AsyncWebServerRequest *request) {
    otaTriggered = true;
    request->send(200, "application/json", "{\"status\":\"started\",\"message\":\"Update started\"}");
  });

  server.begin();
  Serial.println("Web dashboard started on port 80");

  // mDNS
  if (MDNS.begin("eink-panel")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://eink-panel.local");
  }
}

#endif
