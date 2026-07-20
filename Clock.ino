#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>

#include "LGFX_Config.h"

// Project files:
//   Clock.ino       - main application, Wi-Fi bootstrap, NTP, scheduler
//   LGFX_Config.h   - LovyanGFX setup for E32R28T-1 ST7789 display
//   Display.cpp     - all TFT drawing using LovyanGFX, including weather icons
//   Weather.cpp     - Open-Meteo fetch and weather decoding
//   WebConfig.cpp   - configuration web interface
//
// Arduino IDE libraries required:
//   LovyanGFX
//   WiFiManager
//   ArduinoJson
//
// Recommended Arduino board:
//   ESP32 Dev Module

struct AppSettings {
  String city;
  String timezone;
  float latitude;
  float longitude;
  bool use24Hour;
  uint8_t brightness;   // 20-255 PWM duty
  uint8_t rotation;     // 0, 1, 2, or 3
};

struct WeatherData {
  bool valid;
  String summary;
  int weatherCode;
  float tempMax;
  float tempMin;
  float rainSum;
  int precipProb;
  String updatedAt;
};

LGFX tft;
WebServer server(80);
Preferences prefs;

AppSettings settings;
WeatherData weather;

const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.google.com";

const unsigned long SCREEN_INTERVAL_MS = 1000UL;
const unsigned long WEATHER_INTERVAL_MS = 60UL * 60UL * 1000UL;
const unsigned long NTP_RETRY_INTERVAL_MS = 10UL * 60UL * 1000UL;

unsigned long lastScreenUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastNtpRetry = 0;

// Functions implemented in other project files
void drawBootScreen(const String& message);
void drawClockScreen();
void drawStatusScreen(const String& line1, const String& line2);
void applyBrightness();
void invalidateDisplayCache();
bool fetchWeather();
void setupWebServer();

void applyRotation() {
  settings.rotation = constrain(settings.rotation, 0, 3);
  tft.setRotation(settings.rotation);
  invalidateDisplayCache();
}

void loadSettings() {
  prefs.begin("clock", true);

  // Defaults: Christchurch, New Zealand mainland timezone with automatic DST.
  settings.city = prefs.getString("city", "Christchurch");
  settings.timezone = prefs.getString("tz", "NZST-12NZDT,M9.5.0,M4.1.0/3");
  settings.latitude = prefs.getFloat("lat", -43.5321f);
  settings.longitude = prefs.getFloat("lon", 172.6362f);
  settings.use24Hour = prefs.getBool("h24", true);
  settings.brightness = prefs.getUChar("bright", 220);
  settings.rotation = prefs.getUChar("rot", 1); // Landscape default

  prefs.end();
}

void saveSettings() {
  prefs.begin("clock", false);

  prefs.putString("city", settings.city);
  prefs.putString("tz", settings.timezone);
  prefs.putFloat("lat", settings.latitude);
  prefs.putFloat("lon", settings.longitude);
  prefs.putBool("h24", settings.use24Hour);
  prefs.putUChar("bright", settings.brightness);
  prefs.putUChar("rot", settings.rotation);

  prefs.end();
}

bool hasValidTime() {
  struct tm timeinfo;
  return getLocalTime(&timeinfo, 50);
}

void setupTime() {
  setenv("TZ", settings.timezone.c_str(), 1);
  tzset();

  configTzTime(settings.timezone.c_str(), NTP_SERVER_1, NTP_SERVER_2);

  Serial.println("Waiting for NTP time sync...");
  for (int i = 0; i < 30; i++) {
    if (hasValidTime()) {
      Serial.println("NTP time synced.");
      return;
    }
    delay(500);
  }

  Serial.println("NTP sync not available yet; will retry later.");
}

void startWiFi() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  wm.setConfigPortalTimeout(240);
  wm.setConnectTimeout(30);

  drawStatusScreen("WiFi setup", "Connect to ClockSetup");

  bool connected = wm.autoConnect("ClockSetup");
  if (!connected) {
    drawStatusScreen("WiFi failed", "Restarting...");
    delay(2500);
    ESP.restart();
  }

  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  loadSettings();

  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  tft.init();
  tft.setColorDepth(16);
  applyRotation();
  tft.fillScreen(TFT_BLACK);

  applyBrightness();

  drawBootScreen("Starting clock...");

  weather.valid = false;
  weather.summary = "Loading";
  weather.weatherCode = -1;
  weather.tempMax = 0;
  weather.tempMin = 0;
  weather.rainSum = 0;
  weather.precipProb = 0;
  weather.updatedAt = "Never";

  startWiFi();
  setupTime();
  fetchWeather();
  lastWeatherUpdate = millis();

  setupWebServer();
  drawClockScreen();
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (!hasValidTime() && now - lastNtpRetry >= NTP_RETRY_INTERVAL_MS) {
    lastNtpRetry = now;
    setupTime();
  }

  if (now - lastWeatherUpdate >= WEATHER_INTERVAL_MS) {
    lastWeatherUpdate = now;
    fetchWeather();
  }

  if (now - lastScreenUpdate >= SCREEN_INTERVAL_MS) {
    lastScreenUpdate = now;
    drawClockScreen();
  }
}
