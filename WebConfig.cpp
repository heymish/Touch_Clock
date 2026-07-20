#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>

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

extern WebServer server;
extern AppSettings settings;
extern WeatherData weather;

void saveSettings();
void setupTime();
void applyRotation();
bool fetchWeather();
void applyBrightness();
void drawClockScreen();
void invalidateDisplayCache();

static String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

static String pageHeader(const String& title) {
  String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>" + title + "</title>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:620px;margin:28px auto;padding:0 16px;background:#111;color:#eee;}";
  html += "input,select{width:100%;box-sizing:border-box;padding:11px;margin:6px 0 16px;border-radius:8px;border:1px solid #555;background:#222;color:#eee;}";
  html += "input[type=range]{padding:0;}";
  html += "button,a.btn{display:inline-block;background:#00a6d6;color:#001018;border:0;border-radius:8px;padding:12px 16px;text-decoration:none;font-weight:bold;margin:6px 6px 6px 0;}";
  html += ".card{background:#1b1b1b;border:1px solid #333;border-radius:14px;padding:18px;margin:14px 0;}";
  html += ".hint{color:#aaa;font-size:.92em}.danger{background:#d65151!important;color:#fff!important}</style></head><body>";
  html += "<h1>" + title + "</h1>";
  return html;
}

static String pageFooter() {
  return "</body></html>";
}

static String selected(uint8_t value, uint8_t current) {
  return value == current ? " selected" : "";
}

void handleRoot() {
  String html = pageHeader("ESP32 Clock Setup");

  html += "<div class='card'><form method='POST' action='/save'>";
  html += "<label>Display city/name</label>";
  html += "<input name='city' maxlength='40' value='" + htmlEscape(settings.city) + "'>";

  html += "<label>Latitude</label>";
  html += "<input name='lat' inputmode='decimal' value='" + String(settings.latitude, 6) + "'>";

  html += "<label>Longitude</label>";
  html += "<input name='lon' inputmode='decimal' value='" + String(settings.longitude, 6) + "'>";

  html += "<label>POSIX timezone string</label>";
  html += "<input name='tz' value='" + htmlEscape(settings.timezone) + "'>";
  html += "<div class='hint'>NZ mainland example: NZST-12NZDT,M9.5.0,M4.1.0/3</div>";

  html += "<label>Clock format</label>";
  html += "<select name='h24'>";
  html += String("<option value='1'") + (settings.use24Hour ? " selected" : "") + ">24 hour</option>";
  html += String("<option value='0'") + (!settings.use24Hour ? " selected" : "") + ">12 hour</option>";
  html += "</select>";

  html += "<label>Display rotation</label>";
  html += "<select name='rotation'>";
  html += "<option value='0'" + selected(0, settings.rotation) + ">0 - Portrait</option>";
  html += "<option value='1'" + selected(1, settings.rotation) + ">1 - Landscape</option>";
  html += "<option value='2'" + selected(2, settings.rotation) + ">2 - Portrait inverted</option>";
  html += "<option value='3'" + selected(3, settings.rotation) + ">3 - Landscape inverted</option>";
  html += "</select>";
  html += "<div class='hint'>If the display is upside down or sideways, choose another rotation and save.</div>";

  html += "<label>Brightness: " + String(settings.brightness) + "</label>";
  html += "<input type='range' min='20' max='255' name='brightness' value='" + String(settings.brightness) + "'>";

  html += "<button type='submit'>Save settings</button>";
  html += "</form></div>";

  html += "<div class='card'>";
  html += "<p><b>Device IP:</b> " + WiFi.localIP().toString() + "</p>";
  html += "<p><b>Weather:</b> " + htmlEscape(weather.summary) + "</p>";
  html += "<p><b>Rotation:</b> " + String(settings.rotation) + "</p>";
  html += "<a class='btn' href='/weather'>Refresh weather</a>";
  html += "<a class='btn danger' href='/resetwifi'>Reset Wi-Fi</a>";
  html += "<a class='btn danger' href='/reboot'>Reboot</a>";
  html += "</div>";

  html += pageFooter();
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("city")) settings.city = server.arg("city");
  if (server.hasArg("tz")) settings.timezone = server.arg("tz");
  if (server.hasArg("lat")) settings.latitude = server.arg("lat").toFloat();
  if (server.hasArg("lon")) settings.longitude = server.arg("lon").toFloat();
  if (server.hasArg("h24")) settings.use24Hour = server.arg("h24") == "1";
  if (server.hasArg("brightness")) settings.brightness = constrain(server.arg("brightness").toInt(), 20, 255);
  if (server.hasArg("rotation")) settings.rotation = constrain(server.arg("rotation").toInt(), 0, 3);

  saveSettings();
  applyBrightness();
  applyRotation();
  setupTime();
  fetchWeather();
  invalidateDisplayCache();
  drawClockScreen();

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRefreshWeather() {
  fetchWeather();
  invalidateDisplayCache();
  drawClockScreen();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleResetWiFi() {
  String html = pageHeader("Wi-Fi reset");
  html += "<p>Saved Wi-Fi credentials have been cleared. The clock will reboot and start the ClockSetup access point.</p>";
  html += pageFooter();
  server.send(200, "text/html", html);
  delay(1000);

  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

void handleReboot() {
  server.send(200, "text/html", pageHeader("Rebooting") + "<p>Device is rebooting...</p>" + pageFooter());
  delay(1000);
  ESP.restart();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/weather", HTTP_GET, handleRefreshWeather);
  server.on("/resetwifi", HTTP_GET, handleResetWiFi);
  server.on("/reboot", HTTP_GET, handleReboot);

  server.begin();
  Serial.println("Web server started.");
}
