/*
  ESP32 Hybrid Mode WiFi Manager + Live Sensors - FIXED CAPTIVE PORTAL VERSION
  -------------------------------------------------------------------------
  - AP + STA mode with FIXED IP (192.168.4.1)
  - Serves embedded HTML dashboard
  - Async WiFi scan (matches dashboard JS expectations)
  - Connect/disconnect from WiFi
  - Live sensor readings (DHT11 + BH1750) via /api/sensors
  - JSON APIs for UI
  - Captive portal via DNS spoofing + HTTP redirect (FIXED)
  - Fixed all timing, memory, and stability issues
  - Fixed captive portal redirect to wrong IP
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "index_html.h"  // Embedded dashboard HTML

// ===== Sensors =====
#include <Wire.h>
#include <BH1750.h>
#include <DHTesp.h>

// ===== Config =====
const char* apSSID = "ESP32_AP";
const char* apPassword = "12345678";
const byte DNS_PORT = 53;
const String CORRECT_AP_IP = "192.168.4.1"; // Hardcoded correct IP

#define DHT_PIN 4
#define SDA_PIN 21
#define SCL_PIN 22

// ===== Objects =====
WebServer server(80);
DNSServer dnsServer;
DHTesp dht;
BH1750 lightMeter;

// ===== WiFi scan state =====
bool scanInProgress = false;
unsigned long scanStartTime = 0;
String scanResultsJson;
int8_t lastScanResult = WIFI_SCAN_FAILED;

// ===== System Info Tracking =====
int lastNetworkCount = 0;
unsigned long lastSensorUpdateMs = 0;
unsigned long systemStartTime = 0;

// ===== Sensor data caching =====
struct SensorData {
  float temperature = 0.0;
  float humidity = 0.0;
  float light = 0.0;
  unsigned long lastUpdate = 0;
  bool valid = false;
} sensorCache;

// ===== WiFi Connection Management =====
unsigned long wifiConnectStartTime = 0;
bool wifiConnectionInProgress = false;
const unsigned long WIFI_CONNECT_TIMEOUT = 20000; // 20 seconds

// ======================= Helpers =======================
String encryptionTypeStr(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "Unknown";
  }
}

void updateSensorCache() {
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead < 2000) return; // Don't read sensors too frequently
  
  lastSensorRead = millis();
  
  // Read DHT sensor with error checking
  TempAndHumidity th = dht.getTempAndHumidity();
  if (dht.getStatus() == DHTesp::ERROR_NONE) {
    // Only update if readings are reasonable
    if (th.temperature > -40 && th.temperature < 80 && 
        th.humidity >= 0 && th.humidity <= 100) {
      sensorCache.temperature = th.temperature;
      sensorCache.humidity = th.humidity;
      sensorCache.valid = true;
    }
  } else {
    Serial.printf("[DHT] Error: %s\n", dht.getStatusString());
  }
  
  // Read light sensor with error checking
  float lux = lightMeter.readLightLevel();
  if (lux >= 0 && lux < 100000) { // Reasonable light range
    sensorCache.light = lux;
  }
  
  sensorCache.lastUpdate = millis();
  lastSensorUpdateMs = millis();
}

// ======================= Routes ========================

// Serve dashboard HTML
void handleRoot() {
  server.sendHeader("Content-Length", String(index_html_len));
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/html", (const char*)index_html);
}

// Start async WiFi scan
void handleScanStart() {
  if (scanInProgress) {
    server.send(429, "application/json", "{\"status\":\"error\",\"message\":\"Scan already in progress\"}");
    return;
  }
  
  // Clean up any previous scan results
  WiFi.scanDelete();
  
  Serial.println("[SCAN] Starting async WiFi scan...");
  int result = WiFi.scanNetworks(true, true); // async, include hidden
  
  if (result == WIFI_SCAN_RUNNING) {
    scanInProgress = true;
    scanStartTime = millis();
    server.send(202, "application/json", "{\"status\":\"scanning\"}");
  } else {
    Serial.printf("[SCAN] Failed to start scan, result: %d\n", result);
    server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to start scan\"}");
  }
}

// Return scan results
void handleScanResults() {
  if (scanInProgress) {
    int scanRes = WiFi.scanComplete();
    
    // Check for timeout (30 seconds)
    if (millis() - scanStartTime > 30000) {
      Serial.println("[SCAN] Timeout - resetting scan state");
      WiFi.scanDelete();
      scanInProgress = false;
      server.send(408, "application/json", "{\"status\":\"error\",\"message\":\"Scan timeout\"}");
      return;
    }
    
    if (scanRes == WIFI_SCAN_RUNNING) {
      server.send(202, "application/json", "{\"status\":\"scanning\"}");
      return;
    }
    
    if (scanRes >= 0) {
      lastNetworkCount = scanRes;
      lastScanResult = scanRes;
      
      // Use a larger buffer for network results
      DynamicJsonDocument doc(6144);
      doc["status"] = "success";
      JsonArray nets = doc.createNestedArray("networks");
      
      for (int i = 0; i < scanRes; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 0) { // Only include networks with valid SSID
          JsonObject net = nets.createNestedObject();
          net["ssid"] = ssid;
          net["rssi"] = WiFi.RSSI(i);
          net["encryption"] = encryptionTypeStr(WiFi.encryptionType(i));
          net["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
      }
      
      WiFi.scanDelete();
      scanInProgress = false;
      
      scanResultsJson = "";
      serializeJson(doc, scanResultsJson);
      server.send(200, "application/json", scanResultsJson);
      
      Serial.printf("[SCAN] Completed successfully, found %d networks\n", scanRes);
      return;
    }
    
    // Scan failed
    Serial.printf("[SCAN] Failed with result: %d\n", scanRes);
    WiFi.scanDelete();
    scanInProgress = false;
    server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Scan failed\"}");
    
  } else {
    // Return cached results if available
    if (scanResultsJson.length() > 0 && lastScanResult >= 0) {
      server.send(200, "application/json", scanResultsJson);
    } else {
      server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"No scan results available\"}");
    }
  }
}

// Connect to WiFi
void handleConnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"status\":\"error\",\"message\":\"Method Not Allowed\"}");
    return;
  }
  
  if (wifiConnectionInProgress) {
    server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Connection already in progress\"}");
    return;
  }
  
  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Empty request body\"}");
    return;
  }
  
  DynamicJsonDocument req(512);
  DeserializationError error = deserializeJson(req, body);
  if (error) {
    Serial.printf("[WiFi] JSON parse error: %s\n", error.c_str());
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }
  
  const char* ssid = req["ssid"];
  const char* password = req["password"];
  
  if (!ssid || strlen(ssid) == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
    return;
  }
  
  Serial.printf("[WiFi] Connecting to %s...\n", ssid);
  
  // Disconnect from any current connection
  WiFi.disconnect(false);
  delay(100);
  
  wifiConnectionInProgress = true;
  wifiConnectStartTime = millis();
  
  // Start connection
  if (password && strlen(password) > 0) {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }
  
  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
    delay(100);
    yield(); // Allow other tasks to run
  }
  
  wifiConnectionInProgress = false;
  
  DynamicJsonDocument resp(512);
  if (WiFi.status() == WL_CONNECTED) {
    resp["status"] = "success";
    resp["message"] = "Connected successfully";
    resp["ssid"] = WiFi.SSID();
    resp["ip"] = WiFi.localIP().toString();
    resp["rssi"] = WiFi.RSSI();
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    resp["status"] = "error";
    resp["message"] = "Failed to connect - check password and signal strength";
    Serial.printf("[WiFi] Connection failed, status: %d\n", WiFi.status());
  }
  
  String json;
  serializeJson(resp, json);
  server.send(200, "application/json", json);
}

// Disconnect from WiFi
void handleDisconnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"status\":\"error\",\"message\":\"Method Not Allowed\"}");
    return;
  }
  
  Serial.println("[WiFi] Disconnecting...");
  WiFi.disconnect(true);
  delay(100);
  
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Disconnected from WiFi\"}");
}

// Return AP+STA status (FIXED IP)
void handleStatus() {
  DynamicJsonDocument doc(768);
  
  JsonObject ap = doc.createNestedObject("ap");
  ap["ssid"] = apSSID;
  ap["ip"] = CORRECT_AP_IP; // Always use hardcoded correct IP
  ap["connected_clients"] = WiFi.softAPgetStationNum();
  ap["mac"] = WiFi.softAPmacAddress();

  JsonObject sta = doc.createNestedObject("sta");
  sta["connected"] = (WiFi.status() == WL_CONNECTED);
  sta["status"] = WiFi.status();
  
  if (WiFi.status() == WL_CONNECTED) {
    sta["ssid"] = WiFi.SSID();
    sta["ip"] = WiFi.localIP().toString();
    sta["rssi"] = WiFi.RSSI();
    sta["mac"] = WiFi.macAddress();
    sta["gateway"] = WiFi.gatewayIP().toString();
    sta["dns"] = WiFi.dnsIP().toString();
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Return live sensor data
void handleSensors() {
  updateSensorCache();
  
  DynamicJsonDocument doc(256);
  doc["temperature"] = sensorCache.temperature;
  doc["humidity"] = sensorCache.humidity;
  doc["light"] = sensorCache.light;
  doc["last_update"] = sensorCache.lastUpdate;
  doc["valid"] = sensorCache.valid;
  doc["timestamp"] = millis();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);

  Serial.printf("[Sensors] Temp: %.1f°C, Hum: %.1f%%, Light: %.1f lx\n", 
                sensorCache.temperature, sensorCache.humidity, sensorCache.light);
}

// Return system info for dashboard
void handleSystemInfo() {
  DynamicJsonDocument doc(512);
  
  unsigned long currentTime = millis();
  doc["uptime_sec"] = currentTime / 1000;
  doc["heap_total"] = ESP.getHeapSize();
  doc["heap_free"] = ESP.getFreeHeap();
  doc["heap_used_percent"] = ((ESP.getHeapSize() - ESP.getFreeHeap()) * 100) / ESP.getHeapSize();
  
  // System timing info
  doc["last_update"] = lastSensorUpdateMs;
  doc["networks_found"] = lastNetworkCount;
  doc["wifi_status"] = WiFi.status();
  doc["ap_clients"] = WiFi.softAPgetStationNum();
  
  // Flash memory info
  doc["flash_total"] = ESP.getFlashChipSize();
  doc["flash_free"] = ESP.getFreeSketchSpace();
  
  // System uptime formatted
  unsigned long uptimeSec = currentTime / 1000;
  doc["uptime_formatted"] = String(uptimeSec / 3600) + "h " + 
                           String((uptimeSec % 3600) / 60) + "m " + 
                           String(uptimeSec % 60) + "s";

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Handle CORS preflight requests
void handleCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "text/plain", "");
}

// ======================= Setup ========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n===== ESP32 Greenhouse Control System =====");
  Serial.println("Version: 1.1.0 - FIXED CAPTIVE PORTAL");
  
  systemStartTime = millis();
  
  // Initialize WiFi in AP+STA mode
  Serial.println("[WiFi] Setting up AP+STA mode...");
  WiFi.mode(WIFI_AP_STA);
  
  // Force specific AP IP configuration
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  Serial.println("[AP] Configuring fixed IP address...");
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("[AP] ERROR: Failed to configure AP IP!");
  }
  
  // Configure and start Access Point
  Serial.println("[AP] Starting Access Point...");
  bool apStarted = WiFi.softAP(apSSID, apPassword);
  
  if (!apStarted) {
    Serial.println("[AP] ERROR: Failed to start Access Point!");
    delay(2000);
    ESP.restart(); // Restart if AP fails
  }
  
  // Verify the AP IP is correct
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[AP] SSID: %s\n", apSSID);
  Serial.printf("[AP] Password: %s\n", apPassword);
  Serial.printf("[AP] Configured IP: %s\n", local_IP.toString().c_str());
  Serial.printf("[AP] Actual IP: %s\n", apIP.toString().c_str());
  
  // Debug IP information
  Serial.println("[DEBUG] IP Investigation:");
  Serial.printf("  Expected IP: %s\n", CORRECT_AP_IP.c_str());
  Serial.printf("  WiFi.softAPIP(): %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("  IP bytes: %d.%d.%d.%d\n", apIP[0], apIP[1], apIP[2], apIP[3]);
  
  // Double-check IP is correct
  if (apIP.toString() != CORRECT_AP_IP) {
    Serial.printf("[AP] WARNING: IP mismatch! Expected %s, got %s\n", 
                  CORRECT_AP_IP.c_str(), apIP.toString().c_str());
    Serial.println("[AP] Will use hardcoded IP for redirects anyway");
  } else {
    Serial.println("[AP] IP configuration verified - OK!");
  }
  
  // Start DNS server for captive portal with the local IP
  dnsServer.start(DNS_PORT, "*", local_IP);
  Serial.printf("[DNS] Captive portal DNS server started on %s:%d\n", local_IP.toString().c_str(), DNS_PORT);

  // Initialize sensors
  Serial.println("[Sensors] Initializing...");
  
  // Initialize DHT sensor
  dht.setup(DHT_PIN, DHTesp::DHT11);
  Serial.printf("[DHT11] Initialized on pin %d\n", DHT_PIN);
  
  // Initialize I2C and light sensor
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.printf("[I2C] Initialized (SDA: %d, SCL: %d)\n", SDA_PIN, SCL_PIN);
  
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[BH1750] Light sensor initialized successfully");
  } else {
    Serial.println("[BH1750] WARNING - Light sensor not detected!");
  }
  
  // Wait a moment for sensors to stabilize
  delay(2000);
  updateSensorCache();
  Serial.printf("[Sensors] Initial readings - Temp: %.1f°C, Hum: %.1f%%, Light: %.1f lx\n", 
                sensorCache.temperature, sensorCache.humidity, sensorCache.light);
  
  // Setup web server routes
  Serial.println("[HTTP] Setting up routes...");
  
  // Main routes
  server.on("/", HTTP_GET, handleRoot);
  
  // API routes
  server.on("/api/wifi/scan", HTTP_GET, handleScanStart);
  server.on("/api/wifi/scan/results", HTTP_GET, handleScanResults);
  server.on("/api/wifi/connect", HTTP_POST, handleConnect);
  server.on("/api/wifi/disconnect", HTTP_POST, handleDisconnect);
  server.on("/api/wifi/status", HTTP_GET, handleStatus);
  server.on("/api/sensors", HTTP_GET, handleSensors);
  server.on("/api/system/info", HTTP_GET, handleSystemInfo);
  
  // CORS handling
  server.on("/api/wifi/connect", HTTP_OPTIONS, handleCORS);
  server.on("/api/wifi/disconnect", HTTP_OPTIONS, handleCORS);
  
  // FIXED Captive portal redirect - catch all unknown requests
  server.onNotFound([&]() {
    String host = server.hostHeader();
    String path = server.uri();
    
    // Remove port number from host if present
    int colonIndex = host.indexOf(':');
    if (colonIndex > 0) {
      host = host.substring(0, colonIndex);
    }
    
    Serial.printf("[Captive] Host: '%s', Path: '%s'\n", host.c_str(), path.c_str());
    
    // Check if this is already the correct host
    bool isCorrectHost = (host == CORRECT_AP_IP) || (host.length() == 0);
    
    if (!isCorrectHost) {
      // This is a captive portal request - redirect to correct IP
      String redirectURL = "http://" + CORRECT_AP_IP + "/";
      Serial.printf("[Captive] Redirecting from '%s' to '%s'\n", host.c_str(), redirectURL.c_str());
      
      server.sendHeader("Location", redirectURL, true);
      server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      server.sendHeader("Pragma", "no-cache");
      server.sendHeader("Expires", "-1");
      server.send(302, "text/plain", "Redirecting to Greenhouse Control Panel");
    } else if (path == "/" || path.startsWith("/index")) {
      // Serve dashboard for root paths
      Serial.println("[Captive] Serving dashboard for root request");
      handleRoot();
    } else if (!path.startsWith("/api/")) {
      // Redirect unknown non-API paths to root
      String redirectURL = "http://" + CORRECT_AP_IP + "/";
      Serial.printf("[Captive] Redirecting unknown path '%s' to root\n", path.c_str());
      server.sendHeader("Location", redirectURL, true);
      server.send(302, "text/plain", "Redirecting to Dashboard");
    } else {
      // API endpoint not found
      Serial.printf("[API] Unknown API endpoint: %s\n", path.c_str());
      server.send(404, "application/json", "{\"error\":\"API endpoint not found\"}");
    }
  });

  // Start web server
  server.begin();
  Serial.println("[HTTP] Web server started successfully");
  
  // Add server identification header
  server.sendHeader("Server", "ESP32-Greenhouse/1.1");
  
  Serial.println("\n===== System Ready =====");
  Serial.printf("Access Point: %s\n", apSSID);
  Serial.printf("Dashboard URL: http://%s\n", CORRECT_AP_IP.c_str());
  Serial.println("Connect to the WiFi network and navigate to http://192.168.4.1");
  Serial.println("Captive portal should automatically redirect to the dashboard");
  Serial.println("=========================\n");
}

// ======================= Loop ========================
void loop() {
  static unsigned long lastHeartbeat = 0;
  
  // Handle DNS requests for captive portal
  dnsServer.processNextRequest();
  
  // Handle HTTP requests
  server.handleClient();
  
  // Update sensor cache periodically
  updateSensorCache();
  
  // Check WiFi connection timeout
  if (wifiConnectionInProgress && millis() - wifiConnectStartTime > WIFI_CONNECT_TIMEOUT) {
    Serial.println("[WiFi] Connection timeout - resetting state");
    wifiConnectionInProgress = false;
  }
  
  // Check scan timeout
  if (scanInProgress && millis() - scanStartTime > 30000) {
    Serial.println("[SCAN] Scan timeout - cleaning up");
    WiFi.scanDelete();
    scanInProgress = false;
  }
  
  // Heartbeat every 30 seconds
  if (millis() - lastHeartbeat > 30000) {
    lastHeartbeat = millis();
    Serial.printf("[Heartbeat] Uptime: %lu min, Free heap: %u bytes, AP clients: %d, STA: %s\n", 
                  millis() / 60000, ESP.getFreeHeap(), WiFi.softAPgetStationNum(),
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}