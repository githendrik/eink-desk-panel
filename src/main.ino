#include <WiFi.h>
#include <HTTPClient.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "pic.h"
#include "credentials.h"

// Include screen modules
#include "weather_screen.h"
#include "transport_screen.h"
#include "spotify_screen.h"

// Define a black and white image array as the buffer for the e-paper display
uint8_t ImageBW[15000];

// Button pin definitions
#define PRV_KEY 6    // Previous/Up button
#define NEXT_KEY 4   // Next/Down button

// Screen state
int currentScreen = 0;  // Start on weather screen (0=weather, 1=transport, 2=spotify)
const int NUM_SCREENS = 3;
bool screenNeedsUpdate = true;
bool forceFullRefresh = true;  // Start with true for initial boot

// Button debouncing
volatile bool prvButtonPressed = false;
volatile bool nextButtonPressed = false;
volatile unsigned long lastPrvPress = 0;
volatile unsigned long lastNextPress = 0;
const unsigned long DEBOUNCE_DELAY = 200; // 200ms debounce time

// ISR for Previous button
void IRAM_ATTR handlePrvButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastPrvPress > DEBOUNCE_DELAY) {
    prvButtonPressed = true;
    lastPrvPress = currentTime;
  }
}

// ISR for Next button
void IRAM_ATTR handleNextButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastNextPress > DEBOUNCE_DELAY) {
    nextButtonPressed = true;
    lastNextPress = currentTime;
  }
}

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// HTTP response code (shared across all screens)
int httpResponseCode;

// Draw screen indicator circles on the right side
void drawScreenIndicators(int activeScreen)
{
  int circleX = EPD_W - 30;
  int startY = (EPD_H / 2) - 15;
  int radius = 5;
  int spacing = 20;
  
  for (int i = 0; i < NUM_SCREENS; i++) {
    int circleY = startY + (i * spacing);
    if (i == activeScreen) {
      EPD_DrawCircle(circleX, circleY, radius, BLACK, 1);  // Filled
    } else {
      EPD_DrawCircle(circleX, circleY, radius, BLACK, 0);  // Outline
    }
  }
}

// HTTP GET request helper function
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);
  httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
  return payload;
}

// Unified function to update the display based on current screen
void updateDisplay()
{
  switch (currentScreen) {
    case 0:
      display_weather_screen(ImageBW, forceFullRefresh);
      break;
    case 1:
      display_transport_screen(ImageBW, forceFullRefresh);
      break;
    case 2:
      display_spotify_screen(ImageBW, forceFullRefresh);
      break;
  }
  screenNeedsUpdate = false;
}

void setup() {
  Serial.begin(115200);

  // Set button pins as input with pull-up resistors
  pinMode(PRV_KEY, INPUT_PULLUP);
  pinMode(NEXT_KEY, INPUT_PULLUP);
  
  // Attach interrupts for button handling (trigger on falling edge - button press)
  attachInterrupt(digitalPinToInterrupt(PRV_KEY), handlePrvButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(NEXT_KEY), handleNextButton, FALLING);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
  
  // Set up display power
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Initialize the e-ink display
  EPD_GPIOInit();

  // Fetch initial data for all screens
  fetch_weather_data(httpResponseCode, httpGETRequest);
  fetch_transport_data(httpResponseCode, httpGETRequest);
  fetch_spotify_token(httpResponseCode);
  
  // Display initial screen
  updateDisplay();
}

void loop() {
  static unsigned long lastWeatherFetch = 0;
  static unsigned long lastTransportFetch = 0;
  static unsigned long lastSpotifyFetch = 0;
  static unsigned long lastSpotifyTokenFetch = 0;
  
  // Check for button presses using interrupt flags
  if (prvButtonPressed) {
    prvButtonPressed = false;  // Clear flag
    currentScreen--;
    if (currentScreen < 0) {
      currentScreen = NUM_SCREENS - 1;
    }
    Serial.print("Screen changed to: ");
    Serial.println(currentScreen);
    forceFullRefresh = true;
    screenNeedsUpdate = true;
  }
  
  if (nextButtonPressed) {
    nextButtonPressed = false;  // Clear flag
    currentScreen++;
    if (currentScreen >= NUM_SCREENS) {
      currentScreen = 0;
    }
    Serial.print("Screen changed to: ");
    Serial.println(currentScreen);
    forceFullRefresh = true;
    screenNeedsUpdate = true;
  }
  
  // Update display if screen changed
  if (screenNeedsUpdate) {
    updateDisplay();
  }
  
  // Fetch weather data every hour
  if (millis() - lastWeatherFetch >= 1000*60*60) {
    Serial.println("Fetching weather data...");
    fetch_weather_data(httpResponseCode, httpGETRequest);
    lastWeatherFetch = millis();
    
    if (currentScreen == 0) {
      screenNeedsUpdate = true;
    }
  }
  
  // Fetch transport data every minute only when on transport screen
  if (currentScreen == 1) {
    if (millis() - lastTransportFetch >= 1000*60) {
      Serial.println("Fetching transport data...");
      fetch_transport_data(httpResponseCode, httpGETRequest);
      lastTransportFetch = millis();
      screenNeedsUpdate = true;
    }
  }
  
  // Fetch Spotify token every 50 minutes
  if (millis() - lastSpotifyTokenFetch >= 1000*60*50) {
    Serial.println("Fetching Spotify token...");
    fetch_spotify_token(httpResponseCode);
    lastSpotifyTokenFetch = millis();
  }
  
  // Fetch Spotify currently playing only when on Spotify screen, every 10 seconds
  if (currentScreen == 2) {
    if (millis() - lastSpotifyFetch >= 1000*10) {
      Serial.println("Fetching Spotify current song...");
      fetch_spotify_current_song(httpResponseCode);
      lastSpotifyFetch = millis();
      
      // Only update screen if the song data has actually changed
      if (spotify_data_changed()) {
        screenNeedsUpdate = true;
        Serial.println("Song changed, updating display");
      }
    }
  }
  
  // Small delay to prevent excessive CPU usage (reduced from 50ms to 10ms)
  delay(10);
}
