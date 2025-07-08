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

// Function to trigger Netatmo device fetch from external files
void triggerNetatmoDevicesFetch() {
  Serial.println(F("[NETATMO] Device fetch triggered externally"));
  fetchDevicesPending = true;
}

// Function to get cached device data
String getNetatmoDeviceData() {
  return deviceData;
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
    
    // Debug output to check client ID and secret
    Serial.print(F("[NETATMO] Client ID length: "));
    Serial.println(strlen(netatmoClientId));
    Serial.print(F("[NETATMO] Client Secret length: "));
    Serial.println(strlen(netatmoClientSecret));
    
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
    authUrl += "&scope=read_station%20read_homecoach%20access_camera%20read_presence%20read_thermostat&state=state&response_type=code";
    
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
    
    // Simplified HTML response to minimize memory usage
    String html = F("<!DOCTYPE html>");
    html += F("<html><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<title>ESPTimeCast - Authorization</title>");
    html += F("<style>body{font-family:Arial;text-align:center;margin:50px}</style>");
    html += F("</head><body>");
    html += F("<h1>Authorization Successful</h1>");
    html += F("<p>Exchanging code for tokens...</p>");
    html += F("<p>The device will reboot automatically after token exchange.</p>");
    html += F("<p>After reboot, please navigate to <a href='/netatmo.html'>Netatmo Settings</a> to configure your devices.</p>");
    html += F("</body></html>");
    
    request->send(200, "text/html", html);
  });
  
  // Endpoint to get the access token
  server.on("/api/netatmo/token", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling get token request"));
    
    if (strlen(netatmoAccessToken) == 0) {
      Serial.println(F("[NETATMO] Error - No access token"));
      request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
      return;
    }
    
    // Create a JSON response with the access token
    String response = "{\"access_token\":\"";
    response += netatmoAccessToken;
    response += "\"}";
    
    request->send(200, "application/json", response);
  });
  
  // Add a proxy endpoint for Netatmo API calls to avoid CORS issues
  server.on("/api/netatmo/proxy", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling proxy request"));
    
    if (strlen(netatmoAccessToken) == 0) {
      Serial.println(F("[NETATMO] Error - No access token"));
      request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
      return;
    }
    
    // Check which API endpoint to call
    String endpoint = "getstationsdata";
    if (request->hasParam("endpoint")) {
      endpoint = request->getParam("endpoint")->value();
    }
    
    // Create a new client for the API call
    static BearSSL::WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation to save memory
    
    HTTPClient https;
    https.setTimeout(10000); // 10 second timeout
    
    String apiUrl = "https://api.netatmo.com/api/" + endpoint;
    Serial.print(F("[NETATMO] Proxying request to: "));
    Serial.println(apiUrl);
    
    if (!https.begin(client, apiUrl)) {
      Serial.println(F("[NETATMO] Error - Failed to connect"));
      request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API\"}");
      return;
    }
    
    // Add authorization header
    https.addHeader("Authorization", "Bearer " + String(netatmoAccessToken));
    https.addHeader("Accept", "application/json");
    
    // Make the request
    int httpCode = https.GET();
    
    if (httpCode != HTTP_CODE_OK) {
      Serial.print(F("[NETATMO] Error - HTTP code: "));
      Serial.println(httpCode);
      String errorPayload = https.getString();
      Serial.print(F("[NETATMO] Error payload: "));
      Serial.println(errorPayload);
      https.end();
      request->send(httpCode, "application/json", errorPayload);
      return;
    }
    
    // Get the response
    String payload = https.getString();
    https.end();
    
    // Send the response back to the client
    request->send(200, "application/json", payload);
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
  
  // Defragment heap before token exchange to ensure enough contiguous memory
  Serial.println(F("[NETATMO] Checking memory before token exchange"));
  printMemoryStats();
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  tokenExchangePending = false;
  String code = pendingCode;
  pendingCode = "";
  
  // Add a small delay before proceeding to allow system to stabilize
  delay(100);
  
  if (shouldDefragment()) {
    Serial.println(F("[NETATMO] Memory fragmentation detected, defragmenting heap"));
    defragmentHeap();
    
    // Add another delay after defragmentation
    delay(100);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  // Memory optimization: Use static client to reduce stack usage
  static BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  Serial.println(F("[NETATMO] Connecting to token endpoint"));
  if (!https.begin(client, "https://api.netatmo.com/oauth2/token")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Memory optimization: Build the POST data in chunks
  String postData = "grant_type=authorization_code";
  postData += "&client_id=";
  postData += urlEncode(netatmoClientId);
  postData += "&client_secret=";
  postData += urlEncode(netatmoClientSecret);
  postData += "&code=";
  postData += urlEncode(code.c_str());
  postData += "&redirect_uri=";
  
  // Memory optimization: Build the redirect URI directly
  String redirectUri = "http://";
  redirectUri += WiFi.localIP().toString();
  redirectUri += "/api/netatmo/callback";
  postData += urlEncode(redirectUri.c_str());
  
  Serial.println(F("[NETATMO] Sending token request"));
  int httpCode = https.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    https.end();
    return;
  }
  
  // Memory optimization: Process the response in smaller chunks
  // Get the response
  String response = https.getString();
  https.end();
  
  Serial.println(F("[NETATMO] Parsing response"));
  
  // Memory optimization: Use a smaller JSON buffer
  StaticJsonDocument<384> doc;
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
  
  // Memory optimization: Extract tokens directly as strings
  const char* accessToken = doc["access_token"];
  const char* refreshToken = doc["refresh_token"];
  
  // Save the tokens
  Serial.println(F("[NETATMO] Saving tokens"));
  strlcpy(netatmoAccessToken, accessToken, sizeof(netatmoAccessToken));
  strlcpy(netatmoRefreshToken, refreshToken, sizeof(netatmoRefreshToken));
  
  saveTokensToConfig();
  Serial.println(F("[NETATMO] Token exchange complete"));
  
  // Schedule a reboot after token exchange is complete
  Serial.println(F("[NETATMO] Scheduling reboot to apply new tokens"));
  rebootPending = true;
  rebootTime = millis() + 2000; // Reboot in 2 seconds
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
  
  // Memory optimization: Create a minimal response structure
  // This is a placeholder that tells the JavaScript to use the API directly
  deviceData = F("{\"redirect\":\"api\"}");
  
  Serial.println(F("[NETATMO] Set redirect flag for client-side API call"));
}
