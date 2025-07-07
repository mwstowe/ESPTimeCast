#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <memory>

// Function to fetch outdoor temperature from Netatmo
void fetchOutdoorTemperature(bool roundToInteger) {
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
      Serial.println(F("[NETATMO] Response received"));
      
      DynamicJsonDocument doc(8192);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        bool found = false;
        
        if (doc.containsKey("body") && doc["body"].containsKey("devices")) {
          JsonArray devices = doc["body"]["devices"];
          
          for (JsonObject device : devices) {
            if (device["_id"].as<String>() == netatmoDeviceId) {
              if (device.containsKey("modules")) {
                JsonArray modules = device["modules"];
                
                for (JsonObject module : modules) {
                  if (module["_id"].as<String>() == netatmoModuleId) {
                    if (module.containsKey("dashboard_data") && module["dashboard_data"].containsKey("Temperature")) {
                      float temp = module["dashboard_data"]["Temperature"];
                      Serial.print(F("[NETATMO] Raw outdoor temperature: "));
                      Serial.print(temp);
                      Serial.println(F("°C"));
                      
                      outdoorTemp = formatTemperature(temp, roundToInteger) + "º";
                      outdoorTempAvailable = true;
                      found = true;
                      
                      Serial.print(F("[NETATMO] Formatted outdoor temperature: "));
                      Serial.println(outdoorTemp);
                      break;
                    }
                  }
                }
              }
              break;
            }
          }
        }
        
        if (!found) {
          Serial.println(F("[NETATMO] Module not found in response"));
          outdoorTempAvailable = false;
        }
      } else {
        Serial.print(F("[NETATMO] JSON parse error: "));
        Serial.println(error.c_str());
        outdoorTempAvailable = false;
      }
    } else {
      Serial.print(F("[NETATMO] HTTP error: "));
      Serial.println(httpCode);
      
      // Get the error response
      String errorPayload = https.getString();
      Serial.println(F("[NETATMO] Error response:"));
      Serial.println(errorPayload);
      
      // If unauthorized, clear the token
      if (httpCode == 401 || httpCode == 403) {
        Serial.println(F("[NETATMO] 401/403 error - token expired or invalid"));
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
