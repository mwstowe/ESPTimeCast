// Global variables for token exchange
static String pendingCode = "";
static bool tokenExchangePending = false;
static bool fetchDevicesPending = false;
static bool fetchStationsDataPending = false;
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
  yield(); // Allow the watchdog to be fed
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    String errorPayload = https.getString();
    yield(); // Allow the watchdog to be fed
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    return false;
  }
  
  // Get the response
  String response = https.getString();
  yield(); // Allow the watchdog to be fed
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
    
    // Debug: Print the current values
    Serial.print(F("[NETATMO] Current netatmoClientId: "));
    Serial.println(netatmoClientId);
    Serial.print(F("[NETATMO] Current netatmoClientSecret length: "));
    Serial.println(strlen(netatmoClientSecret));
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
    
    // Simplified HTML response with auto-redirect to minimize memory usage
    static const char SUCCESS_HTML[] PROGMEM = 
      "<!DOCTYPE html>"
      "<html><head>"
      "<meta charset='UTF-8'>"
      "<meta http-equiv='refresh' content='3;url=/netatmo.html'>"
      "<title>ESPTimeCast - Authorization</title>"
      "<style>body{font-family:Arial;text-align:center;margin:50px}</style>"
      "</head><body>"
      "<h1>Authorization Successful</h1>"
      "<p>Exchanging code for tokens...</p>"
      "<p>You will be redirected to the Netatmo settings page in 3 seconds.</p>"
      "<p>If not redirected, <a href='/netatmo.html'>click here</a>.</p>"
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
    
    // Check if there's already a pending request
    if (proxyPending) {
      request->send(429, "application/json", "{\"error\":\"Another request is already in progress\"}");
      return;
    }
    
    // Set up the deferred request
    proxyPending = true;
    proxyEndpoint = endpoint;
    proxyRequest = request;
    
    // The actual request will be processed in the loop() function via processProxyRequest()
    // This prevents blocking the web server and avoids watchdog timeouts
    
    // Note: We don't call request->send() here because we'll do that in processProxyRequest()
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
  
  // Add an endpoint to get the current API keys
  server.on("/api/netatmo/get-credentials", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling get credentials request"));
    
    String response = "{";
    response += "\"clientId\":\"" + String(netatmoClientId) + "\"";
    response += ",\"clientSecret\":\"" + String(netatmoClientSecret) + "\"";
    response += ",\"hasClientSecret\":" + String(strlen(netatmoClientSecret) > 0 ? "true" : "false");
    response += "}";
    
    request->send(200, "application/json", response);
  });
  
  // Add an endpoint to get memory information and trigger defragmentation
  server.on("/api/memory", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[MEMORY] Handling memory info request"));
    
    // Check if defragmentation was requested
    bool defrag = false;
    if (request->hasParam("defrag")) {
      defrag = request->getParam("defrag")->value() == "true";
    }
    
    // Get memory stats
    uint32_t freeHeap = ESP.getFreeHeap();
    uint8_t fragmentation = ESP.getHeapFragmentation();
    uint32_t maxFreeBlock = ESP.getMaxFreeBlockSize();
    uint32_t freeStack = ESP.getFreeContStack();
    
    // Print memory stats to serial
    printMemoryStats();
    
    // Perform defragmentation if requested
    if (defrag) {
      Serial.println(F("[MEMORY] Defragmentation requested via API"));
      defragmentHeap();
      
      // Get updated stats
      freeHeap = ESP.getFreeHeap();
      fragmentation = ESP.getHeapFragmentation();
      maxFreeBlock = ESP.getMaxFreeBlockSize();
      freeStack = ESP.getFreeContStack();
    }
    
    // Create response
    String response = "{";
    response += "\"freeHeap\":" + String(freeHeap);
    response += ",\"fragmentation\":" + String(fragmentation);
    response += ",\"maxFreeBlock\":" + String(maxFreeBlock);
    response += ",\"freeStack\":" + String(freeStack);
    response += ",\"defragPerformed\":" + String(defrag ? "true" : "false");
    response += "}";
    
    request->send(200, "application/json", response);
  });
  
  // Add an endpoint to trigger device fetch
  server.on("/api/netatmo/fetch-devices", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling fetch devices request"));
    
    // Check if we have a valid access token
    if (strlen(netatmoAccessToken) == 0) {
      Serial.println(F("[NETATMO] Error - No access token"));
      request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
      return;
    }
    
    // Set the flag to fetch devices in the main loop
    fetchDevicesPending = true;
    
    // Send a response indicating that the fetch has been initiated
    request->send(200, "application/json", "{\"status\":\"initiated\",\"message\":\"Device fetch initiated\"}");
  });
  
  // Add an endpoint to retrieve the saved device data
  server.on("/api/netatmo/saved-devices", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling saved devices request"));
    
    // Check if the file exists
    if (!LittleFS.exists("/devices/netatmo_devices.json")) {
      Serial.println(F("[NETATMO] Device data file not found"));
      request->send(404, "application/json", "{\"error\":\"Device data not found\"}");
      return;
    }
    
    // Open the file
    File deviceFile = LittleFS.open("/devices/netatmo_devices.json", "r");
    if (!deviceFile) {
      Serial.println(F("[NETATMO] Failed to open device data file"));
      request->send(500, "application/json", "{\"error\":\"Failed to open device data file\"}");
      return;
    }
    
    // Create a response stream
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    
    // Stream the file content to the response
    const size_t bufSize = 256;
    uint8_t buf[bufSize];
    
    while (deviceFile.available()) {
      size_t bytesRead = deviceFile.read(buf, bufSize);
      if (bytesRead > 0) {
        response->write(buf, bytesRead);
      }
      yield(); // Allow the watchdog to be fed
    }
    
    deviceFile.close();
    request->send(response);
  });
  
  // Add an endpoint to retrieve mock device data when API is unavailable
  server.on("/api/netatmo/mock-devices", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling mock devices request"));
    
    // Instead of reading from file, use a hardcoded minimal JSON response
    // This avoids file system operations that might cause yield issues
    static const char MOCK_JSON[] PROGMEM = 
      "{\"body\":{\"devices\":[{\"_id\":\"70:ee:50:00:00:01\",\"station_name\":\"Mock Weather Station\",\"type\":\"NAMain\",\"modules\":[{\"_id\":\"70:ee:50:00:00:02\",\"module_name\":\"Mock Outdoor Module\",\"type\":\"NAModule1\"},{\"_id\":\"70:ee:50:00:00:03\",\"module_name\":\"Mock Indoor Module\",\"type\":\"NAModule4\"}]}]},\"status\":\"ok\"}";
    
    request->send_P(200, "application/json", MOCK_JSON);
  });
  
  // Add an endpoint to test connectivity to Netatmo API
  server.on("/api/netatmo/test-connection", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Testing connection to Netatmo API"));
    
    // Create a simpler response with just DNS resolution test
    // This avoids potentially problematic network operations in the handler
    IPAddress ip;
    bool dnsResolved = WiFi.hostByName("api.netatmo.com", ip);
    
    String response = "{\"tests\":[{\"name\":\"DNS Resolution\",\"success\":";
    response += dnsResolved ? "true" : "false";
    response += ",\"details\":\"";
    response += dnsResolved ? ip.toString() : "Failed to resolve hostname";
    response += "\"}]}";
    
    request->send(200, "application/json", response);
  });
  
  // Add an endpoint to refresh stations data
  server.on("/api/netatmo/refresh-stations", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling refresh stations request"));
    
    // Check if we have a valid access token
    if (strlen(netatmoAccessToken) == 0) {
      Serial.println(F("[NETATMO] Error - No access token"));
      request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
      return;
    }
    
    // Set a flag to fetch stations data in the main loop
    fetchStationsDataPending = true;
    
    // Send a response indicating that the refresh has been initiated
    request->send(200, "application/json", "{\"status\":\"initiated\",\"message\":\"Stations refresh initiated\"}");
  });
  
  // Add an endpoint to get stations data
  server.on("/api/netatmo/stations", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling get stations request"));
    
    // Check if the file exists
    if (!LittleFS.exists("/devices/netatmo_devices.json")) {
      Serial.println(F("[NETATMO] Device data file not found"));
      request->send(404, "application/json", "{\"error\":\"Device data not found\"}");
      return;
    }
    
    // Open the file
    File deviceFile = LittleFS.open("/devices/netatmo_devices.json", "r");
    if (!deviceFile) {
      Serial.println(F("[NETATMO] Failed to open device data file"));
      request->send(500, "application/json", "{\"error\":\"Failed to open device data file\"}");
      return;
    }
    
    // Create a response stream
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    
    // Stream the file content to the response
    const size_t bufSize = 256;
    uint8_t buf[bufSize];
    
    while (deviceFile.available()) {
      size_t bytesRead = deviceFile.read(buf, bufSize);
      if (bytesRead > 0) {
        response->write(buf, bytesRead);
      }
      yield(); // Allow the watchdog to be fed
    }
    
    deviceFile.close();
    request->send(response);
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
  
  // Skip memory checks and defragmentation to avoid yield issues
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  // Memory optimization: Use static client to reduce stack usage
  static BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  // Minimize memory usage by setting smaller buffer sizes
  client.setBufferSizes(512, 512); // Minimum buffer sizes
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  Serial.println(F("[NETATMO] Connecting to token endpoint"));
  if (!https.begin(client, "https://api.netatmo.com/oauth2/token")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Build the redirect URI directly to minimize string operations
  String redirectUri = "http://";
  redirectUri += WiFi.localIP().toString();
  redirectUri += "/api/netatmo/callback";
  
  // Build the POST data in chunks to minimize memory usage
  // Use static buffers to avoid heap fragmentation
  static char postData[512]; // Static buffer for POST data
  memset(postData, 0, sizeof(postData));
  
  // Build the POST data manually to minimize memory usage
  strcat(postData, "grant_type=authorization_code");
  strcat(postData, "&client_id=");
  strcat(postData, netatmoClientId);
  strcat(postData, "&client_secret=");
  strcat(postData, netatmoClientSecret);
  strcat(postData, "&code=");
  strncat(postData, code.c_str(), 100); // Limit code length
  strcat(postData, "&redirect_uri=");
  
  // Add the redirect URI - this is a simplified version without full URL encoding
  // but should work for most cases
  char encodedUri[128];
  snprintf(encodedUri, sizeof(encodedUri), "http%%3A%%2F%%2F%s%%2Fapi%%2Fnetatmo%%2Fcallback", 
           WiFi.localIP().toString().c_str());
  strcat(postData, encodedUri);
  
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
  
  // Set flag to save tokens in main loop instead of saving directly
  saveCredentialsPending = true;
  
  Serial.println(F("[NETATMO] Token exchange complete"));
  
  // Immediately fetch stations data
  fetchStationsData();
}

// Function to be called from loop() to fetch Netatmo devices
void processFetchDevices() {
  if (!fetchDevicesPending) {
    return;
  }
  
  Serial.println(F("[NETATMO] Fetching Netatmo devices"));
  
  // Print memory stats before processing
  Serial.println(F("[NETATMO] Memory stats before fetch devices:"));
  printMemoryStats();
  
  // Defragment heap if needed
  if (shouldDefragment()) {
    Serial.println(F("[NETATMO] Defragmenting heap before fetch devices"));
    defragmentHeap();
  }
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  fetchDevicesPending = false;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    deviceData = F("{\"error\":\"WiFi not connected\"}");
    return;
  }
  
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] Error - No access token"));
    deviceData = F("{\"error\":\"No access token\"}");
    return;
  }
  
  // Create a new client for the API call
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // Increase timeout to 10 seconds
  
  // Use getstationsdata endpoint which we know works
  String apiUrl = "https://api.netatmo.com/api/getstationsdata";
  Serial.print(F("[NETATMO] Fetching device list from: "));
  Serial.println(apiUrl);
  
  if (!https.begin(*client, apiUrl)) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    deviceData = F("{\"error\":\"Failed to connect to Netatmo API\"}");
    return;
  }
  
  // Add authorization header
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  https.addHeader("Authorization", authHeader);
  https.addHeader("Accept", "application/json");
  
  // Make the request with yield to avoid watchdog issues
  Serial.println(F("[NETATMO] Sending request..."));
  int httpCode = https.GET();
  yield(); // Allow the watchdog to be fed
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    
    // Get more detailed error information for connection issues
    if (httpCode == -1) {
      Serial.print(F("[NETATMO] Connection error: "));
      Serial.println(https.errorToString(httpCode));
      
      // Check if we can ping the API server
      Serial.println(F("[NETATMO] Attempting to ping api.netatmo.com..."));
      IPAddress ip;
      bool resolved = WiFi.hostByName("api.netatmo.com", ip);
      
      if (resolved) {
        Serial.print(F("[NETATMO] DNS resolution successful. IP: "));
        Serial.println(ip.toString());
        deviceData = "{\"error\":\"Connection failed. DNS resolved to " + ip.toString() + "\"}";
      } else {
        Serial.println(F("[NETATMO] DNS resolution failed"));
        deviceData = F("{\"error\":\"Connection failed. DNS resolution failed.\"}");
      }
      
      https.end();
      return;
    }
    
    // Get error payload with yield - use streaming to avoid memory issues
    String errorPayload = "";
    if (https.getSize() > 0) {
      const size_t bufSize = 128;
      char buf[bufSize];
      WiFiClient* stream = https.getStreamPtr();
      
      // Read first chunk only for error message
      size_t len = stream->available();
      if (len > bufSize) len = bufSize;
      if (len > 0) {
        int c = stream->readBytes(buf, len);
        if (c > 0) {
          buf[c] = 0;
          errorPayload = String(buf);
        }
      }
    } else {
      errorPayload = https.getString();
    }
    
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    
    // If we get a 401 or 403, try to refresh the token
    if (httpCode == 401 || httpCode == 403) {
      Serial.println(F("[NETATMO] Token expired, trying to refresh"));
      
      // Try to refresh the token
      if (refreshNetatmoToken()) {
        Serial.println(F("[NETATMO] Token refreshed, retrying request"));
        deviceData = F("{\"status\":\"token_refreshed\",\"message\":\"Token refreshed. Please try again.\"}");
      } else {
        Serial.println(F("[NETATMO] Failed to refresh token"));
        deviceData = F("{\"error\":\"Failed to refresh token\"}");
      }
    } else {
      deviceData = "{\"error\":\"API error: " + String(httpCode) + "\"}";
    }
    return;
  }
  
  // Instead of trying to parse the response in memory, let's save it to a file
  // and then parse it in smaller chunks
  
  // First, check if we have enough space
  FSInfo fs_info;
  LittleFS.info(fs_info);
  
  if (fs_info.totalBytes - fs_info.usedBytes < https.getSize() * 2) {
    Serial.println(F("[NETATMO] Not enough space to save device data"));
    deviceData = F("{\"error\":\"Not enough space to save device data\"}");
    https.end();
    return;
  }
  
  // Create the devices directory if it doesn't exist
  if (!LittleFS.exists("/devices")) {
    LittleFS.mkdir("/devices");
  }
  
  // Open a file to save the raw response
  File deviceFile = LittleFS.open("/devices/netatmo_devices.json", "w");
  if (!deviceFile) {
    Serial.println(F("[NETATMO] Failed to open file for writing"));
    deviceData = F("{\"error\":\"Failed to open file for writing\"}");
    https.end();
    return;
  }
  
  // Stream the response directly to the file
  WiFiClient* stream = https.getStreamPtr();
  const size_t bufSize = 256;
  uint8_t buf[bufSize];
  int totalRead = 0;
  int expectedSize = https.getSize();
  
  Serial.print(F("[NETATMO] Expected response size: "));
  Serial.print(expectedSize);
  Serial.println(F(" bytes"));
  
  while (https.connected() && (totalRead < expectedSize || expectedSize <= 0)) {
    // Read available data
    size_t available = stream->available();
    if (available) {
      // Read up to buffer size
      size_t readBytes = available > bufSize ? bufSize : available;
      int bytesRead = stream->readBytes(buf, readBytes);
      
      if (bytesRead > 0) {
        // Write to file
        deviceFile.write(buf, bytesRead);
        totalRead += bytesRead;
        
        // Print progress every 1KB
        if (totalRead % 1024 == 0) {
          Serial.print(F("[NETATMO] Read "));
          Serial.print(totalRead);
          Serial.println(F(" bytes"));
        }
      }
      
      yield(); // Allow the watchdog to be fed
    } else if (totalRead >= expectedSize && expectedSize > 0) {
      // We've read all the data
      break;
    } else {
      // No data available, wait a bit
      delay(10);
      yield();
    }
  }
  
  deviceFile.close();
  https.end();
  
  Serial.print(F("[NETATMO] Device data saved to file, bytes: "));
  Serial.println(totalRead);
  
  // Set a success response
  deviceData = "{\"status\":\"success\",\"message\":\"Device data saved to file\",\"bytes\":" + String(totalRead) + "}";
  
  // Print memory stats after processing
  Serial.println(F("[NETATMO] Memory stats after fetch devices:"));
  printMemoryStats();
}

// Function to be called from loop() to process pending credential saves
void processSaveCredentials() {
  if (!saveCredentialsPending) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing pending credential save"));
  
  // Debug: Print the current values before saving
  Serial.print(F("[NETATMO] Saving netatmoClientId: "));
  Serial.println(netatmoClientId);
  Serial.print(F("[NETATMO] Saving netatmoClientSecret length: "));
  Serial.println(strlen(netatmoClientSecret));
  Serial.print(F("[NETATMO] Saving netatmoAccessToken length: "));
  Serial.println(strlen(netatmoAccessToken));
  Serial.print(F("[NETATMO] Saving netatmoRefreshToken length: "));
  Serial.println(strlen(netatmoRefreshToken));
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  saveCredentialsPending = false;
  
  // Save to config file
  saveTokensToConfig();
  
  Serial.println(F("[NETATMO] Credentials saved to config file"));
  
  // Debug: Verify the values were saved by reading the config file
  if (LittleFS.begin()) {
    File f = LittleFS.open("/config.json", "r");
    if (f) {
      DynamicJsonDocument doc(2048);
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      
      if (!err) {
        Serial.print(F("[NETATMO] Verified netatmoClientId in config: "));
        if (doc.containsKey("netatmoClientId")) {
          Serial.println(doc["netatmoClientId"].as<String>());
        } else {
          Serial.println(F("(not found)"));
        }
        
        Serial.print(F("[NETATMO] Verified netatmoClientSecret in config: "));
        if (doc.containsKey("netatmoClientSecret")) {
          Serial.println(F("(present but not shown)"));
        } else {
          Serial.println(F("(not found)"));
        }
      }
    }
  }
}
// Function to fetch stations data directly
void fetchStationsData() {
  Serial.println(F("[NETATMO] Fetching stations data"));
  
  // Report memory status before API call
  Serial.println(F("[MEMORY] Memory status before API call:"));
  printMemoryStats();
  
  // Defragment heap before making the API call
  Serial.println(F("[MEMORY] Defragmenting heap before API call"));
  defragmentHeap();
  
  // Report memory status after defragmentation
  Serial.println(F("[MEMORY] Memory status after defragmentation:"));
  printMemoryStats();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] Error - No access token"));
    return;
  }
  
  // Create a new client for the API call
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  client->setBufferSizes(1024, 1024); // Increase buffer sizes
  
  HTTPClient https;
  https.setTimeout(15000); // Increase timeout to 15 seconds
  
  // Use homesdata endpoint instead of getstationsdata
  String apiUrl = "https://api.netatmo.com/api/homesdata";
  Serial.print(F("[NETATMO] Fetching from: "));
  Serial.println(apiUrl);
  
  // Log the full request details
  Serial.println(F("[NETATMO] Request details:"));
  Serial.print(F("URL: "));
  Serial.println(apiUrl);
  Serial.print(F("Method: GET"));
  Serial.println();
  Serial.println(F("Headers:"));
  Serial.print(F("  Authorization: Bearer "));
  // Print first 5 chars and last 5 chars of token with ... in between
  if (strlen(netatmoAccessToken) > 10) {
    Serial.print(String(netatmoAccessToken).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(netatmoAccessToken).substring(strlen(netatmoAccessToken) - 5));
  } else {
    Serial.print(netatmoAccessToken[0]);
    Serial.print(F("..."));
    Serial.println(netatmoAccessToken[strlen(netatmoAccessToken)-1]);
  }
  Serial.println(F("  Accept: application/json"));
  
  // Now try the HTTPS connection
  Serial.println(F("[NETATMO] Initializing HTTPS connection..."));
  if (!https.begin(*client, apiUrl)) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  // Add authorization header
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  https.addHeader("Authorization", authHeader);
  https.addHeader("Accept", "application/json");
  
  // Make the request with yield to avoid watchdog issues
  Serial.println(F("[NETATMO] Sending request..."));
  
  // Defragment heap right before making the request
  Serial.println(F("[MEMORY] Final defragmentation before API call"));
  defragmentHeap();
  
  int httpCode = https.GET();
  yield(); // Allow the watchdog to be fed
  
  // Report memory status immediately after the API call
  Serial.println(F("[MEMORY] Memory status immediately after API call:"));
  printMemoryStats();
  
  Serial.print(F("[NETATMO] HTTP response code: "));
  Serial.println(httpCode);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    
    // Get error payload with yield
    String errorPayload = https.getString();
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    
    // If we get a 401 or 403, try to refresh the token
    if (httpCode == 401 || httpCode == 403) {
      Serial.println(F("[NETATMO] Token expired, trying to refresh"));
      
      // Try to refresh the token
      if (refreshNetatmoToken()) {
        Serial.println(F("[NETATMO] Token refreshed, retrying request"));
        fetchStationsData(); // Recursive call after token refresh
      } else {
        Serial.println(F("[NETATMO] Failed to refresh token"));
      }
    }
    
    // Try the getstationsdata endpoint as a fallback
    else if (httpCode == -1 || httpCode == 404) {
      Serial.println(F("[NETATMO] Trying getstationsdata endpoint as fallback"));
      fetchStationsDataFallback();
    }
    
    return;
  }
  
  // Log response headers in detail
  Serial.println(F("[NETATMO] Response headers:"));
  for (int i = 0; i < https.headers(); i++) {
    Serial.print(F("  "));
    Serial.print(https.headerName(i));
    Serial.print(F(": "));
    Serial.println(https.header(i));
  }
  
  // Log content length and type
  Serial.print(F("[NETATMO] Content-Length: "));
  Serial.println(https.getSize());
  Serial.print(F("[NETATMO] Content-Type: "));
  Serial.println(https.header("Content-Type"));
  
  // Create the devices directory if it doesn't exist
  if (!LittleFS.exists("/devices")) {
    LittleFS.mkdir("/devices");
  }
  
  // Open a file to save the raw response
  File deviceFile = LittleFS.open("/devices/netatmo_devices.json", "w");
  if (!deviceFile) {
    Serial.println(F("[NETATMO] Failed to open file for writing"));
    https.end();
    return;
  }
  
  // Stream the response directly to the file
  WiFiClient* stream = https.getStreamPtr();
  const size_t bufSize = 256;
  uint8_t buf[bufSize];
  int totalRead = 0;
  int expectedSize = https.getSize();
  
  Serial.print(F("[NETATMO] Expected response size: "));
  Serial.print(expectedSize);
  Serial.println(F(" bytes"));
  
  // For logging the first part of the response
  String responsePreview = "";
  bool previewCaptured = false;
  
  while (https.connected() && (totalRead < expectedSize || expectedSize <= 0)) {
    // Read available data
    size_t available = stream->available();
    if (available) {
      // Read up to buffer size
      size_t readBytes = available > bufSize ? bufSize : available;
      int bytesRead = stream->readBytes(buf, readBytes);
      
      if (bytesRead > 0) {
        // Write to file
        deviceFile.write(buf, bytesRead);
        totalRead += bytesRead;
        
        // Capture the first part of the response for logging
        if (!previewCaptured && responsePreview.length() < 500) {
          for (int i = 0; i < bytesRead && responsePreview.length() < 500; i++) {
            responsePreview += (char)buf[i];
          }
          if (responsePreview.length() >= 500) {
            previewCaptured = true;
          }
        }
        
        // Print progress every 1KB
        if (totalRead % 1024 == 0) {
          Serial.print(F("[NETATMO] Read "));
          Serial.print(totalRead);
          Serial.println(F(" bytes"));
        }
      }
      
      yield(); // Allow the watchdog to be fed
    } else if (totalRead >= expectedSize && expectedSize > 0) {
      // We've read all the data
      break;
    } else {
      // No data available, wait a bit
      delay(10);
      yield();
    }
  }
  
  deviceFile.close();
  https.end();
  
  Serial.print(F("[NETATMO] Stations data saved to file, bytes: "));
  Serial.println(totalRead);
  
  // Log the first part of the response
  Serial.println(F("[NETATMO] Response preview:"));
  Serial.println(responsePreview);
  
  // Report memory status after API call
  Serial.println(F("[MEMORY] Memory status after API call:"));
  printMemoryStats();
  
  // Defragment heap after API call and before extracting device info
  Serial.println(F("[MEMORY] Defragmenting heap after API call"));
  defragmentHeap();
  
  // Report memory status after defragmentation
  Serial.println(F("[MEMORY] Memory status after post-API defragmentation:"));
  printMemoryStats();
  
  // Now extract the device and module IDs for easy access
  extractDeviceInfo();
}
// Function to extract device info from the saved JSON file
void extractDeviceInfo() {
  Serial.println(F("[NETATMO] Extracting device info"));
  
  if (!LittleFS.exists("/devices/netatmo_devices.json")) {
    Serial.println(F("[NETATMO] Error - Device data file not found"));
    return;
  }
  
  File deviceFile = LittleFS.open("/devices/netatmo_devices.json", "r");
  if (!deviceFile) {
    Serial.println(F("[NETATMO] Error - Failed to open device data file"));
    return;
  }
  
  // Use a streaming parser to avoid loading the entire file into memory
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, deviceFile);
  deviceFile.close();
  
  if (error) {
    Serial.print(F("[NETATMO] Error parsing device data: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Check if we have a homesdata response
  if (doc.containsKey("body") && doc["body"].containsKey("homes")) {
    Serial.println(F("[NETATMO] Processing homesdata response"));
    
    JsonArray homes = doc["body"]["homes"];
    if (homes.size() > 0) {
      JsonObject home = homes[0];
      
      Serial.print(F("[NETATMO] Found home: "));
      Serial.println(home["name"].as<String>());
      
      // Look for modules of type NAMain (weather stations)
      JsonArray modules = home["modules"];
      
      for (JsonObject module : modules) {
        const char* moduleType = module["type"];
        const char* moduleId = module["id"];
        const char* moduleName = module["name"];
        
        Serial.print(F("[NETATMO] Found module: "));
        Serial.print(moduleId);
        Serial.print(F(" ("));
        Serial.print(moduleName);
        Serial.print(F("), type: "));
        Serial.println(moduleType);
        
        // Save main station ID
        if (strcmp(moduleType, "NAMain") == 0 && strlen(netatmoDeviceId) == 0) {
          strlcpy(netatmoDeviceId, moduleId, sizeof(netatmoDeviceId));
          Serial.print(F("[NETATMO] Set main station ID: "));
          Serial.println(netatmoDeviceId);
          
          // Find modules connected to this station
          for (JsonObject subModule : modules) {
            // Check if this module is connected to the main station
            if (subModule.containsKey("bridge") && strcmp(subModule["bridge"], moduleId) == 0) {
              const char* subModuleType = subModule["type"];
              const char* subModuleId = subModule["id"];
              const char* subModuleName = subModule["name"];
              
              Serial.print(F("[NETATMO] Found connected module: "));
              Serial.print(subModuleId);
              Serial.print(F(" ("));
              Serial.print(subModuleName);
              Serial.print(F("), type: "));
              Serial.println(subModuleType);
              
              // Save outdoor module ID
              if (strcmp(subModuleType, "NAModule1") == 0 && strlen(netatmoModuleId) == 0) {
                strlcpy(netatmoModuleId, subModuleId, sizeof(netatmoModuleId));
                Serial.print(F("[NETATMO] Set outdoor module ID: "));
                Serial.println(netatmoModuleId);
              }
              
              // Save indoor module ID
              if (strcmp(subModuleType, "NAModule4") == 0 && strlen(netatmoIndoorModuleId) == 0) {
                strlcpy(netatmoIndoorModuleId, subModuleId, sizeof(netatmoIndoorModuleId));
                Serial.print(F("[NETATMO] Set indoor module ID: "));
                Serial.println(netatmoIndoorModuleId);
              }
            }
          }
        }
      }
    }
  } 
  // Check if we have a getstationsdata response
  else if (doc.containsKey("body") && doc["body"].containsKey("devices")) {
    Serial.println(F("[NETATMO] Processing getstationsdata response"));
    
    JsonArray devices = doc["body"]["devices"];
    if (devices.size() > 0) {
      JsonObject device = devices[0];
      const char* deviceId = device["_id"];
      const char* stationName = device["station_name"];
      
      Serial.print(F("[NETATMO] Found device: "));
      Serial.print(deviceId);
      Serial.print(F(" ("));
      Serial.print(stationName);
      Serial.println(F(")"));
      
      // Save device ID if not already set
      if (strlen(netatmoDeviceId) == 0) {
        strlcpy(netatmoDeviceId, deviceId, sizeof(netatmoDeviceId));
        Serial.print(F("[NETATMO] Set device ID: "));
        Serial.println(netatmoDeviceId);
      }
      
      // Extract module info
      JsonArray modules = device["modules"];
      for (JsonObject module : modules) {
        const char* moduleId = module["_id"];
        const char* moduleName = module["module_name"];
        const char* moduleType = module["type"];
        
        Serial.print(F("[NETATMO] Found module: "));
        Serial.print(moduleId);
        Serial.print(F(" ("));
        Serial.print(moduleName);
        Serial.print(F("), type: "));
        Serial.println(moduleType);
        
        // Save outdoor module ID if not already set
        if (strcmp(moduleType, "NAModule1") == 0 && strlen(netatmoModuleId) == 0) {
          strlcpy(netatmoModuleId, moduleId, sizeof(netatmoModuleId));
          Serial.print(F("[NETATMO] Set outdoor module ID: "));
          Serial.println(netatmoModuleId);
        }
        
        // Save indoor module ID if not already set
        if (strcmp(moduleType, "NAModule4") == 0 && strlen(netatmoIndoorModuleId) == 0) {
          strlcpy(netatmoIndoorModuleId, moduleId, sizeof(netatmoIndoorModuleId));
          Serial.print(F("[NETATMO] Set indoor module ID: "));
          Serial.println(netatmoIndoorModuleId);
        }
      }
    }
  } else {
    Serial.println(F("[NETATMO] Unknown response format"));
  }
  
  // Save the updated device and module IDs to config
  saveTokensToConfig();
}
// Function to be called from loop() to process pending stations data fetch
void processFetchStationsData() {
  if (!fetchStationsDataPending) {
    return;
  }
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  fetchStationsDataPending = false;
  
  // Call the fetch stations data function
  fetchStationsData();
}
// Function to fetch stations data using getstationsdata endpoint as fallback
void fetchStationsDataFallback() {
  Serial.println(F("[NETATMO] Fetching stations data using fallback endpoint"));
  
  // Report memory status before API call
  Serial.println(F("[MEMORY] Memory status before fallback API call:"));
  printMemoryStats();
  
  // Defragment heap before making the API call
  Serial.println(F("[MEMORY] Defragmenting heap before fallback API call"));
  defragmentHeap();
  
  // Report memory status after defragmentation
  Serial.println(F("[MEMORY] Memory status after defragmentation:"));
  printMemoryStats();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] Error - No access token"));
    return;
  }
  
  // Create a new client for the API call
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  client->setBufferSizes(1024, 1024); // Increase buffer sizes
  
  HTTPClient https;
  https.setTimeout(15000); // Increase timeout to 15 seconds
  
  // Use getstationsdata endpoint as fallback
  String apiUrl = "https://api.netatmo.com/api/getstationsdata";
  Serial.print(F("[NETATMO] Fetching from fallback endpoint: "));
  Serial.println(apiUrl);
  
  // Log the full request details
  Serial.println(F("[NETATMO] Fallback request details:"));
  Serial.print(F("URL: "));
  Serial.println(apiUrl);
  Serial.print(F("Method: GET"));
  Serial.println();
  Serial.println(F("Headers:"));
  Serial.print(F("  Authorization: Bearer "));
  // Print first 5 chars and last 5 chars of token with ... in between
  if (strlen(netatmoAccessToken) > 10) {
    Serial.print(String(netatmoAccessToken).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(netatmoAccessToken).substring(strlen(netatmoAccessToken) - 5));
  } else {
    Serial.print(netatmoAccessToken[0]);
    Serial.print(F("..."));
    Serial.println(netatmoAccessToken[strlen(netatmoAccessToken)-1]);
  }
  Serial.println(F("  Accept: application/json"));
  
  // Now try the HTTPS connection
  Serial.println(F("[NETATMO] Initializing HTTPS connection..."));
  if (!https.begin(*client, apiUrl)) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  // Add authorization header
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  https.addHeader("Authorization", authHeader);
  https.addHeader("Accept", "application/json");
  
  // Make the request with yield to avoid watchdog issues
  Serial.println(F("[NETATMO] Sending fallback request..."));
  int httpCode = https.GET();
  yield(); // Allow the watchdog to be fed
  
  Serial.print(F("[NETATMO] HTTP response code: "));
  Serial.println(httpCode);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    
    // Get error payload with yield
    String errorPayload = https.getString();
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    return;
  }
  
  // Log response headers in detail
  Serial.println(F("[NETATMO] Response headers:"));
  for (int i = 0; i < https.headers(); i++) {
    Serial.print(F("  "));
    Serial.print(https.headerName(i));
    Serial.print(F(": "));
    Serial.println(https.header(i));
  }
  
  // Log content length and type
  Serial.print(F("[NETATMO] Content-Length: "));
  Serial.println(https.getSize());
  Serial.print(F("[NETATMO] Content-Type: "));
  Serial.println(https.header("Content-Type"));
  
  // Create the devices directory if it doesn't exist
  if (!LittleFS.exists("/devices")) {
    LittleFS.mkdir("/devices");
  }
  
  // Open a file to save the raw response
  File deviceFile = LittleFS.open("/devices/netatmo_devices.json", "w");
  if (!deviceFile) {
    Serial.println(F("[NETATMO] Failed to open file for writing"));
    https.end();
    return;
  }
  
  // Stream the response directly to the file
  WiFiClient* stream = https.getStreamPtr();
  const size_t bufSize = 256;
  uint8_t buf[bufSize];
  int totalRead = 0;
  int expectedSize = https.getSize();
  
  Serial.print(F("[NETATMO] Expected response size: "));
  Serial.print(expectedSize);
  Serial.println(F(" bytes"));
  
  // For logging the first part of the response
  String responsePreview = "";
  bool previewCaptured = false;
  
  while (https.connected() && (totalRead < expectedSize || expectedSize <= 0)) {
    // Read available data
    size_t available = stream->available();
    if (available) {
      // Read up to buffer size
      size_t readBytes = available > bufSize ? bufSize : available;
      int bytesRead = stream->readBytes(buf, readBytes);
      
      if (bytesRead > 0) {
        // Write to file
        deviceFile.write(buf, bytesRead);
        totalRead += bytesRead;
        
        // Capture the first part of the response for logging
        if (!previewCaptured && responsePreview.length() < 500) {
          for (int i = 0; i < bytesRead && responsePreview.length() < 500; i++) {
            responsePreview += (char)buf[i];
          }
          if (responsePreview.length() >= 500) {
            previewCaptured = true;
          }
        }
        
        // Print progress every 1KB
        if (totalRead % 1024 == 0) {
          Serial.print(F("[NETATMO] Read "));
          Serial.print(totalRead);
          Serial.println(F(" bytes"));
        }
      }
      
      yield(); // Allow the watchdog to be fed
    } else if (totalRead >= expectedSize && expectedSize > 0) {
      // We've read all the data
      break;
    } else {
      // No data available, wait a bit
      delay(10);
      yield();
    }
  }
  
  deviceFile.close();
  https.end();
  
  Serial.print(F("[NETATMO] Fallback stations data saved to file, bytes: "));
  Serial.println(totalRead);
  
  // Log the first part of the response
  Serial.println(F("[NETATMO] Fallback response preview:"));
  Serial.println(responsePreview);
  
  // Report memory status after API call
  Serial.println(F("[MEMORY] Memory status after fallback API call:"));
  printMemoryStats();
  
  // Defragment heap after API call and before extracting device info
  Serial.println(F("[MEMORY] Defragmenting heap after fallback API call"));
  defragmentHeap();
  
  // Report memory status after defragmentation
  Serial.println(F("[MEMORY] Memory status after post-API defragmentation:"));
  printMemoryStats();
  
  // Now extract the device and module IDs for easy access
  extractDeviceInfo();
}
