/*
  ESP32 Hybrid Mode WiFi Manager with Captive Portal, Embedded Dashboard, and MQTT Publisher
  ------------------------------------------------------------------------------------------
  - AP + STA mode
  - Serves embedded HTML dashboard from flash
  - Scans nearby WiFi networks
  - Connects/disconnects from WiFi
  - JSON API for UI (incl. /api/sensors)
  - Robust captive portal (DNS spoof, OS probe endpoints, host-agnostic redirect)
  - Publishes simulated sensor data via MQTT (PubSubClient)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "index_html.h"  // Embedded HTML UI (see updated snippet further below)

// ===================== CONFIGURATION =====================
static const char* apSSID = "ESP32_AP";
static const char* apPassword = "12345678";

// MQTT Broker Settings
static const char* mqttServer = "broker.hivemq.com";
static const int   mqttPort   = 1883;
static const char* mqttTopic  = "esp32/sensor/data";

// ===================== GLOBALS ============================
WebServer   server(80);
DNSServer   dnsServer;
WiFiClient  espClient;
PubSubClient mqttClient(espClient);

const byte DNS_PORT = 53;

IPAddress apIP(192,168,4,1);
IPAddress apGateway(192,168,4,1);
IPAddress apSubnet(255,255,255,0);

// Cache variables for last WiFi scan
String lastScanJson;
bool   lastScanAvailable = false;

// Simulated sensor values
int   tempVal     = 25;
int   lightVal    = 500;
float humidityVal = 60.0;

// =================== Helper Functions ====================
String encryptionTypeStr(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN:            return "Open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
#ifdef WIFI_AUTH_WPA3_PSK
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
#endif
#ifdef WIFI_AUTH_WPA2_WPA3_PSK
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
#endif
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    default:                        return "Unknown";
  }
}

// Any request with a Host header not matching our AP IP is treated as captive
bool isCaptivePortal() {
  if (!server.hasHeader("Host")) return true;
  String host = server.header("Host");
  // Accept "ap ip" or "ap ip:80"
  if (host == apIP.toString() || host == (apIP.toString() + ":80")) return false;
  return true;
}

void sendRedirectToRoot() {
  String url = "http://" + apIP.toString() + "/";
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
  Serial.printf("[HTTP] Captive redirect -> %s\n", url.c_str());
}

// ================= MQTT Functions ========================
void mqttReconnect() {
  while (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("[MQTT] Connecting...");
    if (mqttClient.connect("ESP32_Client")) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
  }
}

void publishSensorData() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();

  char payload[100];
  snprintf(payload, sizeof(payload),
           "{\"temp\":%d,\"light\":%d,\"humidity\":%.2f}",
           tempVal, lightVal, humidityVal);

  bool ok = mqttClient.publish(mqttTopic, payload);
  Serial.printf("[MQTT] Publish [%s]: %s\n", ok ? "OK" : "FAIL", payload);

  // Update simulated values (varied but bounded)
  tempVal     = 20 + random(0, 11);
  lightVal    = 400 + random(0, 201);
  humidityVal = 50  + random(0, 21);
}

// ===================== HTTP Handlers =====================
// Serve the dashboard
void handleRoot() {
  Serial.println("[HTTP] GET / (dashboard)");
  // Serve from PROGMEM blob
  server.setContentLength(index_html_len);
  server.send(200, "text/html", (const char*)index_html);
}

// OS captive probes -> force the captive portal UI
void handleAndroidProbe() {  // /generate_204
  Serial.println("[HTTP] Android probe -> redirect");
  sendRedirectToRoot();
}
void handleAppleProbe() {    // /hotspot-detect.html
  Serial.println("[HTTP] Apple probe -> simple page");
  server.send(200, "text/html",
              "<html><head><meta http-equiv='refresh' content='0; url=/'/></head>"
              "<body>Login...</body></html>");
}
void handleWindowsProbe() {  // /ncsi.txt and /connecttest.txt
  Serial.println("[HTTP] Windows probe -> redirect");
  sendRedirectToRoot();
}

// Generic: if host mismatches, redirect; otherwise serve index
void handleAnyPath() {
  if (isCaptivePortal()) {
    Serial.printf("[HTTP] Captive host redirect from path: %s\n", server.uri().c_str());
    sendRedirectToRoot();
    return;
  }
  Serial.printf("[HTTP] GET %s -> serve index\n", server.uri().c_str());
  handleRoot();
}

void handleScan() {
  Serial.println("[HTTP] GET /api/wifi/scan -> starting scan...");
  int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
  Serial.printf("[WIFI] Networks found: %d\n", n);

  DynamicJsonDocument doc(4096);
  doc["status"] = "success";
  doc["count"]  = (n > 0) ? n : 0;
  JsonArray nets = doc.createNestedArray("networks");

  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String encStr = encryptionTypeStr(WiFi.encryptionType(i));
      bool isEnc = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

      JsonObject net = nets.createNestedObject();
      net["ssid"]       = WiFi.SSID(i);
      net["rssi"]       = WiFi.RSSI(i);
      net["channel"]    = WiFi.channel(i);
      net["bssid"]      = WiFi.BSSIDstr(i);
      net["encryption"] = encStr;
      net["encrypted"]  = isEnc;
    }
  }

  String out;
  serializeJson(doc, out);
  lastScanJson = out;
  lastScanAvailable = true;

  WiFi.scanDelete();
  server.send(200, "application/json", out);
  Serial.printf("[HTTP] /api/wifi/scan -> responded with %d networks\n", n);
}

void handleScanResults() {
  Serial.println("[HTTP] GET /api/wifi/scan/results");
  if (lastScanAvailable) {
    server.send(200, "application/json", lastScanJson);
  } else {
    server.send(404, "application/json",
                "{\"status\":\"error\",\"message\":\"No scan results\"}");
  }
}

void handleConnect() {
  Serial.println("[HTTP] POST /api/wifi/connect -> connect request");
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");
  Serial.printf("[HTTP] Body: %s\n", body.c_str());
  DynamicJsonDocument req(512);
  if (deserializeJson(req, body)) {
    server.send(400, "application/json",
                "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* ssid = req["ssid"] | "";
  const char* password = req["password"] | "";
  if (strlen(ssid) == 0) {
    server.send(400, "application/json",
                "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
    return;
  }

  Serial.printf("[WIFI] Connecting to SSID: %s\n", ssid);
  if (strlen(password) > 0) WiFi.begin(ssid, password);
  else                      WiFi.begin(ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  DynamicJsonDocument resp(512);
  if (WiFi.status() == WL_CONNECTED) {
    resp["status"]  = "success";
    resp["message"] = "Connected";
    resp["ssid"]    = WiFi.SSID();
    resp["ip"]      = WiFi.localIP().toString();
    Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    resp["status"]  = "error";
    resp["message"] = "Failed to connect";
    Serial.println("[WIFI] Failed to connect.");
  }

  String out;
  serializeJson(resp, out);
  server.send(WiFi.status() == WL_CONNECTED ? 200 : 500, "application/json", out);
}

void handleDisconnect() {
  Serial.println("[HTTP] POST /api/wifi/disconnect -> disconnecting");
  WiFi.disconnect(true, true);
  server.send(200, "application/json",
              "{\"status\":\"success\",\"message\":\"Disconnected\"}");
}

void handleStatus() {
  Serial.println("[HTTP] GET /api/wifi/status");
  DynamicJsonDocument doc(512);
  JsonObject ap = doc.createNestedObject("ap");
  ap["ssid"] = apSSID;
  ap["ip"]   = apIP.toString();
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

void handleSensors() {
  // Provide the same values the device is publishing to MQTT
  DynamicJsonDocument doc(256);
  doc["temperature"] = tempVal;
  doc["humidity"]    = humidityVal;
  doc["light"]       = lightVal;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
  Serial.printf("[HTTP] /api/sensors -> %s\n", out.c_str());
}

// ========================= SETUP ===========================
void setup() {
  // Give the serial monitor a moment to attach
  delay(200);
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("===== ESP32 WiFi Manager Booting =====");

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);

  // Configure AP IP explicitly (more reliable on some cores)
  if (!WiFi.softAPConfig(apIP, apGateway, apSubnet)) {
    Serial.println("[AP] softAPConfig FAILED, using default 192.168.4.1");
  }
  bool apOk = WiFi.softAP(apSSID, apPassword);
  if (!apOk) {
    Serial.println("[AP] softAP FAILED!");
  } else {
    Serial.printf("[AP] SSID: %s  PASS: %s  IP: %s\n",
                  apSSID, apPassword, WiFi.softAPIP().toString().c_str());
  }

  // Start DNS wildcard -> everything to our AP IP
  bool dnsOk = dnsServer.start(DNS_PORT, "*", apIP);
  Serial.printf("[DNS] start(%d, *, %s) -> %s\n", DNS_PORT,
                apIP.toString().c_str(), dnsOk ? "OK" : "FAIL");

  // --- Web routes ---
  // Captive portal / OS probes
  server.on("/generate_204", HTTP_ANY, handleAndroidProbe);   // Android
  server.on("/hotspot-detect.html", HTTP_ANY, handleAppleProbe); // iOS/macOS
  server.on("/ncsi.txt", HTTP_ANY, handleWindowsProbe);       // Windows
  server.on("/connecttest.txt", HTTP_ANY, handleWindowsProbe);// Win alt

  // API
  server.on("/api/wifi/scan",        HTTP_GET,  handleScan);
  server.on("/api/wifi/scan/results",HTTP_GET,  handleScanResults);
  server.on("/api/wifi/connect",     HTTP_POST, handleConnect);
  server.on("/api/wifi/disconnect",  HTTP_POST, handleDisconnect);
  server.on("/api/wifi/status",      HTTP_GET,  handleStatus);
  server.on("/api/sensors",          HTTP_GET,  handleSensors);

  // Dashboard at "/" and also catch-all for any HTTP path
  server.on("/", HTTP_ANY, handleRoot);
  server.onNotFound(handleAnyPath);

  server.begin();
  Serial.println("[HTTP] server started on port 80");

  // MQTT setup
  mqttClient.setServer(mqttServer, mqttPort);
  Serial.printf("[MQTT] broker: %s:%d topic: %s\n", mqttServer, mqttPort, mqttTopic);
  Serial.println("[BOOT] Setup complete.");
}

// ========================= LOOP ============================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    publishSensorData();
  }

  delay(1000);
}
