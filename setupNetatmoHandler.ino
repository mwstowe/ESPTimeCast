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

// Function to refresh the Netatmo token
bool refreshNetatmoToken() {
  Serial.println(F("[NETATMO] Refreshing token"));
  
  if (strlen(netatmoRefreshToken) == 0) {
    Serial.println(F("[NETATMO] Error - No refresh token"));
    return false;
  }
  
  // Create a new client for the API call
  static BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  if (!https.begin(client, "https://api.netatmo.com/oauth2/token")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return false;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Build the POST data
  String postData = "grant_type=refresh_token";
  postData += "&client_id=";
  postData += urlEncode(netatmoClientId);
  postData += "&client_secret=";
  postData += urlEncode(netatmoClientSecret);
  postData += "&refresh_token=";
  postData += urlEncode(netatmoRefreshToken);
  
  // Make the request
  int httpCode = https.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    String errorPayload = https.getString();
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    return false;
  }
  
  // Get the response
  String response = https.getString();
  https.end();
  
  // Parse the response
  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.print(F("[NETATMO] JSON parse error: "));
    Serial.println(error.c_str());
    return false;
  }
  
  // Extract the tokens
  if (!doc.containsKey("access_token") || !doc.containsKey("refresh_token")) {
    Serial.println(F("[NETATMO] Missing tokens in response"));
    return false;
  }
  
  // Save the tokens
  const char* accessToken = doc["access_token"];
  const char* refreshToken = doc["refresh_token"];
  
  Serial.println(F("[NETATMO] Saving new tokens"));
  strlcpy(netatmoAccessToken, accessToken, sizeof(netatmoAccessToken));
  strlcpy(netatmoRefreshToken, refreshToken, sizeof(netatmoRefreshToken));
  
  saveTokensToConfig();
  return true;
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

// Global flag to indicate that credentials need to be saved
static bool saveCredentialsPending = false;

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
    
    // Set flag to save in main loop instead of doing it here
    saveCredentialsPending = true;
    
    // Send a simple response
    request->send(200, "application/json", "{\"success\":true}");
    
    Serial.println(F("[NETATMO] Credentials saved to memory, will be written to config in main loop"));
  });
  
  // Endpoint to get auth URL without redirecting
  server.on("/api/netatmo/get-auth-url", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling get auth URL request"));
    
    // Debug output to check client ID and secret
    Serial.print(F("[NETATMO] Client ID length: "));
    Serial.println(strlen(netatmoClientId));
    Serial.print(F("[NETATMO] Client Secret length: "));
    Serial.println(strlen(netatmoClientSecret));
    
    if (strlen(netatmoClientId) == 0 || strlen(netatmoClientSecret) == 0) {
      Serial.println(F("[NETATMO] Error - No credentials"));
      request->send(400, "application/json", "{\"error\":\"Missing credentials\"}");
      return;
    }
    
    // Generate authorization URL with minimal memory usage
    String redirectUri = "http://";
    redirectUri += WiFi.localIP().toString();
    redirectUri += "/api/netatmo/callback";
    
    String authUrl = "https://api.netatmo.com/oauth2/authorize";
    authUrl += "?client_id=";
    authUrl += urlEncode(netatmoClientId);
    authUrl += "&redirect_uri=";
    authUrl += urlEncode(redirectUri.c_str());
    authUrl += "&scope=read_station%20read_homecoach&state=state&response_type=code";
    
    Serial.print(F("[NETATMO] Auth URL: "));
    Serial.println(authUrl);
    
    // Return the URL as JSON
    String response = "{\"url\":\"";
    response += authUrl;
    response += "\"}";
    
    request->send(200, "application/json", response);
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
    // Store the URL in a static variable to avoid stack issues
    static String authUrl;
    authUrl = "https://api.netatmo.com/oauth2/authorize";
    authUrl += "?client_id=";
    authUrl += urlEncode(netatmoClientId);
    authUrl += "&redirect_uri=";
    
    // Build the redirect URI directly
    String redirectUri = "http://";
    redirectUri += WiFi.localIP().toString();
    redirectUri += "/api/netatmo/callback";
    
    authUrl += urlEncode(redirectUri.c_str());
    authUrl += "&scope=read_station%20read_homecoach%20access_camera%20read_presence%20read_thermostat&state=state&response_type=code";
    
    Serial.print(F("[NETATMO] Auth URL: "));
    Serial.println(authUrl);
    
    // Use a simpler response with a meta refresh instead of a redirect
    String html = F("<!DOCTYPE html>");
    html += F("<html><head>");
    html += F("<meta http-equiv='refresh' content='0;url=");
    html += authUrl;
    html += F("'>");
    html += F("<title>Redirecting...</title>");
    html += F("</head><body>");
    html += F("<p>Redirecting to Netatmo authorization page...</p>");
    html += F("<p>If you are not redirected, <a href='");
    html += authUrl;
    html += F("'>click here</a>.</p>");
    html += F("</body></html>");
    
    request->send(200, "text/html", html);
    Serial.println(F("[NETATMO] Auth page sent"));
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
    
    // Store the code for later processing in the main loop
    // This avoids deep stack issues during the callback
    pendingCode = code;
    tokenExchangePending = true;
    
    // Simplified HTML response to minimize memory usage
    static const char SUCCESS_HTML[] PROGMEM = 
      "<!DOCTYPE html>"
      "<html><head>"
      "<meta charset='UTF-8'>"
      "<title>ESPTimeCast - Authorization</title>"
      "<style>body{font-family:Arial;text-align:center;margin:50px}</style>"
      "</head><body>"
      "<h1>Authorization Successful</h1>"
      "<p>Exchanging code for tokens...</p>"
      "<p>The device will reboot automatically after token exchange.</p>"
      "<p>After reboot, please navigate to <a href='/netatmo.html'>Netatmo Settings</a> to configure your devices.</p>"
      "</body></html>";
    
    request->send_P(200, "text/html", SUCCESS_HTML);
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
  
  // Add a debug endpoint to check token status
  server.on("/api/netatmo/token-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling token status request"));
    
    String response = "{";
    response += "\"hasAccessToken\":" + String(strlen(netatmoAccessToken) > 0 ? "true" : "false");
    response += ",\"hasRefreshToken\":" + String(strlen(netatmoRefreshToken) > 0 ? "true" : "false");
    response += ",\"accessTokenLength\":" + String(strlen(netatmoAccessToken));
    response += ",\"refreshTokenLength\":" + String(strlen(netatmoRefreshToken));
    
    // Add the first few characters of the tokens for verification
    if (strlen(netatmoAccessToken) > 10) {
      response += ",\"accessTokenPrefix\":\"" + String(netatmoAccessToken).substring(0, 10) + "...\"";
    }
    
    if (strlen(netatmoRefreshToken) > 10) {
      response += ",\"refreshTokenPrefix\":\"" + String(netatmoRefreshToken).substring(0, 10) + "...\"";
    }
    
    response += "}";
    
    request->send(200, "application/json", response);
  });
  
  // Add a token refresh endpoint
  server.on("/api/netatmo/refresh-token", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling token refresh request"));
    
    if (strlen(netatmoRefreshToken) == 0) {
      Serial.println(F("[NETATMO] Error - No refresh token"));
      request->send(401, "application/json", "{\"error\":\"No refresh token available\"}");
      return;
    }
    
    // Create a new client for the API call
    static BearSSL::WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation to save memory
    
    HTTPClient https;
    https.setTimeout(10000); // 10 second timeout
    
    if (!https.begin(client, "https://api.netatmo.com/oauth2/token")) {
      Serial.println(F("[NETATMO] Error - Failed to connect"));
      request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API\"}");
      return;
    }
    
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Build the POST data
    String postData = "grant_type=refresh_token";
    postData += "&client_id=";
    postData += urlEncode(netatmoClientId);
    postData += "&client_secret=";
    postData += urlEncode(netatmoClientSecret);
    postData += "&refresh_token=";
    postData += urlEncode(netatmoRefreshToken);
    
    // Make the request
    int httpCode = https.POST(postData);
    
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
    String response = https.getString();
    https.end();
    
    // Parse the response
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.print(F("[NETATMO] JSON parse error: "));
      Serial.println(error.c_str());
      request->send(500, "application/json", "{\"error\":\"Failed to parse response\"}");
      return;
    }
    
    // Extract the tokens
    if (!doc.containsKey("access_token") || !doc.containsKey("refresh_token")) {
      Serial.println(F("[NETATMO] Missing tokens in response"));
      request->send(500, "application/json", "{\"error\":\"Missing tokens in response\"}");
      return;
    }
    
    // Save the tokens
    const char* accessToken = doc["access_token"];
    const char* refreshToken = doc["refresh_token"];
    
    Serial.println(F("[NETATMO] Saving new tokens"));
    strlcpy(netatmoAccessToken, accessToken, sizeof(netatmoAccessToken));
    strlcpy(netatmoRefreshToken, refreshToken, sizeof(netatmoRefreshToken));
    
    saveTokensToConfig();
    
    // Send success response
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Token refreshed successfully\"}");
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
  
  // Add a proxy endpoint for Netatmo API calls to avoid CORS issues
  server.on("/api/netatmo/proxy", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling proxy request"));
    
    // Check if we have a valid access token
    if (strlen(netatmoAccessToken) == 0) {
      Serial.println(F("[NETATMO] Error - No access token"));
      request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
      return;
    }
    
    // Debug output
    Serial.print(F("[NETATMO] Using access token: "));
    Serial.println(netatmoAccessToken);
    
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
    String authHeader = "Bearer ";
    authHeader += netatmoAccessToken;
    https.addHeader("Authorization", authHeader);
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
      
      // If we get a 401 or 403, try to refresh the token
      if (httpCode == 401 || httpCode == 403) {
        Serial.println(F("[NETATMO] Token expired, trying to refresh"));
        
        // Try to refresh the token
        if (refreshNetatmoToken()) {
          Serial.println(F("[NETATMO] Token refreshed, retrying request"));
          
          // Retry the request with the new token
          HTTPClient https2;
          https2.setTimeout(10000);
          
          if (!https2.begin(client, apiUrl)) {
            Serial.println(F("[NETATMO] Error - Failed to connect on retry"));
            request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API on retry\"}");
            return;
          }
          
          // Add authorization header with new token
          String newAuthHeader = "Bearer ";
          newAuthHeader += netatmoAccessToken;
          https2.addHeader("Authorization", newAuthHeader);
          https2.addHeader("Accept", "application/json");
          
          // Make the request again
          int httpCode2 = https2.GET();
          
          if (httpCode2 != HTTP_CODE_OK) {
            Serial.print(F("[NETATMO] Error on retry - HTTP code: "));
            Serial.println(httpCode2);
            String errorPayload2 = https2.getString();
            Serial.print(F("[NETATMO] Error payload on retry: "));
            Serial.println(errorPayload2);
            https2.end();
            request->send(httpCode2, "application/json", errorPayload2);
            return;
          }
          
          // Get the response
          String payload2 = https2.getString();
          https2.end();
          
          // Send the response back to the client
          request->send(200, "application/json", payload2);
          return;
        } else {
          Serial.println(F("[NETATMO] Failed to refresh token"));
        }
      }
      
      request->send(httpCode, "application/json", errorPayload);
      return;
    }
    
    // Get the response
    String payload = https.getString();
    https.end();
    
    // Send the response back to the client
    request->send(200, "application/json", payload);
  });
  
  // Add a debug endpoint to check token status
  server.on("/api/netatmo/token-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling token status request"));
    
    String response = "{";
    response += "\"hasAccessToken\":" + String(strlen(netatmoAccessToken) > 0 ? "true" : "false");
    response += ",\"hasRefreshToken\":" + String(strlen(netatmoRefreshToken) > 0 ? "true" : "false");
    response += ",\"accessTokenLength\":" + String(strlen(netatmoAccessToken));
    response += ",\"refreshTokenLength\":" + String(strlen(netatmoRefreshToken));
    
    // Add the first few characters of the tokens for verification
    if (strlen(netatmoAccessToken) > 10) {
      response += ",\"accessTokenPrefix\":\"" + String(netatmoAccessToken).substring(0, 10) + "...\"";
    }
    
    if (strlen(netatmoRefreshToken) > 10) {
      response += ",\"refreshTokenPrefix\":\"" + String(netatmoRefreshToken).substring(0, 10) + "...\"";
    }
    
    response += "}";
    
    request->send(200, "application/json", response);
  });
  
  Serial.println(F("[NETATMO] OAuth handler setup complete"));
}

// Function to be called from loop() to process pending token exchange
void processTokenExchange() {
  if (!tokenExchangePending || pendingCode.isEmpty()) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing token exchange"));
  
  // Print detailed memory stats before we start
  Serial.println(F("[NETATMO] Memory status before token exchange:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  Serial.print(F("[MEMORY] Heap fragmentation: "));
  Serial.print(ESP.getHeapFragmentation());
  Serial.println(F("%"));
  Serial.print(F("[MEMORY] Largest free block: "));
  Serial.print(ESP.getMaxFreeBlockSize());
  Serial.println(F(" bytes"));
  
  // Perform a gentle defragmentation if needed
  if (ESP.getHeapFragmentation() > 30 || ESP.getMaxFreeBlockSize() < 20000) {
    Serial.println(F("[NETATMO] Performing gentle heap defragmentation"));
    
    // Allocate and free a block to help defragment
    size_t blockSize = ESP.getMaxFreeBlockSize() * 0.7; // 70% of largest block
    if (blockSize > 1024) {
      char* block = (char*)malloc(blockSize);
      if (block) {
        // Touch the memory to ensure it's physically allocated
        memset(block, 0, blockSize);
        free(block);
        Serial.println(F("[NETATMO] Defragmentation complete"));
      } else {
        Serial.println(F("[NETATMO] Failed to allocate block for defragmentation"));
      }
    }
    
    // Print memory stats after defragmentation
    Serial.println(F("[NETATMO] Memory status after defragmentation:"));
    Serial.print(F("[MEMORY] Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    Serial.print(F("[MEMORY] Heap fragmentation: "));
    Serial.print(ESP.getHeapFragmentation());
    Serial.println(F("%"));
    Serial.print(F("[MEMORY] Largest free block: "));
    Serial.print(ESP.getMaxFreeBlockSize());
    Serial.println(F(" bytes"));
  }
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  tokenExchangePending = false;
  String code = pendingCode;
  pendingCode = "";
  
  // Skip memory checks and defragmentation to avoid yield issues
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  // Debug WiFi status
  Serial.print(F("[NETATMO] WiFi RSSI: "));
  Serial.println(WiFi.RSSI());
  Serial.print(F("[NETATMO] WiFi IP: "));
  Serial.println(WiFi.localIP());
  
  // Try with a completely different approach using a direct TCP connection
  // This avoids the SSL/TLS overhead that might be causing issues
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation
  client.setTimeout(20000); // 20 second timeout
  
  // Enable SSL debugging (not available in BearSSL::WiFiClientSecure)
  // Debug output will be visible in Serial monitor
  Serial.println(F("[NETATMO] SSL debugging not available in BearSSL::WiFiClientSecure"));
  
  Serial.println(F("[NETATMO] Memory before connection:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  Serial.println(F("[NETATMO] Connecting directly to api.netatmo.com:443"));
  if (!client.connect("api.netatmo.com", 443)) {
    Serial.println(F("[NETATMO] Direct connection failed"));
    
    // Try to get more info about the failure
    Serial.println(F("[NETATMO] SSL error details not available in BearSSL::WiFiClientSecure"));
    
    // Try a non-SSL connection to check basic connectivity
    WiFiClient plainClient;
    Serial.println(F("[NETATMO] Trying plain connection to api.netatmo.com:80 to check connectivity"));
    if (plainClient.connect("api.netatmo.com", 80)) {
      Serial.println(F("[NETATMO] Plain connection succeeded - issue is likely with SSL"));
      plainClient.stop();
    } else {
      Serial.println(F("[NETATMO] Plain connection also failed - issue might be with DNS or network"));
    }
    
    return;
  }
  
  Serial.println(F("[NETATMO] Connected! Building request"));
  Serial.println(F("[NETATMO] Memory after connection:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  // Build the redirect URI
  String redirectUri = "http://";
  redirectUri += WiFi.localIP().toString();
  redirectUri += "/api/netatmo/callback";
  
  // Build the POST data
  String postData = "grant_type=authorization_code";
  postData += "&client_id=";
  postData += urlEncode(netatmoClientId);
  postData += "&client_secret=";
  postData += urlEncode(netatmoClientSecret);
  postData += "&code=";
  postData += urlEncode(code.c_str());
  postData += "&redirect_uri=";
  postData += urlEncode(redirectUri.c_str());
  
  // Build the HTTP request manually
  String request = "POST /oauth2/token HTTP/1.1\r\n";
  request += "Host: api.netatmo.com\r\n";
  request += "Connection: close\r\n";
  request += "Content-Type: application/x-www-form-urlencoded\r\n";
  request += "Accept: application/json\r\n";
  request += "Content-Length: " + String(postData.length()) + "\r\n";
  request += "\r\n";
  request += postData;
  
  Serial.println(F("[NETATMO] Sending direct request"));
  Serial.println(F("[NETATMO] Memory before sending:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  client.print(request);
  
  // Wait for the response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 20000) {
      Serial.println(F("[NETATMO] Direct request timeout"));
      client.stop();
      return;
    }
    delay(10); // Small delay to avoid watchdog issues
  }
  
  Serial.println(F("[NETATMO] Memory after sending:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  // Read the response headers
  Serial.println(F("[NETATMO] Reading response headers"));
  String headers = "";
  String line = "";
  bool headerDone = false;
  
  while (client.available() && !headerDone) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      headerDone = true;
    } else {
      headers += line + "\n";
    }
  }
  
  Serial.println(F("[NETATMO] Response headers:"));
  Serial.println(headers);
  
  // Read the response body
  Serial.println(F("[NETATMO] Reading response body"));
  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
    
    // Yield occasionally to avoid watchdog issues
    if (response.length() % 100 == 0) {
      delay(0);
    }
  }
  
  client.stop();
  
  Serial.println(F("[NETATMO] Response received"));
  Serial.print(F("[NETATMO] Response length: "));
  Serial.println(response.length());
  Serial.println(F("[NETATMO] Memory after receiving:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  // Check if we got a valid JSON response
  if (response.indexOf("{") != 0) {
    Serial.println(F("[NETATMO] Invalid JSON response"));
    Serial.println(response);
    return;
  }
  
  // Try an alternative approach if the direct connection fails
  if (response.length() == 0 || response.indexOf("{") != 0) {
    Serial.println(F("[NETATMO] Direct connection failed, trying HTTPClient approach"));
    
    // Memory optimization: Use static client to reduce stack usage
    static BearSSL::WiFiClientSecure httpsClient;
    httpsClient.setInsecure(); // Skip certificate validation to save memory
    httpsClient.setTimeout(15000); // Increase timeout to 15 seconds
    
    HTTPClient https;
    https.setTimeout(15000); // Increase timeout to 15 seconds
    
    Serial.println(F("[NETATMO] Connecting to token endpoint via HTTPClient"));
    if (!https.begin(httpsClient, "https://api.netatmo.com/oauth2/token")) {
      Serial.println(F("[NETATMO] Error - Failed to connect"));
      return;
    }
    
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    https.addHeader("Accept", "application/json");
    https.addHeader("Connection", "close"); // Try to avoid keep-alive issues
    
    // Rebuild the POST data
    String httpPostData = "grant_type=authorization_code";
    httpPostData += "&client_id=";
    httpPostData += urlEncode(netatmoClientId);
    httpPostData += "&client_secret=";
    httpPostData += urlEncode(netatmoClientSecret);
    httpPostData += "&code=";
    httpPostData += urlEncode(code.c_str());
    httpPostData += "&redirect_uri=";
    httpPostData += urlEncode(redirectUri.c_str());
    
    Serial.println(F("[NETATMO] Sending token request via HTTPClient"));
    int httpCode = https.POST(httpPostData);
    
    if (httpCode != HTTP_CODE_OK) {
      Serial.print(F("[NETATMO] Error - HTTP code: "));
      Serial.println(httpCode);
      
      // Get more detailed error information if available
      if (httpCode > 0) {
        String payload = https.getString();
        Serial.print(F("[NETATMO] Error response: "));
        Serial.println(payload);
      } else {
        Serial.print(F("[NETATMO] Connection error: "));
        Serial.println(https.errorToString(httpCode));
      }
      
      https.end();
      return;
    }
    
    // Get the response
    response = https.getString();
    https.end();
  }
  
  Serial.println(F("[NETATMO] Memory before JSON parsing:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  processTokenResponse(response);
  
  Serial.println(F("[NETATMO] Memory after token processing:"));
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
}

// Helper function to process token response
void processTokenResponse(String response) {
  Serial.println(F("[NETATMO] Parsing response"));
  Serial.print(F("[NETATMO] Response: "));
  Serial.println(response);
  
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
  
  // Set flag to save tokens in main loop
  saveCredentialsPending = true;
  
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

// Function to be called from loop() to process pending credential saves
void processSaveCredentials() {
  if (!saveCredentialsPending) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing pending credential save"));
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  saveCredentialsPending = false;
  
  // Save to config file
  saveTokensToConfig();
  
  Serial.println(F("[NETATMO] Credentials saved to config file"));
}
