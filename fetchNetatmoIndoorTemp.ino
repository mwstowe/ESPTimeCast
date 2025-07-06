// Function to fetch indoor temperature from Netatmo
void fetchNetatmoIndoorTemperature() {
  Serial.println(F("\n[NETATMO] Fetching indoor temperature..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    netatmoIndoorTempAvailable = false;
    return;
  }
  
  if (strlen(netatmoDeviceId) == 0 || strlen(netatmoIndoorModuleId) == 0) {
    Serial.println(F("[NETATMO] Skipped: Missing device or indoor module ID"));
    Serial.print(F("[NETATMO] Device ID: '"));
    Serial.print(netatmoDeviceId);
    Serial.println(F("'"));
    Serial.print(F("[NETATMO] Indoor Module ID: '"));
    Serial.print(netatmoIndoorModuleId);
    Serial.println(F("'"));
    netatmoIndoorTempAvailable = false;
    return;
  }
  
  String token = getNetatmoToken();
  if (token.length() == 0) {
    Serial.println(F("[NETATMO] Skipped: No access token available"));
    netatmoIndoorTempAvailable = false;
    return;
  }
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  
  // Try using the getstationsdata endpoint instead of getmeasure
  String url = "https://api.netatmo.com/api/getstationsdata?device_id=";
  url += netatmoDeviceId;
  
  Serial.print(F("[NETATMO] GET "));
  Serial.println(url);
  Serial.print(F("[NETATMO] Using token: "));
  Serial.println(token.substring(0, 10) + "...");
  
  if (https.begin(*client, url)) {
    https.addHeader("Authorization", "Bearer " + token);
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "ESPTimeCast/1.0");
    
    Serial.println(F("[NETATMO] Sending request..."));
    int httpCode = https.GET();
    Serial.print(F("[NETATMO] HTTP response code: "));
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      Serial.println(F("[NETATMO] Response:"));
      Serial.println(payload);
      
      DynamicJsonDocument doc(8192); // Increased from 4096 to 8192
      
      if (parseNetatmoJson(payload, doc)) {
        // Navigate through the JSON to find the indoor module
        JsonArray devices = doc["body"]["devices"];
        
        Serial.print(F("[NETATMO] Number of devices found: "));
        Serial.println(devices.size());
        
        bool deviceFound = false;
        bool moduleFound = false;
        
        for (JsonObject device : devices) {
          String deviceId = device["_id"].as<String>();
          Serial.print(F("[NETATMO] Found device ID: "));
          Serial.println(deviceId);
          
          if (deviceId == netatmoDeviceId) {
            deviceFound = true;
            Serial.println(F("[NETATMO] Device ID matched!"));
            
            // First check if the indoor module is the main device itself
            if (String(netatmoIndoorModuleId) == deviceId) {
              moduleFound = true;
              Serial.println(F("[NETATMO] Indoor module is the main device"));
              
              JsonObject dashboard_data = device["dashboard_data"];
              
              if (dashboard_data.containsKey("Temperature")) {
                float temp = dashboard_data["Temperature"];
                netatmoIndoorTemp = temp;
                netatmoIndoorTempAvailable = true;
                Serial.print(F("[NETATMO] Indoor temperature: "));
                Serial.print(netatmoIndoorTemp);
                Serial.println(F("°C"));
                return;
              } else {
                Serial.println(F("[NETATMO] Temperature data not found in dashboard_data"));
                Serial.println(F("[NETATMO] Available dashboard_data fields:"));
                for (JsonPair kv : dashboard_data) {
                  Serial.print(kv.key().c_str());
                  Serial.print(F(": "));
                  Serial.println(kv.value().as<String>());
                }
              }
            }
            
            // If not found in main device, check modules
            if (!moduleFound) {
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
                
                if (moduleId == String(netatmoIndoorModuleId)) {
                  moduleFound = true;
                  Serial.println(F("[NETATMO] Indoor module ID matched!"));
                  
                  JsonObject dashboard_data = module["dashboard_data"];
                  
                  if (dashboard_data.containsKey("Temperature")) {
                    float temp = dashboard_data["Temperature"];
                    netatmoIndoorTemp = temp;
                    netatmoIndoorTempAvailable = true;
                    Serial.print(F("[NETATMO] Indoor temperature: "));
                    Serial.print(netatmoIndoorTemp);
                    Serial.println(F("°C"));
                    return;
                  } else {
                    Serial.println(F("[NETATMO] Temperature data not found in dashboard_data"));
                    Serial.println(F("[NETATMO] Available dashboard_data fields:"));
                    for (JsonPair kv : dashboard_data) {
                      Serial.print(kv.key().c_str());
                      Serial.print(F(": "));
                      Serial.println(kv.value().as<String>());
                    }
                  }
                  break;
                }
              }
            }
            
            if (!moduleFound) {
              Serial.println(F("[NETATMO] Error: Indoor module ID not found in the device's modules"));
              netatmoIndoorTempAvailable = false;
            }
            break;
          }
        }
        
        if (!deviceFound) {
          Serial.println(F("[NETATMO] Error: Device ID not found in the response"));
          netatmoIndoorTempAvailable = false;
        }
      } else {
        Serial.println(F("[NETATMO] JSON parsing failed"));
        netatmoIndoorTempAvailable = false;
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
      
      netatmoIndoorTempAvailable = false;
    }
    
    https.end();
  } else {
    Serial.println(F("[NETATMO] Unable to connect to Netatmo API"));
    netatmoIndoorTempAvailable = false;
  }
}
