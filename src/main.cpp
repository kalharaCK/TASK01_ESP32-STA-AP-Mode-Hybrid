/*
  ESP32 WiFi Manager with Captive Portal + Dashboard
  ---------------------------------------------------
  - Button double-press switches to Config Mode
  - Normal Mode: AP + STA, serves index_html dashboard
  - Config Mode: AP only, serves config_html page
  - Persistent AP SSID/password in NVS
  - API endpoints for index_html.h to be fully functional
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "index_html.h"
#include "config_html.h"

// ===================== CONFIG =====================
#define BUTTON_PIN 26
#define DEBOUNCE_TIME 50
#define DOUBLE_PRESS_TIME 2000
const byte DNS_PORT = 53;

// Defaults if nothing stored in NVS
static const char* DEFAULT_AP_SSID = "ESP32-Normal";
static const char* DEFAULT_AP_PASS = "12345678";
static const char* DEFAULT_CFG_SSID = "ESP32-Config";
static const char* DEFAULT_CFG_PASS = "12345678";

// ===================== GLOBALS ====================
WebServer   server(80);
DNSServer   dnsServer;
Preferences prefs;

volatile unsigned long lastPressTime = 0;
volatile unsigned long firstPressTime = 0;
volatile bool buttonPressed = false;
volatile int pressCount = 0;

bool configMode = false;

IPAddress apIP(192,168,4,1);
IPAddress apGateway(192,168,4,1);
IPAddress apSubnet(255,255,255,0);

// Simulated sensor values
float tempVal = 25.0;
float humidityVal = 60.0;
int   lightVal = 500;

// ===================== BUTTON ISR =================
void IRAM_ATTR handleButton() {
  unsigned long now = millis();
  if (now - lastPressTime > DEBOUNCE_TIME) {
    pressCount++;
    buttonPressed = true;
    lastPressTime = now;
  }
}

// ===================== HELPERS ====================
String encryptionTypeStr(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    default: return "Unknown";
  }
}

bool isCaptivePortal() {
  if (!server.hasHeader("Host")) return true;
  String host = server.header("Host");
  if (host == apIP.toString() || host == (apIP.toString() + ":80")) return false;
  return true;
}

void sendRedirectToRoot() {
  String url = "http://" + apIP.toString() + "/";
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}

// ===================== API HANDLERS ====================
void handleSensors() {
  StaticJsonDocument<256> doc;
  // Simulate value drift
  tempVal += random(-5, 6) * 0.1; if (tempVal < 15) tempVal = 15; if (tempVal > 35) tempVal = 35;
  humidityVal += random(-3, 4) * 0.5; if (humidityVal < 20) humidityVal = 20; if (humidityVal > 90) humidityVal = 90;
  lightVal += random(-20, 21); if (lightVal < 100) lightVal = 100; if (lightVal > 2000) lightVal = 2000;

  doc["temperature"] = tempVal;
  doc["humidity"]    = humidityVal;
  doc["light"]       = lightVal;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleWiFiStatus() {
  StaticJsonDocument<512> doc;
  JsonObject ap = doc.createNestedObject("ap");
  ap["ssid"] = WiFi.softAPSSID();
  ap["ip"] = WiFi.softAPIP().toString();
  ap["connected_clients"] = WiFi.softAPgetStationNum();

  JsonObject sta = doc.createNestedObject("sta");
  sta["connected"] = (WiFi.status() == WL_CONNECTED);
  if (WiFi.status() == WL_CONNECTED) {
    sta["ssid"] = WiFi.SSID();
    sta["ip"]   = WiFi.localIP().toString();
    sta["rssi"] = WiFi.RSSI();
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleScan() {
  int n = WiFi.scanNetworks(false, true);
  DynamicJsonDocument doc(2048);
  doc["status"] = "success";
  doc["count"] = (n > 0) ? n : 0;
  JsonArray nets = doc.createNestedArray("networks");

  for (int i = 0; i < n; i++) {
    JsonObject net = nets.createNestedObject();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["channel"] = WiFi.channel(i);
    net["bssid"] = WiFi.BSSIDstr(i);
    net["encryption"] = encryptionTypeStr(WiFi.encryptionType(i));
    net["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }

  WiFi.scanDelete();
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleConnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  DynamicJsonDocument req(256);
  if (deserializeJson(req, server.arg("plain"))) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }
  String ssid = req["ssid"] | "";
  String pass = req["password"] | "";

  if (ssid == "") {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
    return;
  }

  WiFi.begin(ssid.c_str(), pass.c_str());
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(250);
    tries++;
  }

  DynamicJsonDocument resp(256);
  if (WiFi.status() == WL_CONNECTED) {
    resp["status"] = "success";
    resp["message"] = "Connected";
    resp["ssid"] = WiFi.SSID();
    resp["ip"] = WiFi.localIP().toString();
  } else {
    resp["status"] = "error";
    resp["message"] = "Failed to connect";
  }
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

void handleDisconnect() {
  WiFi.disconnect(true, true);
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Disconnected\"}");
}

// /save in Config Mode
void handleSave() {
  if (server.method() == HTTP_POST) {
    String ssid = server.arg("ssid");
    String pass = server.arg("password");

    prefs.begin("wifi", false);
    prefs.putString("ap_ssid", ssid);
    prefs.putString("ap_pass", pass);
    prefs.end();

    server.send(200, "text/html", "<html><body><h2>Saved! Rebooting...</h2></body></html>");
    delay(1000);
    ESP.restart();
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

// ===================== SERVER MODES ====================
void setupServerRoutes() {
  // Unified root
  server.on("/", HTTP_GET, [](){
    if (configMode) {
      server.send(200, "text/html", config_html);
    } else {
      server.send(200, "text/html", index_html);
    }
  });

  // APIs (only valid in Normal Mode)
  server.on("/api/sensors", HTTP_GET, handleSensors);
  server.on("/api/wifi/status", HTTP_GET, handleWiFiStatus);
  server.on("/api/wifi/scan", HTTP_GET, handleScan);
  server.on("/api/wifi/scan/results", HTTP_GET, handleScan);
  server.on("/api/wifi/connect", HTTP_POST, handleConnect);
  server.on("/api/wifi/disconnect", HTTP_POST, handleDisconnect);

  // Config Mode handler
  server.on("/save", HTTP_POST, handleSave);

  // Unified onNotFound
  server.onNotFound([](){
    if (isCaptivePortal()) {
      sendRedirectToRoot();
    } else {
      if (configMode) {
        server.send(200, "text/html", config_html);
      } else {
        server.send(200, "text/html", index_html);
      }
    }
  });
  server.begin();
}

void startNormalMode() {
  prefs.begin("wifi", true);
  String ap_ssid = prefs.getString("ap_ssid", DEFAULT_AP_SSID);
  String ap_pass = prefs.getString("ap_pass", DEFAULT_AP_PASS);
  prefs.end();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  if (ap_pass == "") WiFi.softAP(ap_ssid.c_str());
  else WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  setupServerRoutes();
}

void startConfigMode() {
  prefs.begin("wifi", true);
  String ap_ssid = prefs.getString("ap_ssid", DEFAULT_CFG_SSID);
  String ap_pass = prefs.getString("ap_pass", DEFAULT_CFG_PASS);
  prefs.end();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  if (ap_pass == "") WiFi.softAP(ap_ssid.c_str());
  else WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  setupServerRoutes();
}

// ===================== SETUP/LOOP ====================
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton, FALLING);

  randomSeed(analogRead(0));
  startNormalMode();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (buttonPressed) {
    buttonPressed = false;
    if (pressCount == 1) {
      firstPressTime = millis();
    }
    else if (pressCount == 2) {
      unsigned long interval = millis() - firstPressTime;
      if (interval <= DOUBLE_PRESS_TIME) {
        configMode = true;
        server.stop();
        dnsServer.stop();
        startConfigMode();
      }
      pressCount = 0;
    }
  }
  if (pressCount == 1 && (millis() - firstPressTime > DOUBLE_PRESS_TIME)) {
    pressCount = 0;
  }
}
