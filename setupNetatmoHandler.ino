// Global variables for token exchange
static String pendingCode = "";
static bool tokenExchangePending = false;
static bool fetchDevicesPending = false;
static String deviceData = "";

// Helper function to URL encode a string (memory-efficient version)
String urlEncode(const char* input) {
  const char *hex = "0123456789ABCDEF";
  String result = "";
  
  while (*input) {
    char c = *input++;
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      result += c;
    } else {
      result += '%';
      result += hex[c >> 4];
      result += hex[c & 0xF];
    }
  }
  return result;
}

// Function to exchange authorization code for tokens
void exchangeAuthCode(const String &code) {
  Serial.println(F("[NETATMO] Starting token exchange process"));
  
  // This will be called in the next loop iteration
  pendingCode = code;
  
  // Set a flag to process this in the main loop
  tokenExchangePending = true;
}

// Function to setup Netatmo OAuth handler
void setupNetatmoHandler() {
  Serial.println(F("[NETATMO] Setting up Netatmo OAuth handler..."));
  
  // Check if LittleFS is mounted
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] ERROR: Failed to mount LittleFS"));
    return;
  }
  Serial.println(F("[NETATMO] LittleFS mounted successfully"));
  
  // Instead of creating new objects, use simpler approach
  Serial.println(F("[NETATMO] Setting up API endpoints"));
  
  // Endpoint to save Netatmo credentials without rebooting
  server.on("/api/netatmo/save-credentials", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling save credentials"));
    
    if (!request->hasParam("clientId", true) || !request->hasParam("clientSecret", true)) {
      Serial.println(F("[NETATMO] Error - Missing parameters"));
      request->send(400, "text/plain", "Missing parameters");
      return;
    }
    
    String clientId = request->getParam("clientId", true)->value();
    String clientSecret = request->getParam("clientSecret", true)->value();
    
    Serial.print(F("[NETATMO] Client ID: "));
    Serial.println(clientId);
    Serial.println(F("[NETATMO] Client Secret is set"));
    
    // Save to global variables
    strlcpy(netatmoClientId, clientId.c_str(), sizeof(netatmoClientId));
    strlcpy(netatmoClientSecret, clientSecret.c_str(), sizeof(netatmoClientSecret));
    
    // Save to config file
    saveTokensToConfig();
    
    // Send a simple response
    request->send(200, "application/json", "{\"success\":true}");
    
    Serial.println(F("[NETATMO] Credentials saved"));
  });
  
  // Endpoint to initiate OAuth flow
  server.on("/api/netatmo/auth", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling auth request"));
    
    if (strlen(netatmoClientId) == 0 || strlen(netatmoClientSecret) == 0) {
      Serial.println(F("[NETATMO] Error - No credentials"));
      request->send(400, "text/plain", "Missing credentials");
      return;
    }
    
    // Generate authorization URL with minimal memory usage
    String redirectUri = "http://" + WiFi.localIP().toString() + "/api/netatmo/callback";
    String authUrl = "https://api.netatmo.com/oauth2/authorize";
    authUrl += "?client_id=";
    authUrl += urlEncode(netatmoClientId);
    authUrl += "&redirect_uri=";
    authUrl += urlEncode(redirectUri.c_str());
    authUrl += "&scope=read_station%20read_thermostat&response_type=code";
    
    Serial.print(F("[NETATMO] Auth URL: "));
    Serial.println(authUrl);
    
    Serial.println(F("[NETATMO] Redirecting to auth page"));
    request->redirect(authUrl);
  });
  
  // Endpoint to handle OAuth callback
  server.on("/api/netatmo/callback", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling OAuth callback"));
    
    // Simple error handling to save memory
    if (request->hasParam("error")) {
      String error = request->getParam("error")->value();
      Serial.print(F("[NETATMO] OAuth error: "));
      Serial.println(error);
      
      // Simple HTML response to save memory
      String html = F("<html><body><h1>Auth Error</h1><p>Error: ");
      html += error;
      html += F("</p><a href='/netatmo.html'>Back</a></body></html>");
      
      request->send(200, "text/html", html);
      return;
    }
    
    if (!request->hasParam("code")) {
      Serial.println(F("[NETATMO] Error - No authorization code in callback"));
      request->send(400, "text/plain", "No code parameter");
      return;
    }
    
    String code = request->getParam("code")->value();
    Serial.print(F("[NETATMO] Received code: "));
    Serial.println(code);
    
    // Store the code for later processing
    pendingCode = code;
    tokenExchangePending = true;
    
    // Simple HTML response to save memory
    String html = F("<html><body><h1>Authorization Successful</h1>");
    html += F("<p>Exchanging code for tokens...</p>");
    html += F("<p>You will be redirected in 5 seconds.</p>");
    html += F("<meta http-equiv='refresh' content='5;url=/netatmo.html'>");
    html += F("</body></html>");
    
    request->send(200, "text/html", html);
  });
  
  // Endpoint to get Netatmo devices
  server.on("/api/netatmo/devices", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling get devices request"));
    
    if (strlen(netatmoAccessToken) == 0) {
      Serial.println(F("[NETATMO] Error - No access token"));
      request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
      return;
    }
    
    // Check if we should force a refresh
    bool forceRefresh = false;
    if (request->hasParam("refresh")) {
      forceRefresh = request->getParam("refresh")->value() == "true";
    }
    
    // If we already have device data and no refresh is requested, return it immediately
    if (deviceData.length() > 0 && !forceRefresh) {
      request->send(200, "application/json", deviceData);
      return;
    }
    
    // Otherwise, fetch it asynchronously
    fetchDevicesPending = true;
    
    // Send a simple response
    request->send(200, "application/json", "{\"body\":{\"devices\":[]},\"status\":\"fetching\"}");
  });
  
  // Endpoint to save Netatmo settings without rebooting
  server.on("/api/netatmo/save-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling save settings request"));
    
    // Process all parameters
    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter* p = request->getParam(i);
      String name = p->name();
      String value = p->value();
      
      if (name == "netatmoDeviceId") {
        strlcpy(netatmoDeviceId, value.c_str(), sizeof(netatmoDeviceId));
      }
      else if (name == "netatmoModuleId") {
        // Handle "none" value
        if (value == "none") {
          netatmoModuleId[0] = '\0'; // Empty string
        } else {
          strlcpy(netatmoModuleId, value.c_str(), sizeof(netatmoModuleId));
        }
      }
      else if (name == "netatmoIndoorModuleId") {
        // Handle "none" value
        if (value == "none") {
          netatmoIndoorModuleId[0] = '\0'; // Empty string
        } else {
          strlcpy(netatmoIndoorModuleId, value.c_str(), sizeof(netatmoIndoorModuleId));
        }
      }
      else if (name == "useNetatmoOutdoor") {
        useNetatmoOutdoor = (value == "true" || value == "on" || value == "1");
      }
      else if (name == "prioritizeNetatmoIndoor") {
        prioritizeNetatmoIndoor = (value == "true" || value == "on" || value == "1");
      }
    }
    
    // Save to config file
    saveTokensToConfig();
    
    // Send success response
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings saved\"}");
  });
  
  Serial.println(F("[NETATMO] OAuth handler setup complete"));
}

// Function to be called from loop() to process pending token exchange
void processTokenExchange() {
  if (!tokenExchangePending || pendingCode.isEmpty()) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing token exchange"));
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  tokenExchangePending = false;
  String code = pendingCode;
  pendingCode = "";
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  // Set up the HTTPS client with minimal memory usage
  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  Serial.println(F("[NETATMO] Connecting to token endpoint"));
  if (!https.begin(client, "https://api.netatmo.com/oauth2/token")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Construct the POST data with minimal memory usage
  String redirectUri = "http://" + WiFi.localIP().toString() + "/api/netatmo/callback";
  String postData = "grant_type=authorization_code";
  postData += "&client_id=";
  postData += urlEncode(netatmoClientId);
  postData += "&client_secret=";
  postData += urlEncode(netatmoClientSecret);
  postData += "&code=";
  postData += urlEncode(code.c_str());
  postData += "&redirect_uri=";
  postData += urlEncode(redirectUri.c_str());
  
  Serial.println(F("[NETATMO] Sending token request"));
  int httpCode = https.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    https.end();
    return;
  }
  
  // Get the response
  String response = https.getString();
  https.end();
  
  Serial.println(F("[NETATMO] Parsing response"));
  
  // Use a smaller JSON buffer
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.print(F("[NETATMO] JSON parse error: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Extract the tokens
  if (!doc.containsKey("access_token") || !doc.containsKey("refresh_token")) {
    Serial.println(F("[NETATMO] Missing tokens in response"));
    return;
  }
  
  const char* accessToken = doc["access_token"];
  const char* refreshToken = doc["refresh_token"];
  
  // Save the tokens
  Serial.println(F("[NETATMO] Saving tokens"));
  strlcpy(netatmoAccessToken, accessToken, sizeof(netatmoAccessToken));
  strlcpy(netatmoRefreshToken, refreshToken, sizeof(netatmoRefreshToken));
  
  saveTokensToConfig();
  Serial.println(F("[NETATMO] Token exchange complete"));
  
  // Trigger a device fetch after successful token exchange
  fetchDevicesPending = true;
}

// Function to be called from loop() to fetch Netatmo devices
void processFetchDevices() {
  if (!fetchDevicesPending) {
    return;
  }
  
  Serial.println(F("[NETATMO] Fetching Netatmo devices"));
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  fetchDevicesPending = false;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] Error - No access token"));
    return;
  }
  
  // Set up the HTTPS client with minimal memory usage
  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  // Use the correct API endpoint with explicit parameters
  Serial.println(F("[NETATMO] Connecting to Netatmo API"));
  
  // Print the access token (first few characters)
  Serial.print(F("[NETATMO] Using access token: "));
  if (strlen(netatmoAccessToken) > 10) {
    char tokenPrefix[11];
    strncpy(tokenPrefix, netatmoAccessToken, 10);
    tokenPrefix[10] = '\0';
    Serial.print(tokenPrefix);
    Serial.println(F("..."));
  } else {
    Serial.println(netatmoAccessToken);
  }
  
  // Try a different API endpoint
  if (!https.begin(client, "https://api.netatmo.com/api/devicelist")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  // Set the authorization header correctly
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  https.addHeader("Authorization", authHeader);
  https.addHeader("Accept", "application/json");
  
  Serial.println(F("[NETATMO] Sending request to /api/devicelist"));
  int httpCode = https.GET();
  
  Serial.print(F("[NETATMO] HTTP response code: "));
  Serial.println(httpCode);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    if (httpCode > 0) {
      Serial.println(F("[NETATMO] Response: "));
      Serial.println(https.getString());
    }
    https.end();
    return;
  }
  
  // Get the response
  String response = https.getString();
  https.end();
  
  Serial.println(F("[NETATMO] Received device data"));
  Serial.print(F("[NETATMO] Response size: "));
  Serial.println(response.length());
  
  if (response.length() > 0) {
    // Print the first 100 characters of the response for debugging
    Serial.println(F("[NETATMO] Response preview:"));
    if (response.length() > 100) {
      Serial.println(response.substring(0, 100) + "...");
    } else {
      Serial.println(response);
    }
    
    // Create a simple response format that matches what the JavaScript expects
    String formattedResponse = "{\"body\":{\"devices\":";
    
    // Parse the response to extract just the devices array
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.print(F("[NETATMO] JSON parse error: "));
      Serial.println(error.c_str());
      return;
    }
    
    // Check if we have a valid response with devices
    if (doc.containsKey("body") && doc["body"].containsKey("devices")) {
      // Extract just the devices array
      JsonArray devices = doc["body"]["devices"];
      
      // Create a new document for the devices array
      DynamicJsonDocument devicesDoc(3072);
      JsonArray devicesArray = devicesDoc.to<JsonArray>();
      
      // Copy each device to the new array
      for (JsonObject device : devices) {
        JsonObject newDevice = devicesArray.createNestedObject();
        
        // Copy essential fields
        if (device.containsKey("_id")) newDevice["_id"] = device["_id"].as<const char*>();
        if (device.containsKey("station_name")) newDevice["station_name"] = device["station_name"].as<const char*>();
        if (device.containsKey("type")) newDevice["type"] = device["type"].as<const char*>();
        
        // Copy modules if they exist
        if (device.containsKey("modules")) {
          JsonArray modules = device["modules"];
          JsonArray newModules = newDevice.createNestedArray("modules");
          
          for (JsonObject module : modules) {
            JsonObject newModule = newModules.createNestedObject();
            
            // Copy essential module fields
            if (module.containsKey("_id")) newModule["_id"] = module["_id"].as<const char*>();
            if (module.containsKey("module_name")) newModule["module_name"] = module["module_name"].as<const char*>();
            if (module.containsKey("type")) newModule["type"] = module["type"].as<const char*>();
          }
        }
      }
      
      // Serialize the devices array
      String devicesJson;
      serializeJson(devicesArray, devicesJson);
      
      // Complete the response
      formattedResponse += devicesJson;
      formattedResponse += "}}";
      
      // Store the formatted response
      deviceData = formattedResponse;
      
      Serial.println(F("[NETATMO] Device data processed and cached"));
      Serial.print(F("[NETATMO] Formatted response size: "));
      Serial.println(deviceData.length());
    } else {
      // Try to adapt the response format if it's different
      Serial.println(F("[NETATMO] Trying alternative response format"));
      
      // Create a new document for the devices array
      DynamicJsonDocument devicesDoc(3072);
      JsonArray devicesArray = devicesDoc.to<JsonArray>();
      
      // Check if we have a devices array directly
      if (doc.containsKey("devices") && doc["devices"].is<JsonArray>()) {
        JsonArray devices = doc["devices"];
        
        // Copy each device to the new array
        for (JsonObject device : devices) {
          JsonObject newDevice = devicesArray.createNestedObject();
          
          // Copy essential fields
          if (device.containsKey("_id")) newDevice["_id"] = device["_id"].as<const char*>();
          else if (device.containsKey("id")) newDevice["_id"] = device["id"].as<const char*>();
          
          if (device.containsKey("station_name")) newDevice["station_name"] = device["station_name"].as<const char*>();
          else if (device.containsKey("name")) newDevice["station_name"] = device["name"].as<const char*>();
          
          if (device.containsKey("type")) newDevice["type"] = device["type"].as<const char*>();
          
          // Copy modules if they exist
          if (device.containsKey("modules")) {
            JsonArray modules = device["modules"];
            JsonArray newModules = newDevice.createNestedArray("modules");
            
            for (JsonObject module : modules) {
              JsonObject newModule = newModules.createNestedObject();
              
              // Copy essential module fields
              if (module.containsKey("_id")) newModule["_id"] = module["_id"].as<const char*>();
              else if (module.containsKey("id")) newModule["_id"] = module["id"].as<const char*>();
              
              if (module.containsKey("module_name")) newModule["module_name"] = module["module_name"].as<const char*>();
              else if (module.containsKey("name")) newModule["module_name"] = module["name"].as<const char*>();
              
              if (module.containsKey("type")) newModule["type"] = module["type"].as<const char*>();
            }
          }
        }
        
        // Serialize the devices array
        String devicesJson;
        serializeJson(devicesArray, devicesJson);
        
        // Complete the response
        formattedResponse += devicesJson;
        formattedResponse += "}}";
        
        // Store the formatted response
        deviceData = formattedResponse;
        
        Serial.println(F("[NETATMO] Device data processed using alternative format"));
        Serial.print(F("[NETATMO] Formatted response size: "));
        Serial.println(deviceData.length());
      } else {
        Serial.println(F("[NETATMO] Error - Invalid response format"));
        Serial.println(response);
      }
    }
  } else {
    Serial.println(F("[NETATMO] Error - Empty response"));
    
    // If the first attempt failed, try the getstationsdata endpoint
    Serial.println(F("[NETATMO] Trying alternative API endpoint"));
    
    if (!https.begin(client, "https://api.netatmo.com/api/getstationsdata")) {
      Serial.println(F("[NETATMO] Error - Failed to connect to alternative endpoint"));
      return;
    }
    
    // Set the authorization header correctly
    https.addHeader("Authorization", authHeader);
    https.addHeader("Accept", "application/json");
    
    Serial.println(F("[NETATMO] Sending request to /api/getstationsdata"));
    httpCode = https.GET();
    
    Serial.print(F("[NETATMO] HTTP response code: "));
    Serial.println(httpCode);
    
    if (httpCode != HTTP_CODE_OK) {
      Serial.print(F("[NETATMO] Error - HTTP code: "));
      Serial.println(httpCode);
      if (httpCode > 0) {
        Serial.println(F("[NETATMO] Response: "));
        Serial.println(https.getString());
      }
      https.end();
      return;
    }
    
    // Get the response
    response = https.getString();
    https.end();
    
    Serial.println(F("[NETATMO] Received device data from alternative endpoint"));
    Serial.print(F("[NETATMO] Response size: "));
    Serial.println(response.length());
    
    if (response.length() > 0) {
      // Process the response as before
      // Store the response for future requests
      deviceData = response;
      Serial.println(F("[NETATMO] Device data cached from alternative endpoint"));
    } else {
      Serial.println(F("[NETATMO] Error - Empty response from alternative endpoint"));
    }
  }
}
