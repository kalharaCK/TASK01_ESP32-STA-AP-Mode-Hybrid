#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

//============================STA mode credentials (your home router)=======================
const char* ssid_sta = "DEFAULT SSID";             // replace with your Wi-Fi SSID
const char* password_sta = "DEFAULT PASSWORD";     // replace with your Wi-Fi password
// Note: If you want to use the default credentials, leave them as is.

//===========================AP mode credentials (ESP32's own Wi-Fi)=========================
const char* ssid_ap = "ESP32-AccessPoint";         // name of the ESP32's access point
const char* password_ap = "12345678";              // must be at least 8 characters
AsyncWebServer server(80);

// =====================Variables to store new WiFi credentials===================================
String new_ssid = "";
String new_password = "";
bool wifi_connect_requested = false;

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
  
  //===================================Start Access Point (AP) mode=============================
  bool apResult = WiFi.softAP(ssid_ap, password_ap);
  if (apResult) {
    Serial.println("ESP32 AP started.");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start ESP32 AP.");
  }
  //==========================================================================================

  // ===================================Serve dashboard========================================
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  // ===========================API endpoint for WiFi connection================================

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
      
      //===============================Store credentials for connection attempt==============================
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
  
  // ==========================================API endpoint to get WiFi status=============================
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
  
  // =========================================API endpoint to disconnect from WiFi==========================
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

  server.serveStatic("/", SPIFFS, "/");
  server.begin();
  Serial.println("HTTP server started.");
  Serial.println("Your ip address is: ");
  Serial.println(WiFi.localIP());
  // =========================================================================================

  //======================================== Start Station (STA) mode===========================
  WiFi.begin(ssid_sta, password_sta);
  Serial.println("Connecting to WiFi (STA Mode)...");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 10) {
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
  //=================================================================================================
}

void loop() {
  // ===================================Handle WiFi connection requests==============================
  if (wifi_connect_requested) {
    Serial.println("=== Processing WiFi Connection Request ===");
    wifi_connect_requested = false;
    
    Serial.println("Attempting to connect to new WiFi...");
    Serial.println("SSID: '" + new_ssid + "'");
    Serial.println("Password: '" + new_password + "'");
    
    // Disconnect from current WiFi
    Serial.println("Disconnecting from current WiFi...");
    WiFi.disconnect();
    delay(2000);
    
    // Try to connect to new WiFi
    Serial.println("Starting connection attempt...");
    WiFi.begin(new_ssid.c_str(), new_password.c_str());
    
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {  // Increased timeout
      delay(500);
      Serial.print(".");
      Serial.print(WiFi.status());  // Print status code
      retry++;
      
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
      
      Serial.println("Attempting to reconnect to original WiFi...");
      WiFi.begin(ssid_sta, password_sta);
      
      int original_retry = 0;
      while (WiFi.status() != WL_CONNECTED && original_retry < 20) {
        delay(500);
        Serial.print("o");
        original_retry++;
      }
      Serial.println();
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnected to original WiFi: " + String(ssid_sta));
      } else {
        Serial.println("Failed to reconnect to original WiFi!");
      }
    }
    Serial.println("=== WiFi Connection Process Complete ===");
  }
  
  delay(100);
}