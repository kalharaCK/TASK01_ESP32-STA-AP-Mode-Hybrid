#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "index_html.h" //converted index.html into a ready-to-use C header file

//============================STA mode credentials (your home router)==============================================
const char* ssid_sta = "DEFAULT SSID";             // replace with your Wi-Fi SSID
const char* password_sta = "DEFAULT PASSWORD";     // replace with your Wi-Fi password
// Note: If you want to use the default credentials, leave them as is.

//===========================AP mode credentials (ESP32's own Wi-Fi)===============================================
const char* ssid_ap = "ESP32-AccessPoint";         // name of the ESP32's access point
const char* password_ap = "12345678";              // must be at least 8 characters

AsyncWebServer server(80);
DNSServer dnsServer;

// =====================Variables to store new WiFi credentials====================================================
String new_ssid = "";
String new_password = "";
bool wifi_connect_requested = false;

// Captive portal function - redirects all requests to our main page
bool isCaptivePortal(AsyncWebServerRequest *request) {
  if (!request->hasHeader("Host")) {
    return true;
  }
  String hostHeader = request->getHeader("Host")->value();
  return !hostHeader.equals(WiFi.softAPIP().toString());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  // Hybrid Mode
  WiFi.mode(WIFI_AP_STA);
  
  //===================================Start Access Point (AP) mode===============================================
  bool apResult = WiFi.softAP(ssid_ap, password_ap);
  if (apResult) {
    Serial.println("ESP32 AP started.");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start ESP32 AP.");
  }

  // Start DNS Server for captive portal
  if (dnsServer.start(53, "*", WiFi.softAPIP())) {
    Serial.println("DNS Server started for captive portal");
  } else {
    Serial.println("Failed to start DNS Server");
  }
  //==============================================================================================================

  // ===================================Captive Portal Setup======================================================
  // Handle captive portal detection requests
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    String redirectURL = "http://" + WiFi.softAPIP().toString();
    request->redirect(redirectURL);
  });
  
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request){
    String redirectURL = "http://" + WiFi.softAPIP().toString();
    request->redirect(redirectURL);
  });
  
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){
    String redirectURL = "http://" + WiFi.softAPIP().toString();
    request->redirect(redirectURL);
  });
  
  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request){
    String redirectURL = "http://" + WiFi.softAPIP().toString();
    request->redirect(redirectURL);
  });

  // Additional captive portal detection endpoints
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request){
    String redirectURL = "http://" + WiFi.softAPIP().toString();
    request->redirect(redirectURL);
  });
  
  server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request){
    String redirectURL = "http://" + WiFi.softAPIP().toString();
    request->redirect(redirectURL);
  });

  // Catch-all handler for captive portal
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println("Request for: " + request->url());
    if (isCaptivePortal(request)) {
      Serial.println("Captive portal redirect: " + request->url());
      String redirectURL = "http://" + WiFi.softAPIP().toString();
      request->redirect(redirectURL);
    } else {
      // Serve the main page instead of 404 for direct IP access
      Serial.println("Serving main page for: " + request->url());
      request->send_P(200, "text/html", (const char*)index_html);
    }
  });
  //==============================================================================================================

  // ===================================Serve dashboard===========================================================
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Serving main dashboard page");
    request->send_P(200, "text/html", (const char*)index_html);
  });

  // Alternative if you want to serve from SPIFFS:
  /*
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  */

  // ===========================API endpoint for WiFi connection==================================================
  server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
      Serial.println("=== WiFi Connect API Called ===");
      Serial.printf("Data length: %d\n", len);
      Serial.printf("Raw data: %s\n", (char*)data);
      
      // Parse JSON data
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, (char*)data);
      
      if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
      }
      
      String ssid = doc["ssid"];
      String password = doc["password"];
      
      Serial.println("WiFi Connect Request:");
      Serial.println("SSID: '" + ssid + "'");
      Serial.println("Password: '" + password + "'");
      Serial.printf("SSID length: %d\n", ssid.length());
      Serial.printf("Password length: %d\n", password.length());
      
      if (ssid.length() == 0) {
        Serial.println("ERROR: Empty SSID received!");
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID cannot be empty\"}");
        return;
      }
      
      //===============================Store credentials for connection attempt===================================
      new_ssid = ssid;
      new_password = password;
      wifi_connect_requested = true;
      
      Serial.println("Credentials stored, wifi_connect_requested set to true");
      
      // Send response
      DynamicJsonDocument response(512);
      response["status"] = "connecting";
      response["message"] = "Attempting to connect to " + ssid;
      
      String responseStr;
      serializeJson(response, responseStr);
      request->send(200, "application/json", responseStr);
      Serial.println("Response sent to client");
    });
  
  // ==========================================API endpoint to get WiFi status====================================
  server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(512);
    
    // AP Status
    doc["ap"]["ssid"] = ssid_ap;
    doc["ap"]["ip"] = WiFi.softAPIP().toString();
    doc["ap"]["connected_clients"] = WiFi.softAPgetStationNum();
    
    // STA Status
    doc["sta"]["status"] = WiFi.status();
    doc["sta"]["connected"] = (WiFi.status() == WL_CONNECTED);
    if (WiFi.status() == WL_CONNECTED) {
      doc["sta"]["ssid"] = WiFi.SSID();
      doc["sta"]["ip"] = WiFi.localIP().toString();
      doc["sta"]["rssi"] = WiFi.RSSI();
    } else {
      doc["sta"]["ssid"] = "";
      doc["sta"]["ip"] = "";
      doc["sta"]["rssi"] = 0;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // =========================================API endpoint to disconnect from WiFi================================
  server.on("/api/wifi/disconnect", HTTP_POST, [](AsyncWebServerRequest *request){
    WiFi.disconnect();
    Serial.println("WiFi disconnected by user");
    
    DynamicJsonDocument doc(256);
    doc["status"] = "disconnected";
    doc["message"] = "Disconnected from WiFi";
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ===========================API endpoint to scan for available WiFi networks===================================
  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("WiFi scan requested");
    
    int networksFound = WiFi.scanNetworks();
    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("networks");
    
    for (int i = 0; i < networksFound; i++) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
    }
    
    WiFi.scanDelete(); // Clean up scan results
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/");
  
  // Start the server
  server.begin();
  Serial.println("HTTP server started.");
  
  // =============================================================================================================

  //======================================== Start Station (STA) mode=============================================
  WiFi.begin(ssid_sta, password_sta);
  Serial.println("Connecting to WiFi (STA Mode)...");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) { // Increased retry count
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to STA WiFi");
    Serial.print("STA IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to STA WiFi");
  }
  
  // Print both IP addresses for easy access
  Serial.println("=== Network Information ===");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("STA IP Address: ");
    Serial.println(WiFi.localIP());
  }
  Serial.println("===========================");
  Serial.println("Captive Portal Active - Users will be automatically redirected to configuration page");
  //==============================================================================================================
}

void loop() {
  // Process DNS requests for captive portal
  dnsServer.processNextRequest();
  
  // ===================================Handle WiFi connection requests===========================================
  if (wifi_connect_requested) {
    Serial.println("=== Processing WiFi Connection Request ===");
    wifi_connect_requested = false;
    
    Serial.println("Attempting to connect to new WiFi...");
    Serial.println("SSID: '" + new_ssid + "'");
    Serial.println("Password: '" + new_password + "'");
    
    // Disconnect from current WiFi
    Serial.println("Disconnecting from current WiFi...");
    WiFi.disconnect();
    delay(1000); // Reduced delay
    
    // Try to connect to new WiFi
    Serial.println("Starting connection attempt...");
    WiFi.begin(new_ssid.c_str(), new_password.c_str());
    
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {  // 15 second timeout
      delay(500);
      Serial.print(".");
      retry++;
      
      // ==============================Continue processing DNS requests during connection attempt=========================
      // This allows the captive portal to remain responsive
      dnsServer.processNextRequest();
      
      if (retry % 10 == 0) {  // Print status every 5 seconds
        Serial.println();
        Serial.printf("Connection attempt %d/30, Status: %d\n", retry, WiFi.status());
      }
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("SUCCESS: Connected to: " + new_ssid);
      Serial.print("New IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    } else {
      Serial.println("FAILED: Could not connect to: " + new_ssid);
      Serial.printf("Final WiFi status: %d\n", WiFi.status());
      Serial.println("WiFi Status meanings:");
      Serial.println("0=WL_IDLE_STATUS, 1=WL_NO_SSID_AVAIL, 2=WL_SCAN_COMPLETED");
      Serial.println("3=WL_CONNECTED, 4=WL_CONNECT_FAILED, 5=WL_CONNECTION_LOST, 6=WL_DISCONNECTED");
      
      // Only attempt to reconnect to original WiFi if it's not the default placeholder
      if (String(ssid_sta) != "DEFAULT SSID") {
        Serial.println("Attempting to reconnect to original WiFi...");
        WiFi.begin(ssid_sta, password_sta);
        
        int original_retry = 0;
        while (WiFi.status() != WL_CONNECTED && original_retry < 20) {
          delay(500);
          Serial.print("o");
          original_retry++;
          
          // Continue processing DNS requests during reconnection attempt
          dnsServer.processNextRequest();
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Reconnected to original WiFi: " + String(ssid_sta));
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());
        } else {
          Serial.println("Failed to reconnect to original WiFi!");
        }
      }
    }
    Serial.println("=== WiFi Connection Process Complete ===");
  }
  
  delay(100);
}