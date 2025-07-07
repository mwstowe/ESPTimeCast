// Function to fetch Netatmo stations and devices
String fetchNetatmoDevices() {
  Serial.println(F("[NETATMO] Fetching stations and devices..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    return "{\"error\":\"WiFi not connected\"}";
  }
  
  // Get access token from config
  if (!LittleFS.begin()) {
    return "{\"error\":\"Failed to mount file system\"}";
  }
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    return "{\"error\":\"Failed to open config file\"}";
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  
  if (err) {
    return "{\"error\":\"Failed to parse config file\"}";
  }
  
  String accessToken = doc["netatmoAccessToken"].as<String>();
  
  if (accessToken.length() == 0) {
    return "{\"error\":\"No access token available. Please authorize with Netatmo first.\"}";
  }
  
  // Make API request to get devices
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, "https://api.netatmo.com/api/getstationsdata");
  http.addHeader("Authorization", "Bearer " + accessToken);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_UNAUTHORIZED) {
    Serial.println(F("[NETATMO] Token expired, refreshing"));
    
    // Try to refresh the token
    if (refreshNetatmoToken()) {
      // Get the new access token
      File f = LittleFS.open("/config.json", "r");
      if (!f) {
        http.end();
        return "{\"error\":\"Failed to open config file after token refresh\"}";
      }
      
      DynamicJsonDocument doc(2048);
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      
      if (err) {
        http.end();
        return "{\"error\":\"Failed to parse config file after token refresh\"}";
      }
      
      accessToken = doc["netatmoAccessToken"].as<String>();
      
      // Retry the request with the new token
      http.end();
      http.begin(client, "https://api.netatmo.com/api/getstationsdata");
      http.addHeader("Authorization", "Bearer " + accessToken);
      
      httpCode = http.GET();
    } else {
      http.end();
      return "{\"error\":\"Failed to refresh token\"}";
    }
  }
  
  if (httpCode != HTTP_CODE_OK) {
    String errorPayload = http.getString();
    Serial.print(F("[NETATMO] API request failed: "));
    Serial.println(httpCode);
    Serial.print(F("[NETATMO] Response: "));
    Serial.println(errorPayload);
    http.end();
    return "{\"error\":\"API request failed: " + String(httpCode) + "\"}";
  }
  
  String payload = http.getString();
  http.end();
  
  // Parse the response
  DynamicJsonDocument apiDoc(8192);
  DeserializationError apiErr = deserializeJson(apiDoc, payload);
  
  if (apiErr) {
    Serial.print(F("[NETATMO] Failed to parse API response: "));
    Serial.println(apiErr.c_str());
    return "{\"error\":\"Failed to parse API response\"}";
  }
  
  // Check for API error
  if (apiDoc["status"] != "ok") {
    String errorMsg = apiDoc["error"]["message"].as<String>();
    Serial.print(F("[NETATMO] API error: "));
    Serial.println(errorMsg);
    return "{\"error\":\"API error: " + errorMsg + "\"}";
  }
  
  // Extract devices
  JsonArray devices = apiDoc["body"]["devices"];
  
  // Create a simplified response
  DynamicJsonDocument responseDoc(4096);
  JsonArray responseDevices = responseDoc.createNestedArray("devices");
  
  for (JsonObject device : devices) {
    JsonObject responseDevice = responseDevices.createNestedObject();
    
    responseDevice["id"] = device["_id"].as<String>();
    responseDevice["name"] = device["station_name"].as<String>();
    responseDevice["type"] = device["type"].as<String>();
    
    // Add the main device as a module option (for indoor temperature)
    JsonArray responseModules = responseDevice.createNestedArray("modules");
    JsonObject mainModule = responseModules.createNestedObject();
    mainModule["id"] = device["_id"].as<String>();
    mainModule["name"] = "Main Station";
    mainModule["type"] = "NAMain";
    
    // Add modules
    if (device.containsKey("modules")) {
      JsonArray modules = device["modules"];
      
      for (JsonObject module : modules) {
        JsonObject responseModule = responseModules.createNestedObject();
        
        responseModule["id"] = module["_id"].as<String>();
        responseModule["name"] = module["module_name"].as<String>();
        responseModule["type"] = module["type"].as<String>();
      }
    }
  }
  
  String response;
  serializeJson(responseDoc, response);
  return response;
}

// Helper function to parse Netatmo JSON response
bool parseNetatmoJson(String &payload, DynamicJsonDocument &doc) {
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("[NETATMO] JSON parsing failed: "));
    Serial.println(error.c_str());
    return false;
  }
  return true;
}
