#ifndef TRANSPORT_SCREEN_H
#define TRANSPORT_SCREEN_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include "EPD_GUI.h"

// Transport-related data structure and variables
struct Departure {
  String time;
  String destination;
  String platform;
  String busNumber;
  int delayMinutes;  // Delay in minutes (0 = on time, positive = late)
  unsigned long timestamp;  // Unix timestamp for sorting
};

Departure station1Departures[5];  // Departures from station 1 (BernMobil)
Departure station2Departures[5];  // Departures from station 2 (PostAuto)
int numStation1Departures = 0;
int numStation2Departures = 0;

// JSON buffers
String transportJsonBuffer;
JSONVar transportObject;

// Helper function to parse a single stationboard entry
void parseStationboardEntry(JSONVar entry, Departure* departuresArray, int index) {
  // Get departure timestamp for sorting
  String timestampStr = JSON.stringify(entry["stop"]["departureTimestamp"]);
  departuresArray[index].timestamp = (unsigned long)timestampStr.toInt();
  
  // Get departure time (format: "2026-01-23T11:38:00+0100")
  String fullTime = JSON.stringify(entry["stop"]["departure"]);
  fullTime.replace("\"", "");
  
  // Extract HH:MM from the time string
  int tIndex = fullTime.indexOf('T');
  if (tIndex != -1) {
    String timeOnly = fullTime.substring(tIndex + 1, tIndex + 6);
    departuresArray[index].time = timeOnly;
  } else {
    departuresArray[index].time = "N/A";
  }
  
  // Get bus/train number
  String busNum = JSON.stringify(entry["number"]);
  busNum.replace("\"", "");
  if (busNum == "null" || busNum.length() == 0) {
    departuresArray[index].busNumber = "?";
  } else {
    departuresArray[index].busNumber = busNum;
  }
  
  // Get platform
  String platform = JSON.stringify(entry["stop"]["platform"]);
  platform.replace("\"", "");
  if (platform == "null" || platform.length() == 0) {
    departuresArray[index].platform = "";
  } else {
    departuresArray[index].platform = platform;
  }
  
  // Get destination
  String dest = JSON.stringify(entry["to"]);
  dest.replace("\"", "");
  departuresArray[index].destination = dest;
  
  // Get delay (in minutes)
  String delayStr = JSON.stringify(entry["stop"]["delay"]);
  if (delayStr == "null" || delayStr.length() == 0) {
    departuresArray[index].delayMinutes = 0;
  } else {
    // Delay is already in minutes in stationboard API
    departuresArray[index].delayMinutes = delayStr.toInt();
  }
}

// Function to fetch transport departure times from multiple stations
void fetch_transport_data(int& httpResponseCode, String (*httpGETRequest)(const char*))
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected - cannot fetch transport data");
    numStation1Departures = 0;
    numStation2Departures = 0;
    return;
  }
  
  numStation1Departures = 0;
  numStation2Departures = 0;
  
  // Define stations to query using the stationboard API
  // Station IDs: 8590069 (Bremgarten BE, Stuckishaus), 8571371 (another station)
  // Use fields parameter to limit response size
  String stations[2] = {
    "http://transport.opendata.ch/v1/stationboard?id=8590069&limit=5&fields[]=stationboard/stop/departure&fields[]=stationboard/stop/departureTimestamp&fields[]=stationboard/stop/delay&fields[]=stationboard/stop/platform&fields[]=stationboard/number&fields[]=stationboard/to",
    "http://transport.opendata.ch/v1/stationboard?id=8571371&limit=12&fields[]=stationboard/stop/departure&fields[]=stationboard/stop/departureTimestamp&fields[]=stationboard/stop/delay&fields[]=stationboard/stop/platform&fields[]=stationboard/number&fields[]=stationboard/to"
  };
  
  // Fetch from both stations
  for (int stationIndex = 0; stationIndex < 2; stationIndex++) {
    httpResponseCode = 0;
    
    Serial.print("Fetching from station ");
    Serial.println(stationIndex + 1);
    
    transportJsonBuffer = httpGETRequest(stations[stationIndex].c_str());
    transportObject = JSON.parse(transportJsonBuffer);
    
    if (JSON.typeof(transportObject) == "undefined") {
      Serial.print("Transport data parsing failed for station ");
      Serial.println(stationIndex + 1);
      Serial.print("JSON buffer length: ");
      Serial.println(transportJsonBuffer.length());
      Serial.print("JSON response: ");
      Serial.println(transportJsonBuffer.substring(0, 500));
      continue;
    }
    
    // Debug: Check if stationboard key exists
    String stationboardType = JSON.typeof(transportObject["stationboard"]);
    Serial.print("Stationboard type: ");
    Serial.println(stationboardType);
    
    if (stationboardType == "undefined" || stationboardType == "null") {
      Serial.print("Stationboard key missing/null for station ");
      Serial.println(stationIndex + 1);
      Serial.print("JSON response: ");
      Serial.println(transportJsonBuffer.substring(0, 500));  // Print first 500 chars
      continue;
    }
    
    JSONVar stationboard = transportObject["stationboard"];
    
    // Determine which array and counter to use
    Departure* targetArray = (stationIndex == 0) ? station1Departures : station2Departures;
    int& targetCount = (stationIndex == 0) ? numStation1Departures : numStation2Departures;
    
    int stationboardLength = stationboard.length();
    Serial.print("Stationboard entries received: ");
    Serial.println(stationboardLength);
    
    if (stationboardLength < 0) {
      Serial.println("ERROR: Stationboard is not an array!");
      Serial.print("JSON response: ");
      Serial.println(transportJsonBuffer.substring(0, 500));
      continue;
    }
    
    // Parse stationboard entries from this station
    int skipped = 0;
    for (int i = 0; i < stationboard.length() && targetCount < 5; i++) {
      // For station 2, filter to only include departures to "Bern, Hauptbahnhof"
      if (stationIndex == 1) {
        String destination = JSON.stringify(stationboard[i]["to"]);
        destination.replace("\"", "");
        if (destination != "Bern, Hauptbahnhof") {
          skipped++;
          Serial.print("  Skipping: ");
          Serial.println(destination);
          continue;  // Skip this departure
        }
      }
      
      parseStationboardEntry(stationboard[i], targetArray, targetCount);
      targetCount++;
    }
    
    if (stationIndex == 1) {
      Serial.print("Station 2: Skipped ");
      Serial.print(skipped);
      Serial.print(" departures, kept ");
      Serial.println(targetCount);
    }
    
    // Sort this station's departures by timestamp
    for (int i = 0; i < targetCount - 1; i++) {
      for (int j = 0; j < targetCount - i - 1; j++) {
        if (targetArray[j].timestamp > targetArray[j + 1].timestamp) {
          // Swap
          Departure temp = targetArray[j];
          targetArray[j] = targetArray[j + 1];
          targetArray[j + 1] = temp;
        }
      }
    }
  }
  
  // Print departures from both stations
  Serial.println("\n=== Station 1 Departures (BernMobil) ===");
  for (int i = 0; i < numStation1Departures; i++) {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(station1Departures[i].time);
    Serial.print(" Bus ");
    Serial.print(station1Departures[i].busNumber);
    Serial.print(" to ");
    Serial.print(station1Departures[i].destination);
    if (station1Departures[i].delayMinutes > 0) {
      Serial.print(" +");
      Serial.print(station1Departures[i].delayMinutes);
      Serial.print("m");
    }
    Serial.println();
  }
  Serial.print("Total: ");
  Serial.println(numStation1Departures);
  
  Serial.println("\n=== Station 2 Departures (PostAuto) ===");
  for (int i = 0; i < numStation2Departures; i++) {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(station2Departures[i].time);
    Serial.print(" Bus ");
    Serial.print(station2Departures[i].busNumber);
    Serial.print(" to ");
    Serial.print(station2Departures[i].destination);
    if (station2Departures[i].delayMinutes > 0) {
      Serial.print(" +");
      Serial.print(station2Departures[i].delayMinutes);
      Serial.print("m");
    }
    Serial.println();
  }
  Serial.print("Total: ");
  Serial.println(numStation2Departures);
}

// Display Screen 2 - Transport Departures
void display_transport_screen(uint8_t* ImageBW, bool& forceFullRefresh)
{
  static char buffer[128];

  // Do a full refresh if switching screens
  if (forceFullRefresh) {
    EPD_Init();
    EPD_Clear();
    // Removed blocking delay - EPD_Clear() already waits for completion
    forceFullRefresh = false;
  }

  // Initialize fast refresh mode
  EPD_Init_Fast(Fast_Seconds_1_5s);
  
  // Create a white canvas and clear the display
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE); // Fill display with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Clear to white

  // Split screen vertically: left side for station 1, right side for station 2
  int screenMid = EPD_W / 2;
  int rowHeight = 42;  // Reduced from 52
  int headerY = 15;  // Moved down from 5
  int yStart = 60;  // Moved down from 40
  
  // Draw headers
  EPD_ShowStringUTF8(10, headerY, "BernMobil", 20, BLACK);  // Increased from 16 to 20
  EPD_ShowStringUTF8(screenMid + 10, headerY, "PostAuto", 20, BLACK);  // Increased from 16 to 20
  
  // Draw vertical separator line (moved 30px left)
  EPD_DrawLine(screenMid - 30, 0, screenMid - 30, EPD_H, BLACK);

  // Left side - Station 1 (BernMobil)
  int leftTimeCol = 10;
  int leftBusCol = 100;  // Moved closer to time (was 130)
  int yPos = yStart;
  
  for (int i = 0; i < numStation1Departures && i < 5; i++) {
    // Time column (with delay indicator if present)
    memset(buffer, 0, sizeof(buffer));
    if (station1Departures[i].delayMinutes > 0) {
      snprintf(buffer, sizeof(buffer), "%s+%d", 
               station1Departures[i].time.c_str(), 
               station1Departures[i].delayMinutes);
    } else {
      snprintf(buffer, sizeof(buffer), "%s", station1Departures[i].time.c_str());
    }
    EPD_ShowStringUTF8(leftTimeCol, yPos, buffer, 24, BLACK);
    
    // Bus number column - smaller font
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", station1Departures[i].busNumber.c_str());
    EPD_ShowStringUTF8(leftBusCol, yPos + 8, buffer, 16, BLACK);  // +4 offset to align with time
    
    yPos += rowHeight;
  }
  
  // Right side - Station 2 (PostAuto)
  int rightTimeCol = screenMid + 10;
  int rightBusCol = screenMid + 100;  // Moved closer to time (was screenMid + 130)
  yPos = yStart;
  
  for (int i = 0; i < numStation2Departures && i < 5; i++) {
    // Time column (with delay indicator if present)
    memset(buffer, 0, sizeof(buffer));
    if (station2Departures[i].delayMinutes > 0) {
      snprintf(buffer, sizeof(buffer), "%s+%d", 
               station2Departures[i].time.c_str(), 
               station2Departures[i].delayMinutes);
    } else {
      snprintf(buffer, sizeof(buffer), "%s", station2Departures[i].time.c_str());
    }
    EPD_ShowStringUTF8(rightTimeCol, yPos, buffer, 24, BLACK);
    
    // Bus number column - smaller font
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s", station2Departures[i].busNumber.c_str());
    EPD_ShowStringUTF8(rightBusCol, yPos + 8, buffer, 16, BLACK);  // +4 offset to align with time
    
    yPos += rowHeight;
  }
  
  // Display message if no departures available for either station
  if (numStation1Departures == 0 && numStation2Departures == 0) {
    EPD_ShowStringUTF8(100, 100, "No departures", 24, BLACK);
    EPD_ShowStringUTF8(100, 130, "available", 24, BLACK);
  }

  // Draw screen indicators
  extern void drawScreenIndicators(int activeScreen);
  drawScreenIndicators(1);

  // Update the e-ink display content with a single refresh
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

#endif
