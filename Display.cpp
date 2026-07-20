#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_arduino_version.h>

#include "LGFX_Config.h"

struct AppSettings {
  String city;
  String timezone;
  float latitude;
  float longitude;
  bool use24Hour;
  uint8_t brightness;
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

extern LGFX tft;
extern AppSettings settings;
extern WeatherData weather;

static String lastDrawnTime = "";
static String lastDrawnDate = "";
static String lastDrawnWeather = "";
static String lastDrawnIp = "";
static bool brightnessReady = false;

void applyBrightness() {
  uint8_t duty = constrain(settings.brightness, 20, 255);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // ESP32 Arduino Core 3.x LEDC API
  if (!brightnessReady) {
    ledcAttach(TFT_BL_PIN, 5000, 8);
    brightnessReady = true;
  }
  ledcWrite(TFT_BL_PIN, duty);
#else
  // ESP32 Arduino Core 2.x LEDC API
  const int backlightChannel = 0;
  if (!brightnessReady) {
    ledcSetup(backlightChannel, 5000, 8);
    ledcAttachPin(TFT_BL_PIN, backlightChannel);
    brightnessReady = true;
  }
  ledcWrite(backlightChannel, duty);
#endif
}

void drawBootScreen(const String& message) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(middle_center);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("ESP32 Clock", 160, 88, &fonts::Font4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(message, 160, 130, &fonts::Font2);
}

void drawStatusScreen(const String& line1, const String& line2) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(middle_center);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(line1, 160, 100, &fonts::Font4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(line2, 160, 140, &fonts::Font2);
}

static String formatTime(const struct tm& timeinfo) {
  char buf[16];

  if (settings.use24Hour) {
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  } else {
    strftime(buf, sizeof(buf), "%I:%M:%S", &timeinfo);
    if (buf[0] == '0') {
      memmove(buf, buf + 1, strlen(buf));
    }
  }

  return String(buf);
}

static String formatDate(const struct tm& timeinfo) {
  char buf[40];
  strftime(buf, sizeof(buf), "%A %d %b %Y", &timeinfo);
  return String(buf);
}

void drawClockScreen() {
  struct tm timeinfo;
  bool gotTime = getLocalTime(&timeinfo, 50);

  String timeLine = gotTime ? formatTime(timeinfo) : "--:--:--";
  String dateLine = gotTime ? formatDate(timeinfo) : "Waiting for NTP";
  String ipLine = WiFi.isConnected() ? WiFi.localIP().toString() : "WiFi offline";

  String weatherLine;
  if (weather.valid) {
    weatherLine = weather.summary + "  " +
                  String(weather.tempMin, 1) + "-" + String(weather.tempMax, 1) + "C  " +
                  "Rain " + String(weather.precipProb) + "%";
  } else {
    weatherLine = "Weather loading...";
  }

  bool fullRedraw = false;
  if (lastDrawnDate != dateLine || lastDrawnWeather != weatherLine || lastDrawnIp != ipLine) {
    fullRedraw = true;
  }

  if (fullRedraw) {
    tft.fillScreen(TFT_BLACK);

    tft.drawRoundRect(6, 6, 308, 116, 10, TFT_DARKGREY);
    tft.drawRoundRect(6, 130, 308, 82, 10, TFT_DARKGREY);

    tft.setTextDatum(middle_center);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(dateLine, 160, 104, &fonts::Font2);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(settings.city, 160, 146, &fonts::Font4);

    tft.setTextColor(weather.valid ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
    tft.drawString(weatherLine, 160, 180, &fonts::Font2);

    if (weather.valid) {
      String rainLine = "Today rain: " + String(weather.rainSum, 1) + " mm  Updated: " + weather.updatedAt;
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      tft.drawString(rainLine, 160, 200, &fonts::Font2);
    }

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(ipLine, 160, 228, &fonts::Font2);
  }

  if (lastDrawnTime != timeLine || fullRedraw) {
    tft.fillRect(15, 30, 290, 58, TFT_BLACK);
    tft.setTextDatum(middle_center);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);

    // Font7 gives large clock digits similar to the TFT_eSPI version.
    tft.drawString(timeLine, 160, 62, &fonts::Font7);
  }

  lastDrawnTime = timeLine;
  lastDrawnDate = dateLine;
  lastDrawnWeather = weatherLine;
  lastDrawnIp = ipLine;
}
