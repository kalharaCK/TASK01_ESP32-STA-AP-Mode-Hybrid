/*
  ESP32 Hybrid Mode WiFi Manager with Captive Portal and Embedded index.html
  -------------------------------------------------------------------------
  - AP + STA mode
  - Serves embedded HTML dashboard from flash
  - Scans nearby WiFi networks
  - Connects/disconnects from WiFi
  - JSON API for UI
  - Captive portal via DNS spoofing + HTTP redirect

  Dependencies:
    - ArduinoJson v6
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "index_html.h"  // Your embedded index.html header

// ====== CONFIGURATION ======
const char* apSSID = "ESP32_AP";
const char* apPassword = "12345678";

WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;

// Cache variables for last WiFi scan
String lastScanJson;
bool lastScanAvailable = false;

// ====== Helper: Convert encryption type enum to readable string ======
String encryptionTypeStr(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    default: return "Unknown";
  }
}

// ====== ROUTE: Serve embedded index.html ======
void handleRoot() {
  Serial.println("[HTTP] GET / -> Serving embedded index.html");
  // Send embedded html file stored in flash
  server.sendHeader("Content-Length", String(index_html_len));
  server.send(200, "text/html", (const char *)index_html);
  Serial.println("[INFO] Embedded index.html served");
}

// ====== ROUTE: Scan WiFi networks ======
void handleScan() {
  Serial.println("[HTTP] GET /api/wifi/scan         -> Starting scan...");
  int n = WiFi.scanNetworks(false, true);
  Serial.printf("[INFO] Networks Found: %-3d\n", n);

  if (n > 0) {
    Serial.println("---------------------------------------------------------------");
    Serial.printf("%-3s %-32s %-8s %-12s %-10s\n", "#", "SSID", "RSSI", "Encryption", "Encrypted");
    Serial.println("---------------------------------------------------------------");
  }

  DynamicJsonDocument doc(4096);
  doc["status"] = "success";
  doc["count"] = (n > 0) ? n : 0;
  JsonArray nets = doc.createNestedArray("networks");

  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String encStr = encryptionTypeStr(WiFi.encryptionType(i));
      bool isEnc = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

      JsonObject net = nets.createNestedObject();
      net["ssid"] = WiFi.SSID(i);
      net["rssi"] = WiFi.RSSI(i);
      net["encryption"] = encStr;
      net["encrypted"] = isEnc;

      Serial.printf("%-3d %-32s %-8d %-12s %-10s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    encStr.c_str(),
                    isEnc ? "Yes" : "No");
    }
    Serial.println("---------------------------------------------------------------");
  }

  String out;
  serializeJson(doc, out);
  lastScanJson = out;
  lastScanAvailable = true;

  WiFi.scanDelete();
  server.send(200, "application/json", out);
  Serial.println("[INFO] Scan results sent to client");
}

// ====== ROUTE: Return cached scan results ======
void handleScanResults() {
  Serial.println("[HTTP] GET /api/wifi/scan/results -> Returning cached results");
  if (lastScanAvailable) {
    server.send(200, "application/json", lastScanJson);
  } else {
    Serial.println("[WARN] No cached scan results");
    server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"No scan results\"}");
  }
}

// ====== ROUTE: Connect to WiFi ======
void handleConnect() {
  Serial.println("[HTTP] POST /api/wifi/connect     -> Connect request");
  if (server.method() != HTTP_POST) {
    Serial.println("[ERROR] Invalid HTTP method");
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");
  Serial.printf("[DEBUG] Request JSON: %s\n", body.c_str());

  DynamicJsonDocument req(512);
  if (deserializeJson(req, body)) {
    Serial.println("[ERROR] JSON parse failed");
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* ssid = req["ssid"];
  const char* password = req["password"];

  if (!ssid || strlen(ssid) == 0) {
    Serial.println("[ERROR] Missing SSID");
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
    return;
  }

  Serial.printf("[INFO] Connecting to SSID: %-32s\n", ssid);
  if (password && strlen(password) > 0) {
    Serial.println("[DEBUG] Using password: ******");
    WiFi.begin(ssid, password);
  } else {
    Serial.println("[DEBUG] Open network, no password");
    WiFi.begin(ssid);
  }

  // Wait up to 10 seconds for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  DynamicJsonDocument resp(512);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[SUCCESS] Connected | IP: %-15s | RSSI: %d\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
    resp["status"] = "success";
    resp["message"] = "Connected";
    resp["ssid"] = WiFi.SSID();
    resp["ip"] = WiFi.localIP().toString();
    String out; serializeJson(resp, out);
    server.send(200, "application/json", out);
  } else {
    Serial.println("[FAIL] Connection attempt failed");
    resp["status"] = "error";
    resp["message"] = "Failed to connect";
    String out; serializeJson(resp, out);
    server.send(500, "application/json", out);
  }
}

// ====== ROUTE: Disconnect from WiFi ======
void handleDisconnect() {
  Serial.println("[HTTP] POST /api/wifi/disconnect  -> Disconnect request");
  WiFi.disconnect(true, true);  // Disconnect and erase stored credentials
  Serial.println("[INFO] Disconnected from STA");
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Disconnected\"}");
}

// ====== ROUTE: Status info ======
void handleStatus() {
  Serial.println("[HTTP] GET /api/wifi/status       -> Status request");
  DynamicJsonDocument doc(512);

  // AP Info
  JsonObject ap = doc.createNestedObject("ap");
  ap["ssid"] = apSSID;
  ap["ip"] = WiFi.softAPIP().toString();
  ap["connected_clients"] = WiFi.softAPgetStationNum();

  Serial.printf("[DEBUG] AP:   SSID: %-32s IP: %-15s Clients: %d\n",
                apSSID,
                WiFi.softAPIP().toString().c_str(),
                WiFi.softAPgetStationNum());

  // STA Info
  JsonObject sta = doc.createNestedObject("sta");
  sta["connected"] = (WiFi.status() == WL_CONNECTED);
  if (WiFi.status() == WL_CONNECTED) {
    sta["ssid"] = WiFi.SSID();
    sta["ip"] = WiFi.localIP().toString();
    sta["rssi"] = WiFi.RSSI();

    Serial.printf("[DEBUG] STA:  SSID: %-32s IP: %-15s RSSI: %d\n",
                  WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
  } else {
    Serial.println("[DEBUG] STA:  Not connected");
  }

  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== ESP32 WiFi Manager Booting =====");

  // Start AP + STA mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPassword);

  IPAddress apIP = WiFi.softAPIP();

  Serial.printf("[INFO] AP Started | SSID: %-32s IP: %-15s\n",
                apSSID,
                apIP.toString().c_str());

  // Start DNS Server for captive portal - redirects all DNS queries to ESP32
  dnsServer.start(DNS_PORT, "*", apIP);

  // Web routes
  server.on("/", HTTP_GET, handleRoot);

  // Captive portal - redirect unknown URLs to main page
  server.onNotFound([]() {
    Serial.printf("[HTTP] Unknown URL %s - redirecting to /\n", server.uri().c_str());
    IPAddress apIP = WiFi.softAPIP();
    server.sendHeader("Location", String("http://") + apIP.toString(), true);
    server.send(302, "text/plain", "");
  });

  // API endpoints
  server.on("/api/wifi/scan", HTTP_GET, handleScan);
  server.on("/api/wifi/scan/results", HTTP_GET, handleScanResults);
  server.on("/api/wifi/connect", HTTP_POST, handleConnect);
  server.on("/api/wifi/disconnect", HTTP_POST, handleDisconnect);
  server.on("/api/wifi/status", HTTP_GET, handleStatus);

  server.begin();
  Serial.println("[INFO] HTTP server started");
}

// ====== MAIN LOOP ======
void loop() {
  dnsServer.processNextRequest();  // Handle captive portal DNS requests
  server.handleClient();           // Handle web server requests
}