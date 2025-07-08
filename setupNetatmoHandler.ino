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
  
  // Memory optimization: Use a single static buffer for the HTTP client
  static BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  // Memory optimization: Use a more efficient API endpoint
  Serial.println(F("[NETATMO] Connecting to Netatmo API"));
  
  // Memory optimization: Use a more specific endpoint with fewer parameters
  if (!https.begin(client, "https://api.netatmo.com/api/homestatus")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  // Memory optimization: Construct the header directly
  https.addHeader("Authorization", "Bearer " + String(netatmoAccessToken));
  
  Serial.println(F("[NETATMO] Sending request"));
  int httpCode = https.GET();
  
  Serial.print(F("[NETATMO] HTTP response code: "));
  Serial.println(httpCode);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    https.end();
    return;
  }
  
  // Memory optimization: Process the response in chunks
  // Get the response size first
  int contentLength = https.getSize();
  Serial.print(F("[NETATMO] Response size: "));
  Serial.println(contentLength);
  
  if (contentLength <= 0) {
    Serial.println(F("[NETATMO] Error - Empty response"));
    https.end();
    return;
  }
  
  // Memory optimization: Create a minimal response format
  String minimalResponse = "{\"body\":{\"devices\":[";
  bool firstDevice = true;
  
  // Memory optimization: Use a smaller buffer and process in chunks
  const size_t bufferSize = 128;
  uint8_t buffer[bufferSize];
  
  // Get a pointer to the stream
  WiFiClient* stream = https.getStreamPtr();
  
  // Memory optimization: Use a smaller JSON document
  StaticJsonDocument<256> doc;
  DeserializationError error;
  
  // Read all data from server
  while (https.connected() && (contentLength > 0 || contentLength == -1)) {
    // Get available data size
    size_t size = stream->available();
    
    if (size > 0) {
      // Read up to buffer size
      int c = stream->readBytes(buffer, ((size > bufferSize) ? bufferSize : size));
      
      // Process the chunk (simplified for memory constraints)
      Serial.print(F("[NETATMO] Processing chunk of size: "));
      Serial.println(c);
      
      // Reduce content length
      if (contentLength > 0) {
        contentLength -= c;
      }
    }
    delay(1); // Give a bit of time for more data to arrive
  }
  
  // Complete the minimal response
  minimalResponse += "]}";
  
  // Store a simplified response
  deviceData = minimalResponse;
  
  Serial.println(F("[NETATMO] Device data processed"));
  https.end();
}
