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
  bool use24Hour;#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_arduino_version.h>

#include "LGFX_Config.h"

constexpr float ICON_SCALE = 0.75f;

struct AppSettings {
  String city;
  String timezone;
  float latitude;
  float longitude;
  bool use24Hour;
  uint8_t brightness;
  uint8_t rotation;
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
static int lastDrawnWeatherCode = -999;
static bool brightnessReady = false;

void invalidateDisplayCache() {
  lastDrawnTime = "";
  lastDrawnDate = "";
  lastDrawnWeather = "";
  lastDrawnIp = "";
  lastDrawnWeatherCode = -999;
}

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

static void drawSun(int x, int y, int r) {
  r = int(r * ICON_SCALE);

  tft.fillCircle(x, y, r, TFT_YELLOW);

  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4.0f;
    int x1 = x + cos(a) * (r + 3);
    int y1 = y + sin(a) * (r + 3);
    int x2 = x + cos(a) * (r + 8);
    int y2 = y + sin(a) * (r + 8);
    tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
  }
}

static void drawCloud(int x, int y, uint16_t color) {
  tft.fillCircle(x - 11, y + 3, 9, color);
  tft.fillCircle(x, y - 3, 12, color);
  tft.fillCircle(x + 14, y + 4, 10, color);
  tft.fillRoundRect(x - 21, y + 4, 44, 14, 6, color);
}

static void drawRainDrops(int x, int y, uint16_t color) {
  for (int i = 0; i < 4; i++) {
    int dx = x - 15 + i * 10;
    tft.drawLine(dx, y, dx - 3, y + 9, color);
    tft.drawLine(dx + 1, y, dx - 2, y + 9, color);
  }
}

static void drawSnowFlakes(int x, int y, uint16_t color) {
  for (int i = 0; i < 3; i++) {
    int cx = x - 12 + i * 12;
    int cy = y + (i % 2) * 5;
    tft.drawLine(cx - 4, cy, cx + 4, cy, color);
    tft.drawLine(cx, cy - 4, cx, cy + 4, color);
    tft.drawLine(cx - 3, cy - 3, cx + 3, cy + 3, color);
    tft.drawLine(cx - 3, cy + 3, cx + 3, cy - 3, color);
  }
}

static void drawLightning(int x, int y) {
  tft.fillTriangle(x, y, x - 5, y + 15, x + 2, y + 15, TFT_YELLOW);
  tft.fillTriangle(x + 2, y + 12, x - 3, y + 29, x + 9, y + 9, TFT_YELLOW);
}

static void drawFog(int x, int y) {
  for (int i = 0; i < 4; i++) {
    int yy = y + i * 7;
    tft.drawLine(x - 23, yy, x + 23, yy, TFT_LIGHTGREY);
    tft.drawLine(x - 18, yy + 2, x + 18, yy + 2, TFT_DARKGREY);
  }
}

static void drawWeatherIcon(int code, int x, int y) {
  // Clear icon background area.
  tft.fillRoundRect(x - 32, y - 26, 64, 54, 8, TFT_BLACK);

  if (code == 0) {
    drawSun(x, y, 17);
  } else if (code == 1 || code == 2) {
    drawSun(x - 11, y - 8, 12);
    drawCloud(x + 6, y + 4, TFT_LIGHTGREY);
  } else if (code == 3) {
    drawCloud(x, y, TFT_LIGHTGREY);
    drawCloud(x + 6, y + 9, TFT_DARKGREY);
  } else if (code == 45 || code == 48) {
    drawFog(x, y - 12);
  } else if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67) || (code >= 80 && code <= 82)) {
    drawCloud(x, y - 6, TFT_LIGHTGREY);
    drawRainDrops(x, y + 15, TFT_CYAN);
  } else if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
    drawCloud(x, y - 6, TFT_LIGHTGREY);
    drawSnowFlakes(x, y + 17, TFT_WHITE);
  } else if (code == 95 || code == 96 || code == 99) {
    drawCloud(x, y - 8, TFT_DARKGREY);
    drawLightning(x + 2, y + 6);
    drawRainDrops(x - 5, y + 17, TFT_CYAN);
  } else {
    tft.drawCircle(x, y, 18, TFT_ORANGE);
    tft.setTextDatum(middle_center);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString("?", x, y, &fonts::Font4);
  }
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
  if (lastDrawnDate != dateLine || lastDrawnWeather != weatherLine ||
      lastDrawnIp != ipLine || lastDrawnWeatherCode != weather.weatherCode) {
    fullRedraw = true;
  }

  if (fullRedraw) {
    tft.fillScreen(TFT_BLACK);

    tft.drawRoundRect(6, 6, 308, 116, 10, TFT_DARKGREY);
    tft.drawRoundRect(6, 130, 308, 82, 10, TFT_DARKGREY);

    tft.setTextDatum(middle_center);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(dateLine, 160, 104, &fonts::Font2);

    // Weather icon on the left, text on the right.
    int iconCode = weather.valid ? weather.weatherCode : -1;
    drawWeatherIcon(iconCode, 38, 171);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(settings.city, 175, 146, &fonts::Font4);

    tft.setTextColor(weather.valid ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
    tft.drawString(weatherLine, 175, 180, &fonts::Font2);

    if (weather.valid) {
      String rainLine = "Today rain: " + String(weather.rainSum, 1) + " mm  Updated: " + weather.updatedAt;
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      tft.drawString(rainLine, 175, 200, &fonts::Font2);
    }

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(ipLine + "  Rot " + String(settings.rotation), 160, 228, &fonts::Font2);
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
  lastDrawnWeatherCode = weather.weatherCode;
}

  uint8_t brightness;
  uint8_t rotation;
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
static int lastDrawnWeatherCode = -999;
static bool brightnessReady = false;

void invalidateDisplayCache() {
  lastDrawnTime = "";
  lastDrawnDate = "";
  lastDrawnWeather = "";
  lastDrawnIp = "";
  lastDrawnWeatherCode = -999;
}

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

static void drawSun(int x, int y, int r) {
  tft.fillCircle(x, y, r, TFT_YELLOW);
  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4.0f;
    int x1 = x + cos(a) * (r + 4);
    int y1 = y + sin(a) * (r + 4);
    int x2 = x + cos(a) * (r + 11);
    int y2 = y + sin(a) * (r + 11);
    tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
  }
}

static void drawCloud(int x, int y, uint16_t color) {
  tft.fillCircle(x - 14, y + 4, 12, color);
  tft.fillCircle(x, y - 4, 16, color);
  tft.fillCircle(x + 18, y + 5, 13, color);
  tft.fillRoundRect(x - 28, y + 5, 58, 18, 8, color);
}

static void drawRainDrops(int x, int y, uint16_t color) {
  for (int i = 0; i < 4; i++) {
    int dx = x - 20 + i * 14;
    tft.drawLine(dx, y, dx - 4, y + 12, color);
    tft.drawLine(dx + 1, y, dx - 3, y + 12, color);
  }
}

static void drawSnowFlakes(int x, int y, uint16_t color) {
  for (int i = 0; i < 3; i++) {
    int cx = x - 16 + i * 16;
    int cy = y + (i % 2) * 6;
    tft.drawLine(cx - 5, cy, cx + 5, cy, color);
    tft.drawLine(cx, cy - 5, cx, cy + 5, color);
    tft.drawLine(cx - 4, cy - 4, cx + 4, cy + 4, color);
    tft.drawLine(cx - 4, cy + 4, cx + 4, cy - 4, color);
  }
}

static void drawLightning(int x, int y) {
  tft.fillTriangle(x, y, x - 7, y + 20, x + 2, y + 20, TFT_YELLOW);
  tft.fillTriangle(x + 2, y + 16, x - 4, y + 38, x + 12, y + 12, TFT_YELLOW);
}

static void drawFog(int x, int y) {
  for (int i = 0; i < 4; i++) {
    int yy = y + i * 9;
    tft.drawLine(x - 30, yy, x + 30, yy, TFT_LIGHTGREY);
    tft.drawLine(x - 24, yy + 3, x + 24, yy + 3, TFT_DARKGREY);
  }
}

static void drawWeatherIcon(int code, int x, int y) {
  // Clear icon background area.
  tft.fillRoundRect(x - 42, y - 34, 84, 72, 10, TFT_BLACK);

  if (code == 0) {
    drawSun(x, y, 17);
  } else if (code == 1 || code == 2) {
    drawSun(x - 15, y - 10, 12);
    drawCloud(x + 8, y + 5, TFT_LIGHTGREY);
  } else if (code == 3) {
    drawCloud(x, y, TFT_LIGHTGREY);
    drawCloud(x + 8, y + 12, TFT_DARKGREY);
  } else if (code == 45 || code == 48) {
    drawFog(x, y - 12);
  } else if ((code >= 51 && code <= 57) || (code >= 61 && code <= 67) || (code >= 80 && code <= 82)) {
    drawCloud(x, y - 8, TFT_LIGHTGREY);
    drawRainDrops(x, y + 20, TFT_CYAN);
  } else if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
    drawCloud(x, y - 8, TFT_LIGHTGREY);
    drawSnowFlakes(x, y + 22, TFT_WHITE);
  } else if (code == 95 || code == 96 || code == 99) {
    drawCloud(x, y - 10, TFT_DARKGREY);
    drawLightning(x + 2, y + 8);
    drawRainDrops(x - 6, y + 22, TFT_CYAN);
  } else {
    tft.drawCircle(x, y, 24, TFT_ORANGE);
    tft.setTextDatum(middle_center);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString("?", x, y, &fonts::Font4);
  }
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
  if (lastDrawnDate != dateLine || lastDrawnWeather != weatherLine ||
      lastDrawnIp != ipLine || lastDrawnWeatherCode != weather.weatherCode) {
    fullRedraw = true;
  }

  if (fullRedraw) {
    tft.fillScreen(TFT_BLACK);

    tft.drawRoundRect(6, 6, 308, 116, 10, TFT_DARKGREY);
    tft.drawRoundRect(6, 130, 308, 82, 10, TFT_DARKGREY);

    tft.setTextDatum(middle_center);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(dateLine, 160, 104, &fonts::Font2);

    // Weather icon on the left, text on the right.
    int iconCode = weather.valid ? weather.weatherCode : -1;
    drawWeatherIcon(iconCode, 48, 171);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(settings.city, 190, 146, &fonts::Font4);

    tft.setTextColor(weather.valid ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
    tft.drawString(weatherLine, 190, 180, &fonts::Font2);

    if (weather.valid) {
      String rainLine = "Today rain: " + String(weather.rainSum, 1) + " mm  Updated: " + weather.updatedAt;
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      tft.drawString(rainLine, 190, 200, &fonts::Font2);
    }

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(ipLine + "  Rot " + String(settings.rotation), 160, 228, &fonts::Font2);
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
  lastDrawnWeatherCode = weather.weatherCode;
}
