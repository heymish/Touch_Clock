#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

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
};

extern AppSettings settings;
extern WeatherData weather;

static String weatherCodeToText(int code) {
  switch (code) {
    case 0: return "Clear";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 56:
    case 57: return "Freezing drizzle";
    case 61:
    case 63:
    case 65: return "Rain";
    case 66:
    case 67: return "Freezing rain";
    case 71:
    case 73:
    case 75: return "Snow";
    case 77: return "Snow grains";
    case 80:
    case 81:
    case 82: return "Rain showers";
    case 85:
    case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Storm/hail";
    default: return "Unknown";
  }
}

static String currentLocalTimeShort() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 50)) return "--:--";

  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  return String(buf);
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    weather.valid = false;
    weather.summary = "WiFi offline";
    Serial.println("Weather skipped: WiFi offline.");
    return false;
  }

  String url = "http://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(settings.latitude, 6);
  url += "&longitude=" + String(settings.longitude, 6);
  url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum,precipitation_probability_max";
  url += "&forecast_days=1";
  url += "&timezone=auto";

  Serial.print("Weather URL: ");
  Serial.println(url);

  HTTPClient http;
  http.setTimeout(8000);
  http.begin(url);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error: %d\n", code);
    http.end();
    weather.valid = false;
    weather.summary = "Weather error";
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("Weather JSON error: ");
    Serial.println(err.c_str());
    weather.valid = false;
    weather.summary = "JSON error";
    return false;
  }

  weather.weatherCode = doc["daily"]["weather_code"][0] | -1;
  weather.tempMax = doc["daily"]["temperature_2m_max"][0] | 0.0;
  weather.tempMin = doc["daily"]["temperature_2m_min"][0] | 0.0;
  weather.rainSum = doc["daily"]["precipitation_sum"][0] | 0.0;
  weather.precipProb = doc["daily"]["precipitation_probability_max"][0] | 0;
  weather.summary = weatherCodeToText(weather.weatherCode);
  weather.updatedAt = currentLocalTimeShort();
  weather.valid = true;

  Serial.println("Weather updated OK.");
  return true;
}
