#ifndef SPOTIFY_SCREEN_H
#define SPOTIFY_SCREEN_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "credentials.h"

// Spotify API credentials
String spotifyClientId = SPOTIFY_CLIENT_ID;
String spotifyClientSecret = SPOTIFY_CLIENT_SECRET;
String spotifyRefreshToken = SPOTIFY_REFRESH_TOKEN;
String spotifyAccessToken = "";

// Spotify playback data
String spotifySongName = "No song playing";
String spotifyArtistName = "";
String spotifyAlbumName = "";
String spotifyAlbumArtUrl = "";  // URL to album artwork
String spotifyReleaseYear = "";
bool spotifyIsPlaying = false;

// Track last displayed Spotify data to avoid unnecessary repaints
String lastDisplayedSong = "";
String lastDisplayedArtist = "";
String lastDisplayedAlbum = "";
bool lastDisplayedIsPlaying = false;

// Function to fetch Spotify access token using refresh token
void fetch_spotify_token(int& httpResponseCode)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected - cannot fetch Spotify token");
    return;
  }
  
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification (for simplicity)
  HTTPClient http;
  
  // Spotify token endpoint
  http.begin(client, "https://accounts.spotify.com/api/token");
  
  // Set headers
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Prepare POST data with refresh token
  String postData = "grant_type=refresh_token&refresh_token=" + spotifyRefreshToken + 
                    "&client_id=" + spotifyClientId + 
                    "&client_secret=" + spotifyClientSecret;
  
  // Send POST request
  httpResponseCode = http.POST(postData);
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("Spotify token refreshed successfully");
    
    // Parse the JSON response to get the access token
    JSONVar tokenObject = JSON.parse(payload);
    
    if (JSON.typeof(tokenObject) == "undefined") {
      Serial.println("Failed to parse Spotify token response");
    } else {
      String token = JSON.stringify(tokenObject["access_token"]);
      token.replace("\"", ""); // Remove quotes
      spotifyAccessToken = token;
      Serial.print("Spotify access token obtained: ");
      Serial.println(spotifyAccessToken.substring(0, 20) + "...");
    }
  } else {
    Serial.print("Failed to refresh Spotify token. HTTP code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  
  http.end();
}

// Function to fetch currently playing song from Spotify
void fetch_spotify_current_song(int& httpResponseCode)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected - cannot fetch Spotify data");
    return;
  }
  
  if (spotifyAccessToken.length() == 0) {
    Serial.println("No Spotify access token available");
    spotifyIsPlaying = false;
    return;
  }
  
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification (for simplicity)
  HTTPClient http;
  
  // Spotify currently playing endpoint
  http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing");
  
  // Set authorization header
  http.addHeader("Authorization", "Bearer " + spotifyAccessToken);
  
  // Send GET request
  httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("Spotify currently playing response received");
    
    // Parse the JSON response
    JSONVar songObject = JSON.parse(payload);
    
    if (JSON.typeof(songObject) == "undefined") {
      Serial.println("Failed to parse Spotify song response");
      spotifyIsPlaying = false;
    } else {
      spotifyIsPlaying = true;
      
      // Get song name
      String songName = JSON.stringify(songObject["item"]["name"]);
      songName.replace("\"", "");
      spotifySongName = songName;
      
      // Get artist name (first artist)
      if (songObject["item"]["artists"].length() > 0) {
        String artistName = JSON.stringify(songObject["item"]["artists"][0]["name"]);
        artistName.replace("\"", "");
        spotifyArtistName = artistName;
      }
      
      // Get album name
      String albumName = JSON.stringify(songObject["item"]["album"]["name"]);
      albumName.replace("\"", "");
      spotifyAlbumName = albumName;
      
      // Get album artwork URL (640x640 version)
      if (songObject["item"]["album"]["images"].length() > 0) {
        String artUrl = JSON.stringify(songObject["item"]["album"]["images"][0]["url"]);
        artUrl.replace("\"", "");
        spotifyAlbumArtUrl = artUrl;
        Serial.print("Album art URL: ");
        Serial.println(spotifyAlbumArtUrl);
      } else {
        spotifyAlbumArtUrl = "";
      }
      
      // Get release year (extract year from release_date)
      String releaseDate = JSON.stringify(songObject["item"]["album"]["release_date"]);
      releaseDate.replace("\"", "");
      if (releaseDate.length() >= 4) {
        spotifyReleaseYear = releaseDate.substring(0, 4);
      } else {
        spotifyReleaseYear = "";
      }
      
      Serial.print("Now playing: ");
      Serial.print(spotifySongName);
      Serial.print(" by ");
      Serial.println(spotifyArtistName);
    }
  } else if (httpResponseCode == 204) {
    // 204 means no content - nothing is currently playing
    Serial.println("Nothing currently playing on Spotify");
    spotifyIsPlaying = false;
    spotifySongName = "No song playing";
    spotifyArtistName = "";
    spotifyAlbumName = "";
  } else if (httpResponseCode == 401) {
    // Unauthorized - token might be expired, try to get a new one
    Serial.println("Spotify token expired or invalid, fetching new token");
    fetch_spotify_token(httpResponseCode);
  } else {
    Serial.print("Failed to get Spotify data. HTTP code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
    spotifyIsPlaying = false;
  }
  
  http.end();
}

// Check if Spotify data has changed
bool spotify_data_changed() {
  return (spotifySongName != lastDisplayedSong || 
          spotifyArtistName != lastDisplayedArtist || 
          spotifyAlbumName != lastDisplayedAlbum ||
          spotifyIsPlaying != lastDisplayedIsPlaying);
}

// Display Screen 3 - Spotify Now Playing
void display_spotify_screen(uint8_t* ImageBW, bool& forceFullRefresh)
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

  if (spotifyIsPlaying) {
    // Display text with left padding
    int textX = 20;
    int yPos = 90;  // Moved up from 100
    
    // Display song name
    // Display song name
    memset(buffer, 0, sizeof(buffer));
    if (spotifySongName.length() > 40) {
      spotifySongName.substring(0, 37).toCharArray(buffer, sizeof(buffer));
      strcat(buffer, "...");
    } else {
      spotifySongName.toCharArray(buffer, sizeof(buffer));
    }
    EPD_ShowStringUTF8(textX, yPos, buffer, 24, BLACK);
    
    // Display artist name
    yPos += 40;
    memset(buffer, 0, sizeof(buffer));
    if (spotifyArtistName.length() > 40) {
      spotifyArtistName.substring(0, 37).toCharArray(buffer, sizeof(buffer));
      strcat(buffer, "...");
    } else {
      spotifyArtistName.toCharArray(buffer, sizeof(buffer));
    }
    EPD_ShowStringUTF8(textX, yPos, buffer, 16, BLACK);
    
    // Display album name
    if (spotifyAlbumName.length() > 0) {
      yPos += 30;
      memset(buffer, 0, sizeof(buffer));
      if (spotifyAlbumName.length() > 40) {
        spotifyAlbumName.substring(0, 37).toCharArray(buffer, sizeof(buffer));
        strcat(buffer, "...");
      } else {
        spotifyAlbumName.toCharArray(buffer, sizeof(buffer));
      }
      EPD_ShowStringUTF8(textX, yPos, buffer, 16, BLACK);
    }
    
    // Display release year
    if (spotifyReleaseYear.length() > 0) {
      yPos += 30;
      memset(buffer, 0, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), "%s", spotifyReleaseYear.c_str());
      EPD_ShowStringUTF8(textX, yPos, buffer, 16, BLACK);
    }
  } else {
    // Display "Not Playing" message (centered)
    int centerX = EPD_W / 2;
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Nothing playing");
    int textWidth = EPD_GetUTF8TextWidth(buffer, 24);
    EPD_ShowStringUTF8(centerX - textWidth / 2, 120, buffer, 24, BLACK);
  }

  // Draw screen indicators
  extern void drawScreenIndicators(int activeScreen);
  drawScreenIndicators(2);

  // Update the e-ink display content with a single refresh
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  
  // Update the last displayed values
  lastDisplayedSong = spotifySongName;
  lastDisplayedArtist = spotifyArtistName;
  lastDisplayedAlbum = spotifyAlbumName;
  lastDisplayedIsPlaying = spotifyIsPlaying;
}

#endif
