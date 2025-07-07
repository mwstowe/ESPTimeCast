#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>  // Add mDNS library
#include <sntp.h>
#include <time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "mfactoryfont.h"  // Replace with your font, or comment/remove if not using custom
#include "tz_lookup.h" // Timezone lookup, do not duplicate mapping here!

// Function prototypes
String formatTemperature(float temperature, bool roundToInteger = false);
void fetchNetatmoIndoorTemperature();
String getNetatmoToken();

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   12
#define DATA_PIN  15
#define CS_PIN    13

// DS18B20 Configuration
#define ONE_WIRE_BUS 4  // GPIO4 (D2) for DS18B20 data
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

AsyncWebServer server(80);

char ssid[32] = "";
char password[32] = "";
char netatmoClientId[64] = "";
char netatmoClientSecret[64] = "";
char netatmoUsername[64] = "";
char netatmoPassword[64] = "";
char netatmoAccessToken[256] = "";
char netatmoRefreshToken[256] = "";
char netatmoDeviceId[64] = "";
char netatmoModuleId[64] = "";
char netatmoIndoorModuleId[64] = ""; // New: Module ID for indoor temperature from Netatmo
char mdnsHostname[32] = "esptime"; // Default mDNS hostname
char weatherUnits[12] = "metric";
char timeZone[64] = "";
unsigned long clockDuration = 10000;
unsigned long weatherDuration = 5000;
int tempSource = 0; // 0: Local sensor primary, 1: Netatmo primary, 2: Netatmo only

// ADVANCED SETTINGS
int brightness = 7;
float tempAdjust = 0.0;
bool flipDisplay = false;
bool twelveHourToggle = false;
bool showDayOfWeek = true;
bool showIndoorTemp = true;
bool showOutdoorTemp = true;
char ntpServer1[64] = "pool.ntp.org";
char ntpServer2[64] = "time.nist.gov";

WiFiClient client;
const byte DNS_PORT = 53;
DNSServer dnsServer;

String indoorTemp = "";
String outdoorTemp = "";
bool indoorTempAvailable = false;
bool outdoorTempAvailable = false;
float netatmoIndoorTemp = 0;
bool netatmoIndoorTempAvailable = false;
bool weatherFetched = false;
bool weatherFetchInitiated = false;
bool isAPMode = false;
char tempSymbol = 'C';

unsigned long lastSwitch = 0;
unsigned long lastColonBlink = 0;
int displayMode = 0;
bool indicatorVisible = true; // Shared visibility state for colon and temperature indicators
bool needTokenRefresh = false; // Flag to indicate we need to refresh the token soon
unsigned long lastBlockedRequest = 0; // Timestamp of the last blocked request

bool ntpSyncSuccessful = false;

char daysOfTheWeek[7][12] = {"&", "*", "/", "?", "@", "=", "$"};
char indoorSymbol[12] = {"^"};  // Custom symbol for indoor temperature (IN)
char outdoorSymbol[12] = {"~"}; // Custom symbol for outdoor temperature (OUT)

// NTP Synchronization State Machine
enum NtpState {
  NTP_IDLE,
  NTP_SYNCING,
  NTP_SUCCESS,
  NTP_FAILED
};

NtpState ntpState = NTP_IDLE;
unsigned long ntpStartTime = 0;
const int ntpTimeout = 30000; // 30 seconds
const int maxNtpRetries = 30;
int ntpRetryCount = 0;

void printConfigToSerial() {
  Serial.println(F("========= Loaded Configuration ========="));
  Serial.print(F("WiFi SSID: ")); Serial.println(ssid);
  Serial.print(F("WiFi Password: ")); Serial.println(password);
  Serial.print(F("Netatmo Client ID: ")); Serial.println(netatmoClientId);
  Serial.print(F("Netatmo Client Secret: ")); Serial.println(netatmoClientSecret);
  Serial.print(F("Netatmo Username: ")); Serial.println(netatmoUsername);
  Serial.print(F("Netatmo Device ID: ")); Serial.println(netatmoDeviceId);
  Serial.print(F("Netatmo Module ID: ")); Serial.println(netatmoModuleId);
  Serial.print(F("Temperature Unit: ")); Serial.println(weatherUnits);
  Serial.print(F("Clock duration: ")); Serial.println(clockDuration);
  Serial.print(F("Weather duration: ")); Serial.println(weatherDuration);
  Serial.print(F("TimeZone (IANA): ")); Serial.println(timeZone);
  Serial.print(F("Brightness: ")); Serial.println(brightness);
  Serial.print(F("Temperature Adjustment: ")); Serial.println(tempAdjust);
  Serial.print(F("Flip Display: ")); Serial.println(flipDisplay ? "Yes" : "No");
  Serial.print(F("Show 12h Clock: ")); Serial.println(twelveHourToggle ? "Yes" : "No");
  Serial.print(F("Show Day of the Week: ")); Serial.println(showDayOfWeek ? "Yes" : "No");
  Serial.print(F("Show Indoor Temp: ")); Serial.println(showIndoorTemp ? "Yes" : "No");
  Serial.print(F("Show Outdoor Temp: ")); Serial.println(showOutdoorTemp ? "Yes" : "No");
  Serial.print(F("NTP Server 1: ")); Serial.println(ntpServer1);
  Serial.print(F("NTP Server 2: ")); Serial.println(ntpServer2);
  Serial.println(F("========================================"));
  Serial.println();
}

void loadConfig() {
  Serial.println(F("[CONFIG] Loading configuration..."));
  if (!LittleFS.begin()) {
    Serial.println(F("[ERROR] LittleFS mount failed"));
    return;
  }

  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] config.json not found, creating with defaults..."));
    DynamicJsonDocument doc(512);
    doc[F("ssid")] = "";
    doc[F("password")] = "";
    doc[F("netatmoClientId")] = "";
    doc[F("netatmoClientSecret")] = "";
    doc[F("netatmoUsername")] = "";
    doc[F("netatmoPassword")] = "";
    doc[F("netatmoAccessToken")] = "";
    doc[F("netatmoRefreshToken")] = "";
    doc[F("netatmoDeviceId")] = "";
    doc[F("netatmoModuleId")] = "";
    doc[F("netatmoIndoorModuleId")] = "";
    doc[F("tempSource")] = 0;
    doc[F("mdnsHostname")] = "esptime";
    doc[F("weatherUnits")] = "metric";
    doc[F("clockDuration")] = 10000;
    doc[F("weatherDuration")] = 5000;
    doc[F("timeZone")] = "";
    doc[F("brightness")] = brightness;
    doc[F("tempAdjust")] = tempAdjust;
    doc[F("flipDisplay")] = flipDisplay;
    doc[F("twelveHourToggle")] = twelveHourToggle;
    doc[F("showDayOfWeek")] = showDayOfWeek;
    doc[F("showIndoorTemp")] = showIndoorTemp;
    doc[F("showOutdoorTemp")] = showOutdoorTemp;
    doc[F("ntpServer1")] = ntpServer1;
    doc[F("ntpServer2")] = ntpServer2;
    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJsonPretty(doc, f);
      f.close();
      Serial.println(F("[CONFIG] Default config.json created."));
    } else {
      Serial.println(F("[ERROR] Failed to create default config.json"));
    }
  }
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[ERROR] Failed to open config.json"));
    return;
  }
  size_t size = configFile.size();
  if (size == 0 || size > 1024) {
    Serial.println(F("[ERROR] Invalid config file size"));
    configFile.close();
    return;
  }
  String jsonString = configFile.readString();
  configFile.close();
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.print(F("[ERROR] JSON parse failed: "));
    Serial.println(error.f_str());
    return;
  }
  if (doc.containsKey("ssid")) strlcpy(ssid, doc["ssid"], sizeof(ssid));
  if (doc.containsKey("password")) strlcpy(password, doc["password"], sizeof(password));
  if (doc.containsKey("netatmoClientId")) strlcpy(netatmoClientId, doc["netatmoClientId"], sizeof(netatmoClientId));
  if (doc.containsKey("netatmoClientSecret")) strlcpy(netatmoClientSecret, doc["netatmoClientSecret"], sizeof(netatmoClientSecret));
  if (doc.containsKey("netatmoUsername")) strlcpy(netatmoUsername, doc["netatmoUsername"], sizeof(netatmoUsername));
  if (doc.containsKey("netatmoPassword")) strlcpy(netatmoPassword, doc["netatmoPassword"], sizeof(netatmoPassword));
  if (doc.containsKey("netatmoAccessToken")) strlcpy(netatmoAccessToken, doc["netatmoAccessToken"], sizeof(netatmoAccessToken));
  if (doc.containsKey("netatmoRefreshToken")) strlcpy(netatmoRefreshToken, doc["netatmoRefreshToken"], sizeof(netatmoRefreshToken));
  if (doc.containsKey("netatmoDeviceId")) strlcpy(netatmoDeviceId, doc["netatmoDeviceId"], sizeof(netatmoDeviceId));
  if (doc.containsKey("netatmoModuleId")) strlcpy(netatmoModuleId, doc["netatmoModuleId"], sizeof(netatmoModuleId));
  if (doc.containsKey("netatmoIndoorModuleId")) strlcpy(netatmoIndoorModuleId, doc["netatmoIndoorModuleId"], sizeof(netatmoIndoorModuleId));
  if (doc.containsKey("tempSource")) tempSource = doc["tempSource"];
  if (doc.containsKey("mdnsHostname")) strlcpy(mdnsHostname, doc["mdnsHostname"], sizeof(mdnsHostname));
  if (doc.containsKey("weatherUnits")) strlcpy(weatherUnits, doc["weatherUnits"], sizeof(weatherUnits));
  if (doc.containsKey("clockDuration")) clockDuration = doc["clockDuration"];
  if (doc.containsKey("weatherDuration")) weatherDuration = doc["weatherDuration"];
  if (doc.containsKey("timeZone")) strlcpy(timeZone, doc["timeZone"], sizeof(timeZone));
  if (doc.containsKey("brightness")) brightness = doc["brightness"];
  if (doc.containsKey("tempAdjust")) tempAdjust = doc["tempAdjust"];
  if (doc.containsKey("flipDisplay")) flipDisplay = doc["flipDisplay"];
  if (doc.containsKey("twelveHourToggle")) twelveHourToggle = doc["twelveHourToggle"];
  if (doc.containsKey("showDayOfWeek")) showDayOfWeek = doc["showDayOfWeek"];
  if (doc.containsKey("showIndoorTemp")) showIndoorTemp = doc["showIndoorTemp"]; else showIndoorTemp = true;
  if (doc.containsKey("showOutdoorTemp")) showOutdoorTemp = doc["showOutdoorTemp"]; else showOutdoorTemp = true;
  if (doc.containsKey("ntpServer1")) strlcpy(ntpServer1, doc["ntpServer1"], sizeof(ntpServer1));
  if (doc.containsKey("ntpServer2")) strlcpy(ntpServer2, doc["ntpServer2"], sizeof(ntpServer2));
  if (strcmp(weatherUnits, "imperial") == 0)
    tempSymbol = 'F';
  else if (strcmp(weatherUnits, "standard") == 0)
    tempSymbol = 'K';
  else
    tempSymbol = 'C';
  Serial.println(F("[CONFIG] Configuration loaded."));
}

void connectWiFi() {
  Serial.println(F("[WIFI] Connecting to WiFi..."));
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 15000;
  unsigned long animTimer = 0;
  int animFrame = 0;
  bool animating = true;
  while (animating) {
    unsigned long now = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("[WiFi] Connected: ") + WiFi.localIP().toString());
      isAPMode = false;
      animating = false;
      
      // Initialize mDNS
      if (MDNS.begin(mdnsHostname)) {
        Serial.print(F("[mDNS] Responder started: "));
        Serial.print(mdnsHostname);
        Serial.println(F(".local"));
        
        // Add service to mDNS
        MDNS.addService("http", "tcp", 80);
      } else {
        Serial.println(F("[mDNS] Failed to start responder"));
      }
      
      break;
    } else if (now - startAttemptTime >= timeout) {
      Serial.println(F("\r\n[WiFi] Failed. Starting AP mode..."));
      WiFi.softAP("ESPTimeCast", "12345678");
      Serial.print(F("AP IP address: "));
      Serial.println(WiFi.softAPIP());
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      isAPMode = true;
      animating = false;
      Serial.println(F("[WIFI] AP Mode Started"));
      break;
    }
    if (now - animTimer > 750) {
      animTimer = now;
      P.setTextAlignment(PA_CENTER);
      switch (animFrame % 3) {
        case 0: P.print("WiFi©"); break;
        case 1: P.print("WiFiª"); break;
        case 2: P.print("WiFi«"); break;
      }
      animFrame++;
    }
    yield();
  }
}

void setupTime() {
  sntp_stop();
  Serial.println(F("[TIME] Starting NTP sync..."));
  configTime(0, 0, ntpServer1, ntpServer2); // Use custom NTP servers
  setenv("TZ", ianaToPosix(timeZone), 1);
  tzset();
  ntpState = NTP_SYNCING;
  ntpStartTime = millis();
  ntpRetryCount = 0;
  ntpSyncSuccessful = false; // Reset the flag
}

void setupWebServer() {
  Serial.println(F("[WEBSERVER] Setting up web server..."));
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /"));
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /config.json"));
    File f = LittleFS.open("/config.json", "r");
    if (!f) {
      Serial.println(F("[WEBSERVER] Error opening /config.json"));
      request->send(500, "application/json", "{\"error\":\"Failed to open config.json\"}");
      return;
    }
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
      Serial.print(F("[WEBSERVER] Error parsing /config.json: "));
      Serial.println(err.f_str());
      request->send(500, "application/json", "{\"error\":\"Failed to parse config.json\"}");
      return;
    }
    doc[F("mode")] = isAPMode ? "ap" : "sta";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /save"));    
    DynamicJsonDocument doc(2048);
    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      String n = p->name();
      String v = p->value();
      if (n == "brightness") doc[n] = v.toInt();
      else if (n == "flipDisplay") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "twelveHourToggle") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "showDayOfWeek") doc[n] = (v == "true" || v == "on" || v == "1"); 
      else if (n == "showHumidity") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "tempSource") doc[n] = v.toInt();
      else doc[n] = v;
    }
    if (LittleFS.exists("/config.json")) {
      LittleFS.rename("/config.json", "/config.bak");
    }
    File f = LittleFS.open("/config.json", "w");
    if (!f) {
      Serial.println(F("[WEBSERVER] Failed to open /config.json for writing"));
      DynamicJsonDocument errorDoc(256);
      errorDoc[F("error")] = "Failed to write config";
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }
    serializeJson(doc, f);
    f.close();
    File verify = LittleFS.open("/config.json", "r");
    DynamicJsonDocument test(2048);
    DeserializationError err = deserializeJson(test, verify);
    verify.close();
    if (err) {
      Serial.print(F("[WEBSERVER] Config corrupted after save: "));
      Serial.println(err.f_str());
      DynamicJsonDocument errorDoc(256);
      errorDoc[F("error")] = "Config corrupted. Reboot cancelled.";
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }
    DynamicJsonDocument okDoc(128);
    okDoc[F("message")] = "Saved successfully. Rebooting...";
    String response;
    serializeJson(okDoc, response);
    request->send(200, "application/json", response);
    Serial.println(F("[WEBSERVER] Rebooting..."));
    request->onDisconnect([]() {
      Serial.println(F("[WEBSERVER] Rebooting..."));
      ESP.restart();
    });
  });

  server.on("/restore", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /restore"));
    if (LittleFS.exists("/config.bak")) {
      File src = LittleFS.open("/config.bak", "r");
      if (!src) {
        Serial.println(F("[WEBSERVER] Failed to open /config.bak"));
        DynamicJsonDocument errorDoc(128);
        errorDoc[F("error")] = "Failed to open backup file.";
        String response;
        serializeJson(errorDoc, response);
        request->send(500, "application/json", response);
        return;
      }
      File dst = LittleFS.open("/config.json", "w");
      if (!dst) {
        src.close();
        Serial.println(F("[WEBSERVER] Failed to open /config.json for writing"));
        DynamicJsonDocument errorDoc(128);
        errorDoc[F("error")] = "Failed to open config for writing.";
        String response;
        serializeJson(errorDoc, response);
        request->send(500, "application/json", response);
        return;
      }
      // Copy contents
      while (src.available()) {
        dst.write(src.read());
      }
      src.close();
      dst.close();

      DynamicJsonDocument okDoc(128);
      okDoc[F("message")] = "✅ Backup restored! Device will now reboot.";
      String response;
      serializeJson(okDoc, response);
      request->send(200, "application/json", response);
      request->onDisconnect([]() {
        Serial.println(F("[WEBSERVER] Rebooting after restore..."));
        ESP.restart();
      });

    } else {
      Serial.println(F("[WEBSERVER] No backup found"));
      DynamicJsonDocument errorDoc(128);
      errorDoc[F("error")] = "No backup found.";
      String response;
      serializeJson(errorDoc, response);
      request->send(404, "application/json", response);
    }
  });

  server.on("/ap_status", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.print(F("[WEBSERVER] Request: /ap_status. isAPMode = "));
    Serial.println(isAPMode);
    String json = "{\"isAP\": ";
    json += (isAPMode) ? "true" : "false";
    json += "}";
    request->send(200, "application/json", json);
  });
  
  server.on("/refresh_netatmo_token", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /refresh_netatmo_token"));
    forceNetatmoTokenRefresh();
    String json = "{\"success\": true, \"message\": \"Token refresh initiated\"}";
    request->send(200, "application/json", json);
  });
  
  // Add new API endpoint to fetch Netatmo devices
  server.on("/api/netatmo/devices", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /api/netatmo/devices"));
    String devices = fetchNetatmoDevices();
    request->send(200, "application/json", devices);
  });
  
  // Add OAuth2 endpoints
  server.on("/api/netatmo/auth", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /api/netatmo/auth"));
    handleNetatmoAuth(request);
  });
  
  server.on("/oauth2/callback", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /oauth2/callback"));
    handleNetatmoCallback(request);
  });

  server.begin();
  Serial.println(F("[WEBSERVER] Web server started"));
}

// Function to get Netatmo OAuth token
// Function to save tokens to config.json
void saveTokensToConfig();

String getNetatmoToken() {
  Serial.println(F("[NETATMO] Getting OAuth token..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    return "";
  }
  
  // Check if we have an access token already
  if (strlen(netatmoAccessToken) > 0) {
    Serial.println(F("[NETATMO] Using existing access token"));
    return netatmoAccessToken;
  }
  
  // Check if we have a refresh token
  bool useRefreshToken = strlen(netatmoRefreshToken) > 0;
  Serial.print(F("[NETATMO] Using refresh token: "));
  Serial.println(useRefreshToken ? "Yes" : "No");
  
  // If no refresh token, check if we have username/password
  if (!useRefreshToken && (strlen(netatmoClientId) == 0 || strlen(netatmoClientSecret) == 0 || 
      strlen(netatmoUsername) == 0 || strlen(netatmoPassword) == 0)) {
    Serial.println(F("[NETATMO] Skipped: Missing Netatmo credentials"));
    return "";
  }
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  String token = "";
  
  if (https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "ESPTimeCast/1.0");
    
    String postData;
    
    if (useRefreshToken) {
      // Use refresh token flow
      postData = "grant_type=refresh_token&refresh_token=";
      postData += netatmoRefreshToken;
      postData += "&client_id=";
      postData += netatmoClientId;
      postData += "&client_secret=";
      postData += netatmoClientSecret;
    } else {
      // Use password flow
      postData = "grant_type=password&client_id=";
      postData += netatmoClientId;
      postData += "&client_secret=";
      postData += netatmoClientSecret;
      postData += "&username=";
      postData += netatmoUsername;
      postData += "&password=";
      postData += netatmoPassword;
      
      // Check for special characters in credentials that might need URL encoding
      if (postData.indexOf('@') != -1 || postData.indexOf('+') != -1 || 
          postData.indexOf('%') != -1 || postData.indexOf('&') != -1 ||
          postData.indexOf('=') != -1 || postData.indexOf(' ') != -1) {
        Serial.println(F("[NETATMO] WARNING: Credentials contain special characters that might need URL encoding"));
      }
    }
    postData += "&scope=read_station read_thermostat read_homecoach";
    
    Serial.print(F("[NETATMO] POST data: "));
    // Print a redacted version of the POST data for debugging
    String redactedPostData = postData;
    if (redactedPostData.indexOf("client_secret=") >= 0) {
      int start = redactedPostData.indexOf("client_secret=") + 14;
      int end = redactedPostData.indexOf("&", start);
      if (end < 0) end = redactedPostData.length();
      redactedPostData = redactedPostData.substring(0, start) + "REDACTED" + redactedPostData.substring(end);
    }
    if (redactedPostData.indexOf("password=") >= 0) {
      int start = redactedPostData.indexOf("password=") + 9;
      int end = redactedPostData.indexOf("&", start);
      if (end < 0) end = redactedPostData.length();
      redactedPostData = redactedPostData.substring(0, start) + "REDACTED" + redactedPostData.substring(end);
    }
    if (redactedPostData.indexOf("refresh_token=") >= 0) {
      int start = redactedPostData.indexOf("refresh_token=") + 14;
      int end = redactedPostData.indexOf("&", start);
      if (end < 0) end = redactedPostData.length();
      redactedPostData = redactedPostData.substring(0, start) + "REDACTED" + redactedPostData.substring(end);
    }
    Serial.println(redactedPostData);
    
    Serial.println(F("[NETATMO] Sending token request..."));
    int httpCode = https.POST(postData);
    Serial.print(F("[NETATMO] HTTP response code: "));
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      Serial.println(F("[NETATMO] Response received:"));
      Serial.println(payload);
      
      DynamicJsonDocument doc(2048); // Increased from 1024 to 2048
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if (doc.containsKey("access_token")) {
          token = doc["access_token"].as<String>();
          
          // Store the tokens for future use
          strlcpy(netatmoAccessToken, token.c_str(), sizeof(netatmoAccessToken));
          
          if (doc.containsKey("refresh_token")) {
            String refreshToken = doc["refresh_token"].as<String>();
            strlcpy(netatmoRefreshToken, refreshToken.c_str(), sizeof(netatmoRefreshToken));
            
            // Save the tokens to config.json
            saveTokensToConfig();
          }
          
          Serial.println(F("[NETATMO] Token obtained successfully"));
        } else {
          Serial.println(F("[NETATMO] Error: No access token in response"));
        }
      } else {
        Serial.print(F("[NETATMO] JSON parse error: "));
        Serial.println(error.c_str());
      }
    } else {
      Serial.print(F("[NETATMO] HTTP error: "));
      Serial.println(httpCode);
      
      // Get the error response
      String errorPayload = https.getString();
      Serial.println(F("[NETATMO] Error response:"));
      Serial.println(errorPayload);
      
      // Try to parse the error message
      DynamicJsonDocument errorDoc(2048); // Increased from 1024 to 2048
      DeserializationError errorParseError = deserializeJson(errorDoc, errorPayload);
      if (!errorParseError && errorDoc.containsKey("error")) {
        String errorType = errorDoc["error"].as<String>();
        String errorMessage = errorDoc.containsKey("error_description") ? 
                             errorDoc["error_description"].as<String>() : "Unknown error";
        
        Serial.print(F("[NETATMO] Error type: "));
        Serial.println(errorType);
        Serial.print(F("[NETATMO] Error message: "));
        Serial.println(errorMessage);
        
        // Handle specific error types
        if (errorType == "invalid_grant" || errorType == "invalid_client") {
          Serial.println(F("[NETATMO] Invalid credentials or refresh token, clearing tokens"));
          netatmoRefreshToken[0] = '\0';
          netatmoAccessToken[0] = '\0';
          saveTokensToConfig();
        }
      }
      
      // Check for "request is blocked" HTML response
      if (errorPayload.indexOf("The request is blocked") > 0) {
        handleBlockedRequest(errorPayload);
      }
      
      // If using an existing refresh token and it failed, clear it
      if (useRefreshToken) {
        Serial.println(F("[NETATMO] Refresh token might be expired, clearing it"));
        netatmoRefreshToken[0] = '\0';
        netatmoAccessToken[0] = '\0';
        saveTokensToConfig();
      }
    }
    
    https.end();
  } else {
    Serial.println(F("[NETATMO] Unable to connect to Netatmo API"));
  }
  
  return token;
}

// Function to read indoor temperature from DS18B20
void readIndoorTemperature(bool roundToInteger = false) {
  Serial.println(F("[DS18B20] Reading indoor temperature..."));
  
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  
  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print(F("[DS18B20] Raw temperature: "));
    Serial.print(tempC);
    Serial.println(F("°C"));
    
    // Format the temperature using our helper function
    indoorTemp = formatTemperature(tempC, roundToInteger);
    indoorTemp += "º";
    
    indoorTempAvailable = true;
    Serial.print(F("[DS18B20] Formatted indoor temperature: "));
    Serial.println(indoorTemp);
  } else {
    Serial.println(F("[DS18B20] Error: Could not read temperature data"));
    indoorTempAvailable = false;
  }
}

// Function to force Netatmo token refresh
void forceNetatmoTokenRefresh() {
  Serial.println(F("[NETATMO] Forcing token refresh"));
  netatmoAccessToken[0] = '\0';  // Clear access token
  needTokenRefresh = true;       // Set flag to refresh token
}

// Function to save tokens to config.json
void saveTokensToConfig() {
  Serial.println(F("[CONFIG] Saving tokens to config.json"));
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return;
  }
  
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for reading"));
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.print(F("[CONFIG] Failed to parse config file: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Update the tokens
  doc["netatmoAccessToken"] = netatmoAccessToken;
  doc["netatmoRefreshToken"] = netatmoRefreshToken;
  
  // Save the updated config
  configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for writing"));
    return;
  }
  
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("[CONFIG] Failed to write to config file"));
  } else {
    Serial.println(F("[CONFIG] Tokens saved successfully"));
  }
  
  configFile.close();
}

// Function to fetch outdoor temperature from Netatmo
void fetchOutdoorTemperature(bool roundToInteger = true) {
  Serial.println(F("\n[NETATMO] Fetching outdoor temperature..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    outdoorTempAvailable = false;
    return;
  }
  
  if (strlen(netatmoDeviceId) == 0 || strlen(netatmoModuleId) == 0) {
    Serial.println(F("[NETATMO] Skipped: Missing device or module ID"));
    Serial.print(F("[NETATMO] Device ID: '"));
    Serial.print(netatmoDeviceId);
    Serial.println(F("'"));
    Serial.print(F("[NETATMO] Module ID: '"));
    Serial.print(netatmoModuleId);
    Serial.println(F("'"));
    outdoorTempAvailable = false;
    return;
  }
  
  // Check if we have an access token
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] No access token available"));
    outdoorTempAvailable = false;
    return;
  }
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  
  String url = "https://api.netatmo.com/api/getstationsdata?device_id=";
  url += netatmoDeviceId;
  
  Serial.print(F("[NETATMO] Requesting URL: "));
  Serial.println(url);
  Serial.print(F("[NETATMO] Using token: "));
  Serial.println(String(netatmoAccessToken).substring(0, 10) + "...");
  
  if (https.begin(*client, url)) {
    https.addHeader("Authorization", "Bearer " + String(netatmoAccessToken));
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "ESPTimeCast/1.0");
    
    Serial.println(F("[NETATMO] Sending request..."));
    int httpCode = https.GET();
    Serial.print(F("[NETATMO] HTTP response code: "));
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      Serial.println(F("[NETATMO] Response received:"));
      Serial.println(payload);
      
      DynamicJsonDocument doc(8192); // Increased from 4096 to 8192
      
      if (parseNetatmoJson(payload, doc)) {
        // Navigate through the JSON to find the outdoor module
        JsonArray devices = doc["body"]["devices"];
        
        Serial.print(F("[NETATMO] Number of devices found: "));
        Serial.println(devices.size());
        
        bool deviceFound = false;
        bool moduleFound = false;
        
        for (JsonObject device : devices) {
          String deviceId = device["_id"].as<String>();
          Serial.print(F("[NETATMO] Found device ID: "));
          Serial.println(deviceId);
          
          if (deviceId == String(netatmoDeviceId)) {
            deviceFound = true;
            Serial.println(F("[NETATMO] Device ID matched!"));
            
            JsonArray modules = device["modules"];
            Serial.print(F("[NETATMO] Number of modules found: "));
            Serial.println(modules.size());
            
            for (JsonObject module : modules) {
              String moduleId = module["_id"].as<String>();
              String moduleName = module["module_name"].as<String>();
              Serial.print(F("[NETATMO] Found module: "));
              Serial.print(moduleId);
              Serial.print(F(" ("));
              Serial.print(moduleName);
              Serial.println(F(")"));
              
              if (moduleId == String(netatmoModuleId)) {
                moduleFound = true;
                Serial.println(F("[NETATMO] Module ID matched!"));
                
                JsonObject dashboard_data = module["dashboard_data"];
                
                if (dashboard_data.containsKey("Temperature")) {
                  float temp = dashboard_data["Temperature"];
                  
                  // Convert to the correct unit based on settings
                  if (strcmp(weatherUnits, "imperial") == 0) {
                    temp = (temp * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit
                  }
                  
                  outdoorTemp = formatTemperature(temp, roundToInteger) + "º";
                  Serial.print(F("[NETATMO] Outdoor temperature: "));
                  Serial.println(outdoorTemp);
                  outdoorTempAvailable = true;
                } else {
                  Serial.println(F("[NETATMO] Temperature data not found in dashboard_data"));
                  Serial.println(F("[NETATMO] Available dashboard_data fields:"));
                  for (JsonPair kv : dashboard_data) {
                    Serial.print(kv.key().c_str());
                    Serial.print(F(": "));
                    Serial.println(kv.value().as<String>());
                  }
                  outdoorTempAvailable = false;
                }
                break;
              }
            }
            
            if (!moduleFound) {
              Serial.println(F("[NETATMO] Error: Module ID not found in the device's modules"));
              outdoorTempAvailable = false;
            }
            break;
          }
        }
        
        if (!deviceFound) {
          Serial.println(F("[NETATMO] Error: Device ID not found in the response"));
          outdoorTempAvailable = false;
        }
      } else {
        Serial.println(F("[NETATMO] JSON parsing failed"));
        outdoorTempAvailable = false;
      }
    } else {
      Serial.print(F("[NETATMO] HTTP error: "));
      Serial.println(httpCode);
      
      // Get the error response
      String errorPayload = https.getString();
      Serial.println(F("[NETATMO] Error response:"));
      Serial.println(errorPayload);
      
      // Try to parse the error message
      DynamicJsonDocument errorDoc(1024);
      DeserializationError errorParseError = deserializeJson(errorDoc, errorPayload);
      if (!errorParseError && errorDoc.containsKey("error")) {
        String errorType = errorDoc["error"].as<String>();
        String errorMessage = errorDoc.containsKey("error_description") ? 
                             errorDoc["error_description"].as<String>() : "Unknown error";
        
        Serial.print(F("[NETATMO] Error type: "));
        Serial.println(errorType);
        Serial.print(F("[NETATMO] Error message: "));
        Serial.println(errorMessage);
        
        // Handle specific error types
        if (errorType == "access_denied" || errorMessage.indexOf("blocked") >= 0) {
          Serial.println(F("[NETATMO] Request blocked - check your Netatmo app permissions"));
        }
      }
      
      // Check for "request is blocked" HTML response
      if (errorPayload.indexOf("The request is blocked") > 0) {
        handleBlockedRequest(errorPayload);
      }
      // If we get a 403 error (Forbidden), the token is expired
      if (httpCode == 403 || httpCode == 401) {
        Serial.println(F("[NETATMO] 403/401 error - token expired or invalid"));
        forceNetatmoTokenRefresh();  // Force token refresh on next API call
      }
      
      outdoorTempAvailable = false;
    }
    
    https.end();
  } else {
    Serial.println(F("[NETATMO] Unable to connect to Netatmo API"));
    outdoorTempAvailable = false;
  }
}

// Function to update both indoor and outdoor temperatures
void updateTemperatures() {
  Serial.println(F("[TEMP] Updating temperatures..."));
  
  // Determine if we need to round temperatures to integers
  // (when both indoor and outdoor temps will be displayed)
  bool shouldRound = showIndoorTemp && (strlen(netatmoDeviceId) > 0 && strlen(netatmoModuleId) > 0);
  
  // Handle indoor temperature based on source setting
  if (tempSource == 0) { // Local sensor primary
    // Read indoor temperature from DS18B20
    readIndoorTemperature(shouldRound);
    
    // If local sensor failed and Netatmo indoor module is configured, try that as fallback
    if (!indoorTempAvailable && strlen(netatmoIndoorModuleId) > 0) {
      fetchNetatmoIndoorTemperature();
      if (netatmoIndoorTempAvailable) {
        // Convert Netatmo temperature to the same format as local sensor
        indoorTemp = formatTemperature(netatmoIndoorTemp, shouldRound);
        indoorTemp += "º";
        indoorTempAvailable = true;
        Serial.println(F("[TEMP] Using Netatmo as fallback for indoor temperature"));
      }
    }
  } else if (tempSource == 1) { // Netatmo primary
    // Try Netatmo first if configured
    if (strlen(netatmoIndoorModuleId) > 0) {
      fetchNetatmoIndoorTemperature();
      if (netatmoIndoorTempAvailable) {
        // Convert Netatmo temperature to the same format as local sensor
        indoorTemp = formatTemperature(netatmoIndoorTemp, shouldRound);
        indoorTemp += "º";
        indoorTempAvailable = true;
      } else {
        // Fallback to local sensor if Netatmo failed
        readIndoorTemperature(shouldRound);
        if (indoorTempAvailable) {
          Serial.println(F("[TEMP] Using local sensor as fallback for indoor temperature"));
        }
      }
    } else {
      // No Netatmo indoor module configured, use local sensor
      readIndoorTemperature(shouldRound);
    }
  } else if (tempSource == 2) { // Netatmo only
    // Only use Netatmo if configured
    if (strlen(netatmoIndoorModuleId) > 0) {
      fetchNetatmoIndoorTemperature();
      if (netatmoIndoorTempAvailable) {
        // Convert Netatmo temperature to the same format as local sensor
        indoorTemp = formatTemperature(netatmoIndoorTemp, shouldRound);
        indoorTemp += "º";
        indoorTempAvailable = true;
      } else {
        indoorTempAvailable = false;
      }
    } else {
      indoorTempAvailable = false;
    }
  }
  
  // Fetch outdoor temperature from Netatmo
  fetchOutdoorTemperature(shouldRound);
  
  // Set weatherFetched flag if at least one temperature is available
  weatherFetched = indoorTempAvailable || outdoorTempAvailable;
}

void setup() {
  Serial.begin(115200);
  delay(500); // Give serial monitor time to connect
  Serial.println();
  Serial.println(F("[SETUP] Starting setup..."));
  P.begin();
  P.setFont(mFactory); // Custom font
  loadConfig(); // Load config before setting intensity & flip
  P.setIntensity(brightness);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);
  Serial.println(F("[SETUP] Parola (LED Matrix) initialized"));
  
  // Check Netatmo configuration
  checkNetatmoConfig();
  
  // Initialize DS18B20 temperature sensor
  sensors.begin();
  Serial.println(F("[SETUP] DS18B20 sensor initialized"));
  
  connectWiFi();
  Serial.println(F("[SETUP] Wifi connected"));  
  setupWebServer();
  Serial.println(F("[SETUP] Webserver setup complete"));  
  Serial.println(F("[SETUP] Setup complete"));
  Serial.println();
  printConfigToSerial();
  setupTime(); // Start NTP sync process
  displayMode = 0;
  lastSwitch = millis();
  lastColonBlink = millis();
  indicatorVisible = true;
}

void loop() {
  // Update mDNS responder
  if (WiFi.status() == WL_CONNECTED && !isAPMode) {
    MDNS.update();
  }

  // --- AP Mode Animation ---
  static unsigned long apAnimTimer = 0;
  static int apAnimFrame = 0;
  if (isAPMode) {
    unsigned long now = millis();
    if (now - apAnimTimer > 750) {
      apAnimTimer = now;
      apAnimFrame++;
    }
    P.setTextAlignment(PA_CENTER);
    switch (apAnimFrame % 3) {
      case 0: P.print("AP©"); break;
      case 1: P.print("APª"); break;
      case 2: P.print("AP«"); break;
    }
    yield(); // Let system do background work
    return;  // Don't run normal display logic
  }

  static bool colonVisible = true;
  const unsigned long colonBlinkInterval = 800;
  if (millis() - lastColonBlink > colonBlinkInterval) {
    colonVisible = !colonVisible;
    indicatorVisible = colonVisible; // Sync temperature indicators with colon
    lastColonBlink = millis();
  }

  static unsigned long ntpAnimTimer = 0;
  static int ntpAnimFrame = 0;
  static bool tzSetAfterSync = false;
  static time_t lastPrint = 0;

  // TEMPERATURE UPDATE
  static unsigned long lastTempUpdate = 0;
  const unsigned long tempUpdateInterval = 300000; // 5 minutes

  // NTP state machine
  switch (ntpState) {
    case NTP_IDLE:
      break;
    case NTP_SYNCING: {
      time_t now = time(nullptr);
      if (now > 1000) {
        Serial.println(F("\n[TIME] NTP sync successful."));
        ntpSyncSuccessful = true;
        ntpState = NTP_SUCCESS;
      } else if (millis() - ntpStartTime > ntpTimeout || ntpRetryCount > maxNtpRetries) {
        Serial.println(F("\n[TIME] NTP sync failed."));
        ntpSyncSuccessful = false;
        ntpState = NTP_FAILED;
      } else {
        if (millis() - ntpStartTime > (ntpRetryCount * 1000)) {
          Serial.print(".");
          ntpRetryCount++;
        }
      }
      break;
    }
    case NTP_SUCCESS:
      if (!tzSetAfterSync) {
        const char* posixTz = ianaToPosix(timeZone);
        setenv("TZ", posixTz, 1);
        tzset();
        tzSetAfterSync = true;
      }
      ntpAnimTimer = 0;
      ntpAnimFrame = 0;
      break;
    case NTP_FAILED:
      ntpAnimTimer = 0;
      ntpAnimFrame = 0;
      break;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    if (!weatherFetchInitiated) {
        weatherFetchInitiated = true;
        updateTemperatures();
        lastTempUpdate = millis();
    }
    
    // Check if we need to refresh the token sooner due to an expired token
    if (needTokenRefresh) {
        // If we had a blocked request, wait longer (2 minutes)
        if (lastBlockedRequest > 0) {
            if (millis() - lastBlockedRequest > 120000) { // 2 minutes after blocked request
                Serial.println(F("[LOOP] Retry after blocked request, updating temperatures..."));
                weatherFetched = false;
                updateTemperatures();
                lastTempUpdate = millis();
                needTokenRefresh = false; // Reset the flag
                lastBlockedRequest = 0;   // Reset the blocked request timestamp
            }
        } 
        // Otherwise, just wait 30 seconds
        else if (millis() - lastTempUpdate > 30000) { // 30 seconds after last attempt
            Serial.println(F("[LOOP] Token refresh needed, updating temperatures..."));
            weatherFetched = false;
            updateTemperatures();
            lastTempUpdate = millis();
            needTokenRefresh = false; // Reset the flag
        }
    }
    // Regular temperature update interval
    else if (millis() - lastTempUpdate > tempUpdateInterval) {
        Serial.println(F("[LOOP] Updating temperatures..."));
        weatherFetched = false;
        updateTemperatures();
        lastTempUpdate = millis();
    }
  } else {
    weatherFetchInitiated = false;
  }

  // Time display logic
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  int dayOfWeek = timeinfo.tm_wday;
  char* daySymbol = daysOfTheWeek[dayOfWeek];

  char timeStr[9]; // enough for "12:34 AM"
  if (twelveHourToggle) {
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    sprintf(timeStr, "%d:%02d", hour12, timeinfo.tm_min);
  } else {
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  // Only prepend day symbol if showDayOfWeek is true
  String formattedTime;
  if (showDayOfWeek) {
    formattedTime = String(daySymbol) + " " + String(timeStr);
  } else {
    formattedTime = String(timeStr);
  }

  unsigned long displayDuration = (displayMode == 0) ? clockDuration : weatherDuration;
  if (millis() - lastSwitch > displayDuration) {
    displayMode = (displayMode + 1) % 2;
    lastSwitch = millis();
    Serial.printf("[LOOP] Switching to display mode: %s\n", displayMode == 0 ? "CLOCK" : "TEMPERATURE");
  }

  P.setTextAlignment(PA_CENTER);
  static bool tempWasAvailable = false;

  if (displayMode == 0) { // Clock
    if (ntpState == NTP_SYNCING) {
      if (millis() - ntpAnimTimer > 750) {
        ntpAnimTimer = millis();
        switch (ntpAnimFrame % 3) {
          case 0: P.print("sync®"); break;
          case 1: P.print("sync¯"); break;
          case 2: P.print("sync°"); break;
        }
        ntpAnimFrame++;
      }
    } else if (!ntpSyncSuccessful) {
      P.print("no ntp");
    } else {
      String timeString = formattedTime;
      if (!colonVisible) timeString.replace(":", " ");
      P.print(timeString);
    }
  } else { // Temperature mode
    if (indoorTempAvailable || outdoorTempAvailable) {
      // --- Temperature display string ---
      String tempDisplay;
      
      if (showIndoorTemp && indoorTempAvailable && showOutdoorTemp && outdoorTempAvailable) {
        // Show both temperatures with a separator that blinks but maintains alignment
        // Use a custom invisible separator with the same width as the vertical bar
        tempDisplay = indoorTemp + (indicatorVisible ? "|" : "}") + outdoorTemp;
      } else if (showIndoorTemp && indoorTempAvailable) {
        // Show only indoor temperature with lowercase i that blinks
        tempDisplay = (indicatorVisible ? "i " : "  ") + indoorTemp + tempSymbol;
      } else if (showOutdoorTemp && outdoorTempAvailable) {
        // Show only outdoor temperature with lowercase o that blinks
        tempDisplay = (indicatorVisible ? "o " : "  ") + outdoorTemp + tempSymbol;
      } else {
        // Fallback to clock if no temperatures available
        String timeString = formattedTime;
        if (!colonVisible) timeString.replace(":", " ");
        P.print(timeString);
        return;
      }
      
      P.print(tempDisplay.c_str());
      tempWasAvailable = true;
    } else {
      if (tempWasAvailable) {
        Serial.println(F("[DISPLAY] Temperatures not available, showing clock..."));
        tempWasAvailable = false;
      }
      if (ntpSyncSuccessful) {
        String timeString = formattedTime;
        if (!colonVisible) timeString.replace(":", " ");
        P.print(timeString);
      } else {
        P.print("no temp");
      }
    }
  }

  static unsigned long lastDisplayUpdate = 0;
  const unsigned long displayUpdateInterval = 50;
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();
  }

  yield();
}