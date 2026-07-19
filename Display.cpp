#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <time.h>

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

extern TFT_eSPI tft;
extern AppSettings settings;
extern WeatherData weather;

static String lastDrawnTime = "";
static String lastDrawnDate = "";
static String lastDrawnWeather = "";
static String lastDrawnIp = "";

void applyBrightness() {
  // ESP32 Arduino LEDC API changed in newer cores. analogWrite works on current ESP32 Arduino cores.
  // If your core is older and analogWrite is unavailable, replace with ledcSetup/ledcAttachPin/ledcWrite.
  pinMode(21, OUTPUT);
  analogWrite(21, settings.brightness);
}

void drawBootScreen(const String& message) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("ESP32 Clock", 160, 88, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(message, 160, 130, 2);
}

void drawStatusScreen(const String& line1, const String& line2) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(line1, 160, 100, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(line2, 160, 140, 2);
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

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(dateLine, 160, 104, 2);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(settings.city, 160, 146, 4);

    tft.setTextColor(weather.valid ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
    tft.drawString(weatherLine, 160, 180, 2);

    if (weather.valid) {
      String rainLine = "Today rain: " + String(weather.rainSum, 1) + " mm  Updated: " + weather.updatedAt;
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      tft.drawString(rainLine, 160, 200, 2);
    }

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(ipLine, 160, 228, 2);
  }

  if (lastDrawnTime != timeLine || fullRedraw) {
    tft.fillRect(15, 30, 290, 58, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(timeLine, 160, 62, 7);
  }

  lastDrawnTime = timeLine;
  lastDrawnDate = dateLine;
  lastDrawnWeather = weatherLine;
  lastDrawnIp = ipLine;
}
