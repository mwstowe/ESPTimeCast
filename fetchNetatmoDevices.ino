// Function to fetch Netatmo stations and devices
String fetchNetatmoDevices() {
  Serial.println(F("[NETATMO] Fetching stations and devices..."));
  Serial.print(F("[MEMORY] Free heap before fetch: "));
  Serial.println(ESP.getFreeHeap());
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    return "{\"error\":\"WiFi not connected\"}";
  }
  
  // Get access token from config
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] Failed to mount file system"));
    return "{\"error\":\"Failed to mount file system\"}";
  }
  
  // Read the access token directly from the file to avoid memory issues
  String accessToken = "";
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println(F("[NETATMO] Failed to open config file"));
    return "{\"error\":\"Failed to open config file\"}";
  }
  
  // Read the file line by line to find the access token
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (line.indexOf("\"netatmoAccessToken\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      accessToken = line.substring(startPos, endPos);
      accessToken.trim();
      
      // Remove quotes if present
      if (accessToken.startsWith("\"") && accessToken.endsWith("\"")) {
        accessToken = accessToken.substring(1, accessToken.length() - 1);
      }
      
      break;
    }
  }
  
  f.close();
  
  if (accessToken.length() == 0) {
    Serial.println(F("[NETATMO] No access token available"));
    return "{\"error\":\"No access token available. Please authorize with Netatmo first.\"}";
  }
  
  Serial.println(F("[NETATMO] Access token found"));
  
  // Make API request to get devices
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  Serial.println(F("[NETATMO] Connecting to api.netatmo.com..."));
  
  if (!client.connect("api.netatmo.com", 443)) {
    Serial.println(F("[NETATMO] Connection failed"));
    return "{\"error\":\"Failed to connect to Netatmo API\"}";
  }
  
  Serial.println(F("[NETATMO] Connected to api.netatmo.com"));
  Serial.print(F("[MEMORY] Free heap after connection: "));
  Serial.println(ESP.getFreeHeap());
  
  // Prepare the HTTP request
  String request = "GET /api/getstationsdata HTTP/1.1\r\n";
  request += "Host: api.netatmo.com\r\n";
  request += "Authorization: Bearer " + accessToken + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  
  Serial.println(F("[NETATMO] Sending request..."));
  client.print(request);
  
  Serial.print(F("[MEMORY] Free heap after request: "));
  Serial.println(ESP.getFreeHeap());
  
  // Wait for the response
  unsigned long timeout = millis() + 10000; // 10 second timeout
  while (client.available() == 0) {
    if (millis() > timeout) {
      Serial.println(F("[NETATMO] Client timeout"));
      client.stop();
      return "{\"error\":\"Request timeout\"}";
    }
    delay(10);
  }
  
  Serial.println(F("[NETATMO] Response received"));
  
  // Check the response status
  String status_line = client.readStringUntil('\n');
  Serial.print(F("[NETATMO] Status: "));
  Serial.println(status_line);
  
  // Skip HTTP headers
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  // Check if we need to refresh the token
  if (status_line.indexOf("401") > 0) {
    client.stop();
    Serial.println(F("[NETATMO] Token expired, refreshing"));
    
    // Try to refresh the token
    if (refreshNetatmoToken()) {
      // Try again with the new token
      return fetchNetatmoDevices();
    } else {
      return "{\"error\":\"Failed to refresh token\"}";
    }
  }
  
  // Check for other errors
  if (status_line.indexOf("200") <= 0) {
    client.stop();
    Serial.print(F("[NETATMO] HTTP error: "));
    Serial.println(status_line);
    return "{\"error\":\"HTTP error: " + status_line + "\"}";
  }
  
  // Process the response in chunks to avoid memory issues
  Serial.println(F("[NETATMO] Processing response..."));
  
  // Create a simplified response
  String simpleResponse = "{\"devices\":[";
  bool firstDevice = true;
  
  // Variables for parsing
  bool inDevices = false;
  bool inDevice = false;
  bool inModules = false;
  bool inModule = false;
  
  String currentDeviceId = "";
  String currentDeviceName = "";
  String currentDeviceType = "";
  
  String currentModuleId = "";
  String currentModuleName = "";
  String currentModuleType = "";
  
  // Buffer for reading
  const int bufferSize = 128;
  char buffer[bufferSize];
  int bufferPos = 0;
  
  // Read the response in chunks
  while (client.available()) {
    char c = client.read();
    
    // Add to buffer
    buffer[bufferPos++] = c;
    
    // Process buffer when full or at end of line
    if (bufferPos >= bufferSize - 1 || c == '\n') {
      buffer[bufferPos] = '\0'; // Null terminate
      String chunk = String(buffer);
      bufferPos = 0;
      
      // Look for device info
      if (chunk.indexOf("\"devices\"") >= 0) {
        inDevices = true;
      }
      
      if (inDevices) {
        // Start of a device
        if (chunk.indexOf("\"_id\"") >= 0 && !inDevice) {
          inDevice = true;
          
          // Extract device ID
          int idStart = chunk.indexOf("\"") + 1;
          int idEnd = chunk.indexOf("\"", idStart);
          if (idStart > 0 && idEnd > idStart) {
            currentDeviceId = chunk.substring(idStart, idEnd);
          }
        }
        
        // Device name
        if (inDevice && chunk.indexOf("\"station_name\"") >= 0) {
          int nameStart = chunk.indexOf("\"", chunk.indexOf(":")) + 1;
          int nameEnd = chunk.indexOf("\"", nameStart);
          if (nameStart > 0 && nameEnd > nameStart) {
            currentDeviceName = chunk.substring(nameStart, nameEnd);
          }
        }
        
        // Device type
        if (inDevice && chunk.indexOf("\"type\"") >= 0) {
          int typeStart = chunk.indexOf("\"", chunk.indexOf(":")) + 1;
          int typeEnd = chunk.indexOf("\"", typeStart);
          if (typeStart > 0 && typeEnd > typeStart) {
            currentDeviceType = chunk.substring(typeStart, typeEnd);
          }
        }
        
        // Start of modules array
        if (inDevice && chunk.indexOf("\"modules\"") >= 0) {
          inModules = true;
          
          // Add the device to the response
          if (currentDeviceId.length() > 0) {
            if (!firstDevice) {
              simpleResponse += ",";
            }
            firstDevice = false;
            
            simpleResponse += "{\"id\":\"" + currentDeviceId + "\",";
            simpleResponse += "\"name\":\"" + currentDeviceName + "\",";
            simpleResponse += "\"type\":\"" + currentDeviceType + "\",";
            simpleResponse += "\"modules\":[";
            
            // Add the main device as a module
            simpleResponse += "{\"id\":\"" + currentDeviceId + "\",";
            simpleResponse += "\"name\":\"Main Station\",";
            simpleResponse += "\"type\":\"NAMain\"}";
          }
        }
        
        // Start of a module
        if (inModules && chunk.indexOf("\"_id\"") >= 0 && !inModule) {
          inModule = true;
          
          // Extract module ID
          int idStart = chunk.indexOf("\"", chunk.indexOf(":")) + 1;
          int idEnd = chunk.indexOf("\"", idStart);
          if (idStart > 0 && idEnd > idStart) {
            currentModuleId = chunk.substring(idStart, idEnd);
          }
        }
        
        // Module name
        if (inModule && chunk.indexOf("\"module_name\"") >= 0) {
          int nameStart = chunk.indexOf("\"", chunk.indexOf(":")) + 1;
          int nameEnd = chunk.indexOf("\"", nameStart);
          if (nameStart > 0 && nameEnd > nameStart) {
            currentModuleName = chunk.substring(nameStart, nameEnd);
          }
        }
        
        // Module type
        if (inModule && chunk.indexOf("\"type\"") >= 0) {
          int typeStart = chunk.indexOf("\"", chunk.indexOf(":")) + 1;
          int typeEnd = chunk.indexOf("\"", typeStart);
          if (typeStart > 0 && typeEnd > typeStart) {
            currentModuleType = chunk.substring(typeStart, typeEnd);
          }
        }
        
        // End of a module
        if (inModule && chunk.indexOf("}") >= 0) {
          // Add the module to the response
          if (currentModuleId.length() > 0) {
            simpleResponse += ",{\"id\":\"" + currentModuleId + "\",";
            simpleResponse += "\"name\":\"" + currentModuleName + "\",";
            simpleResponse += "\"type\":\"" + currentModuleType + "\"}";
          }
          
          // Reset module info
          currentModuleId = "";
          currentModuleName = "";
          currentModuleType = "";
          inModule = false;
        }
        
        // End of modules array
        if (inModules && chunk.indexOf("]") >= 0) {
          simpleResponse += "]}";
          inModules = false;
        }
        
        // End of a device
        if (inDevice && chunk.indexOf("}") >= 0 && !inModules) {
          // Reset device info
          currentDeviceId = "";
          currentDeviceName = "";
          currentDeviceType = "";
          inDevice = false;
        }
        
        // End of devices array
        if (inDevices && chunk.indexOf("]") >= 0 && !inDevice) {
          inDevices = false;
        }
      }
    }
  }
  
  client.stop();
  
  // Finalize the response
  simpleResponse += "]}";
  
  Serial.println(F("[NETATMO] Response processing complete"));
  Serial.print(F("[MEMORY] Free heap after processing: "));
  Serial.println(ESP.getFreeHeap());
  
  return simpleResponse;
}

// Function to refresh Netatmo token
bool refreshNetatmoToken() {
  Serial.println(F("[NETATMO] Refreshing token"));
  Serial.print(F("[MEMORY] Free heap before refresh: "));
  Serial.println(ESP.getFreeHeap());
  
  // Get refresh token from config
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] Failed to mount file system"));
    return false;
  }
  
  // Read the client ID, client secret, and refresh token directly from the file
  String clientId = "";
  String clientSecret = "";
  String refreshToken = "";
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println(F("[NETATMO] Failed to open config file"));
    return false;
  }
  
  // Read the file line by line
  while (f.available()) {
    String line = f.readStringUntil('\n');
    
    if (line.indexOf("\"netatmoClientId\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      clientId = line.substring(startPos, endPos);
      clientId.trim();
      
      // Remove quotes if present
      if (clientId.startsWith("\"") && clientId.endsWith("\"")) {
        clientId = clientId.substring(1, clientId.length() - 1);
      }
    }
    
    if (line.indexOf("\"netatmoClientSecret\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      clientSecret = line.substring(startPos, endPos);
      clientSecret.trim();
      
      // Remove quotes if present
      if (clientSecret.startsWith("\"") && clientSecret.endsWith("\"")) {
        clientSecret = clientSecret.substring(1, clientSecret.length() - 1);
      }
    }
    
    if (line.indexOf("\"netatmoRefreshToken\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      refreshToken = line.substring(startPos, endPos);
      refreshToken.trim();
      
      // Remove quotes if present
      if (refreshToken.startsWith("\"") && refreshToken.endsWith("\"")) {
        refreshToken = refreshToken.substring(1, refreshToken.length() - 1);
      }
    }
    
    // Break if we have all the values
    if (clientId.length() > 0 && clientSecret.length() > 0 && refreshToken.length() > 0) {
      break;
    }
  }
  
  f.close();
  
  if (clientId.length() == 0 || clientSecret.length() == 0 || refreshToken.length() == 0) {
    Serial.println(F("[NETATMO] Missing client credentials or refresh token"));
    return false;
  }
  
  // Exchange the refresh token for a new access token
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  Serial.println(F("[NETATMO] Connecting to api.netatmo.com..."));
  
  if (!client.connect("api.netatmo.com", 443)) {
    Serial.println(F("[NETATMO] Connection failed"));
    return false;
  }
  
  Serial.println(F("[NETATMO] Connected to api.netatmo.com"));
  
  // Prepare the POST data
  String postData = "grant_type=refresh_token";
  postData += "&client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&refresh_token=" + refreshToken;
  
  // Prepare the HTTP request
  String request = "POST /oauth2/token HTTP/1.1\r\n";
  request += "Host: api.netatmo.com\r\n";
  request += "Connection: close\r\n";
  request += "Content-Type: application/x-www-form-urlencoded\r\n";
  request += "Content-Length: " + String(postData.length()) + "\r\n";
  request += "\r\n";
  request += postData;
  
  Serial.println(F("[NETATMO] Sending token refresh request..."));
  client.print(request);
  
  // Wait for the response
  unsigned long timeout = millis() + 10000; // 10 second timeout
  while (client.available() == 0) {
    if (millis() > timeout) {
      Serial.println(F("[NETATMO] Client timeout"));
      client.stop();
      return false;
    }
    delay(10);
  }
  
  Serial.println(F("[NETATMO] Response received"));
  
  // Check the response status
  String status_line = client.readStringUntil('\n');
  Serial.print(F("[NETATMO] Status: "));
  Serial.println(status_line);
  
  // Skip HTTP headers
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  
  // Check for errors
  if (status_line.indexOf("200") <= 0) {
    client.stop();
    Serial.print(F("[NETATMO] HTTP error: "));
    Serial.println(status_line);
    return false;
  }
  
  // Read the response body
  String response = client.readString();
  client.stop();
  
  // Parse the response
  int accessTokenStart = response.indexOf("\"access_token\":\"");
  int newRefreshTokenStart = response.indexOf("\"refresh_token\":\"");
  
  if (accessTokenStart < 0) {
    Serial.println(F("[NETATMO] Failed to find access token in response"));
    return false;
  }
  
  accessTokenStart += 16; // Length of "\"access_token\":\""
  int accessTokenEnd = response.indexOf("\"", accessTokenStart);
  
  if (accessTokenEnd < 0) {
    Serial.println(F("[NETATMO] Failed to parse access token"));
    return false;
  }
  
  String accessToken = response.substring(accessTokenStart, accessTokenEnd);
  
  // Extract new refresh token if present
  String newRefreshToken = "";
  if (newRefreshTokenStart >= 0) {
    newRefreshTokenStart += 17; // Length of "\"refresh_token\":\""
    int newRefreshTokenEnd = response.indexOf("\"", newRefreshTokenStart);
    
    if (newRefreshTokenEnd >= 0) {
      newRefreshToken = response.substring(newRefreshTokenStart, newRefreshTokenEnd);
    }
  }
  
  Serial.println(F("[NETATMO] Successfully extracted tokens"));
  
  // Save tokens to config
  saveNetatmoTokens(accessToken, newRefreshToken.length() > 0 ? newRefreshToken : refreshToken);
  
  return true;
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
