#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
//#include <ArduinoOTA.h>
#include "LGFX_Config.h"

// Project files:
//   Clock.ino       - main application, Wi-Fi bootstrap, NTP, scheduler
//   LGFX_Config.h   - LovyanGFX setup for E32R28T-1 ST7789 display
//   Display.cpp     - all TFT drawing, weather icons, dimming brightness logic
//   Weather.cpp     - Open-Meteo fetch and weather decoding
//   WebConfig.cpp   - configuration web interface
  
								  
			  
				
				
  
							 
					 

struct AppSettings {
  String city;
  String timezone;
  float latitude;
  float longitude;
  bool use24Hour;
  uint8_t brightness;       // Daytime/manual brightness, 20-255
  uint8_t rotation;         // 0, 1, 2, or 3
  bool autoDim;             // Dim display automatically at night
  uint8_t nightBrightness;  // Night brightness, 1-255
  uint8_t dimStartHour;     // 0-23, local time
  uint8_t dimEndHour;       // 0-23, local time
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
  float tempCurrent;
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
const unsigned long BRIGHTNESS_CHECK_INTERVAL_MS = 60UL * 1000UL;

//To  be removed
const char* OTA_HOSTNAME = "ESP32-Clock";
const char* OTA_PASSWORD = "";   // Optional. Leave bl

unsigned long lastScreenUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastNtpRetry = 0;
unsigned long lastBrightnessCheck = 0;

bool showWeather = true;
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE_MS = 300;

// Functions implemented in other project files
void drawBootScreen(const String& message);
void drawClockScreen();
void drawStatusScreen(const String& line1, const String& line2);
void applyBrightness();
void invalidateDisplayCache();
bool fetchWeather();
void setupWebServer();

void handleScreenTouch();

void applyRotation() {
  settings.rotation = constrain(settings.rotation, 0, 3);
  tft.setRotation(settings.rotation);
  invalidateDisplayCache();
}

void loadSettings() {
  prefs.begin("clock", true);

  // Defaults: Hokitika/NZ-compatible timezone, with automatic daylight saving.
  settings.city = prefs.getString("city", "Hokitika");
  settings.timezone = prefs.getString("tz", "NZST-12NZDT,M9.5.0,M4.1.0/3");
  settings.latitude = prefs.getFloat("lat", -42.7167f);
  settings.longitude = prefs.getFloat("lon", 170.9667f);
  settings.use24Hour = prefs.getBool("h24", true);
  settings.brightness = prefs.getUChar("bright", 220);
  settings.rotation = prefs.getUChar("rot", 1);

  // Night dimming defaults: dim from 22:00 to 07:00.
  settings.autoDim = prefs.getBool("autodim", true);
  settings.nightBrightness = prefs.getUChar("nbright", 35);
  settings.dimStartHour = prefs.getUChar("dimstart", 22);
  settings.dimEndHour = prefs.getUChar("dimend", 7);

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
  prefs.putBool("autodim", settings.autoDim);
  prefs.putUChar("nbright", settings.nightBrightness);
  prefs.putUChar("dimstart", settings.dimStartHour);
  prefs.putUChar("dimend", settings.dimEndHour);

  prefs.end();
}

bool hasValidTime() {
  struct tm timeinfo;
  return getLocalTime(&timeinfo, 50);
}


// void setupOTA() {
//   ArduinoOTA.setHostname(OTA_HOSTNAME);

//   if (strlen(OTA_PASSWORD) > 0) {
//     ArduinoOTA.setPassword(OTA_PASSWORD);
//   }

//   ArduinoOTA.onStart([](){
//     drawStatusScreen("OTA Update", "Starting...");
//     Serial.println("OTA started");
//   });

//   ArduinoOTA.onEnd([](){
//     Serial.println("\nOTA complete");
//   });

//   ArduinoOTA.onProgress([](unsigned int progress,unsigned int total) {
//     static unsigned long lastUpdate = 0;

//     if (millis() - lastUpdate > 500) {
//       lastUpdate = millis();

//       int percent = (progress * 100) / total;

//       tft.fillRect(20, 150, 280, 30, TFT_BLACK);
//       tft.setTextColor(TFT_GREEN, TFT_BLACK);
//       tft.setTextDatum(middle_center);
//       tft.drawString(
//         String("Updating ") + String(percent) + "%",
//         160, 165,
//         &fonts::Font2
//       );
//       Serial.printf("OTA Progress: %u%%\r", percent);
//     }

    
//   });

//   ArduinoOTA.onError([](ota_error_t error) {
//     Serial.printf("OTA Error[%u]\n", error);

//     String message = "Error";

//     if (error == OTA_AUTH_ERROR) message = "Auth failed";
//     else if (error == OTA_BEGIN_ERROR) message = "Begin failed";
//     else if (error == OTA_CONNECT_ERROR) message = "Connect failed";
//     else if (error == OTA_RECEIVE_ERROR) message = "Receive failed";
//     else if (error == OTA_END_ERROR) message = "End failed";

//     drawStatusScreen("OTA Failed", message);
//   });

//   ArduinoOTA.begin();

//   Serial.print("OTA Ready: ");
//   Serial.println(WiFi.localIP());
// }


void setupTime() {
  setenv("TZ", settings.timezone.c_str(), 1);
  tzset();

  configTzTime(settings.timezone.c_str(), NTP_SERVER_1, NTP_SERVER_2);

  Serial.println("Waiting for NTP time sync...");
  for (int i = 0; i < 30; i++) {
    if (hasValidTime()) {
      Serial.println("NTP time synced.");
      applyBrightness();
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


bool otaAvailable() {
    return WiFi.status() == WL_CONNECTED &&
           !(WiFi.getMode() & WIFI_AP);
}


void handleScreenTouch(){
  if (tft.getTouch(nullptr)){
    unsigned long now = millis();
    if (now - lastTouchTime > TOUCH_DEBOUNCE_MS) {
      lastTouchTime = now;
      showWeather = !showWeather;
      invalidateDisplayCache(); //Force full redraw
      Serial.println(showWeather ? "Switch to weather view" : "Switch to time-only view");
    }
  }
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

  

  // if (otaAvailable()) {
  //   	setupOTA();
  // }

	
  setupTime();
  fetchWeather();
  lastWeatherUpdate = millis();

  setupWebServer();

  tft.setTouchCalibrate(nullptr);

  drawClockScreen();
}

void loop() {

  
	// if (otaAvailable()) {
  //   	ArduinoOTA.handle();
	// }

	
  server.handleClient();

  handleScreenTouch();

  unsigned long now = millis();

  if (!hasValidTime() && now - lastNtpRetry >= NTP_RETRY_INTERVAL_MS) {
    lastNtpRetry = now;
    setupTime();
  }

  if (now - lastBrightnessCheck >= BRIGHTNESS_CHECK_INTERVAL_MS) {
    lastBrightnessCheck = now;
    applyBrightness();
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
