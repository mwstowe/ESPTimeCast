// Function to fetch Netatmo stations and devices
String fetchNetatmoDevices() {
  Serial.println(F("[NETATMO] Fetching stations and devices..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    return "{\"error\":\"WiFi not connected\"}";
  }
  
  // Check if we have an access token
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] No access token available"));
    return "{\"error\":\"No access token available. Please authorize with Netatmo first.\"}";
  }
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  
  String url = "https://api.netatmo.com/api/getstationsdata";
  
  Serial.print(F("[NETATMO] Requesting URL: "));
  Serial.println(url);
  
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
      
      // Create a simplified response with just the device and module information
      DynamicJsonDocument fullDoc(8192);
      if (!parseNetatmoJson(payload, fullDoc)) {
        Serial.println(F("[NETATMO] Failed to parse response"));
        https.end();
        return "{\"error\":\"Failed to parse response\"}";
      }
      
      // Create a smaller document for the simplified response
      DynamicJsonDocument simpleDoc(4096);
      JsonArray devices = simpleDoc.createNestedArray("devices");
      
      // Process each device
      if (fullDoc.containsKey("body") && fullDoc["body"].containsKey("devices")) {
        for (JsonObject device : fullDoc["body"]["devices"].as<JsonArray>()) {
          if (device.containsKey("_id") && device.containsKey("station_name")) {
            JsonObject simpleDevice = devices.createNestedObject();
            simpleDevice["id"] = device["_id"].as<String>();
            simpleDevice["name"] = device["station_name"].as<String>();
            simpleDevice["type"] = "station";
            
            // Add the main device as a module option (for indoor temperature)
            JsonArray modules = simpleDevice.createNestedArray("modules");
            JsonObject mainModule = modules.createNestedObject();
            mainModule["id"] = device["_id"].as<String>();
            mainModule["name"] = "Main Station";
            mainModule["type"] = "NAMain";
            
            // Process modules
            if (device.containsKey("modules")) {
              for (JsonObject module : device["modules"].as<JsonArray>()) {
                if (module.containsKey("_id") && module.containsKey("module_name") && module.containsKey("type")) {
                  JsonObject simpleModule = modules.createNestedObject();
                  simpleModule["id"] = module["_id"].as<String>();
                  simpleModule["name"] = module["module_name"].as<String>();
                  simpleModule["type"] = module["type"].as<String>();
                }
              }
            }
          }
        }
      }
      
      // Serialize the simplified response
      String simpleResponse;
      serializeJson(simpleDoc, simpleResponse);
      https.end();
      return simpleResponse;
    } else {
      Serial.print(F("[NETATMO] HTTP error: "));
      Serial.println(httpCode);
      
      // Get the error response
      String errorPayload = https.getString();
      Serial.println(F("[NETATMO] Error response:"));
      Serial.println(errorPayload);
      
      // If unauthorized, clear the token
      if (httpCode == 401 || httpCode == 403) {
        Serial.println(F("[NETATMO] Token expired or invalid, clearing token"));
        netatmoAccessToken[0] = '\0';
        saveTokensToConfig();
      }
      
      https.end();
      return "{\"error\":\"HTTP error " + String(httpCode) + "\"}";
    }
  } else {
    Serial.println(F("[NETATMO] Unable to connect to Netatmo API"));
    return "{\"error\":\"Unable to connect to Netatmo API\"}";
  }
}
