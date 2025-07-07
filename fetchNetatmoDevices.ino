// Function to fetch Netatmo stations and devices
String fetchNetatmoDevices() {
  Serial.println(F("[NETATMO] Fetching stations and devices..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    return "{\"error\":\"WiFi not connected\"}";
  }
  
  // Get access token from config with error handling
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] Failed to mount file system"));
    return "{\"error\":\"Failed to mount file system\"}";
  }
  
  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[NETATMO] Config file does not exist"));
    return "{\"error\":\"Configuration file not found\"}";
  }
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println(F("[NETATMO] Failed to open config file"));
    return "{\"error\":\"Failed to open config file\"}";
  }
  
  size_t fileSize = f.size();
  if (fileSize == 0) {
    f.close();
    Serial.println(F("[NETATMO] Config file is empty"));
    return "{\"error\":\"Configuration file is empty\"}";
  }
  
  // Read the file content
  String fileContent = f.readString();
  f.close();
  
  // Parse the JSON
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, fileContent);
  
  if (err) {
    Serial.print(F("[NETATMO] Failed to parse config file: "));
    Serial.println(err.c_str());
    return "{\"error\":\"Failed to parse config file: " + String(err.c_str()) + "\"}";
  }
  
  // Check if access token exists
  if (!doc.containsKey("netatmoAccessToken") || doc["netatmoAccessToken"].as<String>().length() == 0) {
    Serial.println(F("[NETATMO] No access token available"));
    return "{\"error\":\"No access token available. Please authorize with Netatmo first.\"}";
  }
  
  String accessToken = doc["netatmoAccessToken"].as<String>();
  
  // Make API request to get devices with timeout protection
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, "https://api.netatmo.com/api/getstationsdata");
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.setTimeout(10000); // 10 second timeout
  
  Serial.println(F("[NETATMO] Sending request to Netatmo API"));
  int httpCode = http.GET();
  Serial.print(F("[NETATMO] HTTP response code: "));
  Serial.println(httpCode);
  
  if (httpCode == HTTP_CODE_UNAUTHORIZED) {
    Serial.println(F("[NETATMO] Token expired, refreshing"));
    
    // Try to refresh the token
    if (refreshNetatmoToken()) {
      // Get the new access token
      File f = LittleFS.open("/config.json", "r");
      if (!f) {
        http.end();
        Serial.println(F("[NETATMO] Failed to open config file after token refresh"));
        return "{\"error\":\"Failed to open config file after token refresh\"}";
      }
      
      String fileContent = f.readString();
      f.close();
      
      DynamicJsonDocument refreshDoc(2048);
      DeserializationError refreshErr = deserializeJson(refreshDoc, fileContent);
      
      if (refreshErr) {
        http.end();
        Serial.println(F("[NETATMO] Failed to parse config file after token refresh"));
        return "{\"error\":\"Failed to parse config file after token refresh\"}";
      }
      
      accessToken = refreshDoc["netatmoAccessToken"].as<String>();
      
      // Retry the request with the new token
      http.end();
      http.begin(client, "https://api.netatmo.com/api/getstationsdata");
      http.addHeader("Authorization", "Bearer " + accessToken);
      http.setTimeout(10000); // 10 second timeout
      
      Serial.println(F("[NETATMO] Retrying request with new token"));
      httpCode = http.GET();
      Serial.print(F("[NETATMO] HTTP response code after refresh: "));
      Serial.println(httpCode);
    } else {
      http.end();
      Serial.println(F("[NETATMO] Failed to refresh token"));
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
  
  Serial.println(F("[NETATMO] Response received, parsing..."));
  
  // Parse the response with error handling
  DynamicJsonDocument apiDoc(8192);
  DeserializationError apiErr = deserializeJson(apiDoc, payload);
  
  if (apiErr) {
    Serial.print(F("[NETATMO] Failed to parse API response: "));
    Serial.println(apiErr.c_str());
    return "{\"error\":\"Failed to parse API response: " + String(apiErr.c_str()) + "\"}";
  }
  
  // Check for API error
  if (!apiDoc.containsKey("status") || apiDoc["status"] != "ok") {
    String errorMsg = apiDoc.containsKey("error") && apiDoc["error"].containsKey("message") ? 
                      apiDoc["error"]["message"].as<String>() : "Unknown error";
    Serial.print(F("[NETATMO] API error: "));
    Serial.println(errorMsg);
    return "{\"error\":\"API error: " + errorMsg + "\"}";
  }
  
  // Check if body and devices exist
  if (!apiDoc.containsKey("body") || !apiDoc["body"].containsKey("devices")) {
    Serial.println(F("[NETATMO] No devices found in response"));
    return "{\"error\":\"No devices found in response\"}";
  }
  
  // Extract devices
  JsonArray devices = apiDoc["body"]["devices"];
  
  // Create a simplified response
  DynamicJsonDocument responseDoc(4096);
  JsonArray responseDevices = responseDoc.createNestedArray("devices");
  
  Serial.print(F("[NETATMO] Found "));
  Serial.print(devices.size());
  Serial.println(F(" devices"));
  
  for (JsonObject device : devices) {
    if (!device.containsKey("_id") || !device.containsKey("station_name")) {
      Serial.println(F("[NETATMO] Skipping device with missing ID or name"));
      continue;
    }
    
    JsonObject responseDevice = responseDevices.createNestedObject();
    
    responseDevice["id"] = device["_id"].as<String>();
    responseDevice["name"] = device["station_name"].as<String>();
    responseDevice["type"] = device.containsKey("type") ? device["type"].as<String>() : "unknown";
    
    // Add the main device as a module option (for indoor temperature)
    JsonArray responseModules = responseDevice.createNestedArray("modules");
    JsonObject mainModule = responseModules.createNestedObject();
    mainModule["id"] = device["_id"].as<String>();
    mainModule["name"] = "Main Station";
    mainModule["type"] = "NAMain";
    
    // Add modules
    if (device.containsKey("modules")) {
      JsonArray modules = device["modules"];
      
      Serial.print(F("[NETATMO] Found "));
      Serial.print(modules.size());
      Serial.println(F(" modules for this device"));
      
      for (JsonObject module : modules) {
        if (!module.containsKey("_id") || !module.containsKey("module_name") || !module.containsKey("type")) {
          Serial.println(F("[NETATMO] Skipping module with missing ID, name, or type"));
          continue;
        }
        
        JsonObject responseModule = responseModules.createNestedObject();
        
        responseModule["id"] = module["_id"].as<String>();
        responseModule["name"] = module["module_name"].as<String>();
        responseModule["type"] = module["type"].as<String>();
      }
    }
  }
  
  // Serialize the response
  String response;
  serializeJson(responseDoc, response);
  
  Serial.println(F("[NETATMO] Successfully processed devices"));
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
