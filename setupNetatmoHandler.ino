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
  
  // Endpoint to save Netatmo credentials
  server.on("/api/netatmo/save-credentials", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling save credentials request"));
    
    if (!request->hasParam("clientId", true) || !request->hasParam("clientSecret", true)) {
      Serial.println(F("[NETATMO] Error - Missing required parameters"));
      request->send(400, "text/plain", "Missing required parameters");
      return;
    }
    
    String clientId = request->getParam("clientId", true)->value();
    String clientSecret = request->getParam("clientSecret", true)->value();
    
    Serial.println(F("[NETATMO] Saving credentials to config"));
    
    // Save to global variables
    strlcpy(netatmoClientId, clientId.c_str(), sizeof(netatmoClientId));
    strlcpy(netatmoClientSecret, clientSecret.c_str(), sizeof(netatmoClientSecret));
    
    // Save to config file
    saveTokensToConfig();
    
    Serial.println(F("[NETATMO] Redirecting to auth endpoint"));
    request->redirect("/api/netatmo/auth");
  });
  
  // Endpoint to initiate OAuth flow
  server.on("/api/netatmo/auth", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling auth request"));
    
    if (strlen(netatmoClientId) == 0 || strlen(netatmoClientSecret) == 0) {
      Serial.println(F("[NETATMO] Error - No valid credentials"));
      request->send(400, "text/plain", "No valid Netatmo credentials found");
      return;
    }
    
    // Generate authorization URL
    String redirectUri = "http://" + WiFi.localIP().toString() + "/api/netatmo/callback";
    String authUrl = "https://api.netatmo.com/oauth2/authorize";
    authUrl += "?client_id=" + urlEncode(netatmoClientId);
    authUrl += "&redirect_uri=" + urlEncode(redirectUri);
    authUrl += "&scope=" + urlEncode("read_station");
    authUrl += "&response_type=code";
    
    Serial.print(F("[NETATMO] Auth URL: "));
    Serial.println(authUrl);
    
    Serial.println(F("[NETATMO] Redirecting to Netatmo authorization page"));
    request->redirect(authUrl);
  });
  
  // Endpoint to handle OAuth callback
  server.on("/api/netatmo/callback", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling OAuth callback"));
    
    if (!request->hasParam("code")) {
      Serial.println(F("[NETATMO] Error - No authorization code in callback"));
      if (request->hasParam("error")) {
        String error = request->getParam("error")->value();
        String errorDescription = request->hasParam("error_description") ? 
                                request->getParam("error_description")->value() : "Unknown error";
        Serial.print(F("[NETATMO] OAuth error: "));
        Serial.print(error);
        Serial.print(F(" - "));
        Serial.println(errorDescription);
      }
      request->send(400, "text/plain", "Authorization failed");
      return;
    }
    
    String code = request->getParam("code")->value();
    Serial.println(F("[NETATMO] Received authorization code, exchanging for tokens"));
    
    // Exchange code for tokens (will be handled asynchronously)
    exchangeAuthCode(code);
    
    // Redirect back to Netatmo page
    request->redirect("/netatmo.html");
  });
  
  Serial.println(F("[NETATMO] OAuth handler setup complete"));
}

// Helper function to URL encode a string
String urlEncode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String result = "";
  
  for (size_t i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
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

// Global variables for token exchange
static String pendingCode = "";
static bool tokenExchangePending = false;

// Function to exchange authorization code for tokens
void exchangeAuthCode(const String &code) {
  Serial.println(F("[NETATMO] Starting token exchange process"));
  
  // This will be called in the next loop iteration
  pendingCode = code;
  
  // Set a flag to process this in the main loop
  tokenExchangePending = true;
}

// Function to be called from loop() to process pending token exchange
void processTokenExchange() {
  if (!tokenExchangePending || pendingCode.isEmpty()) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing token exchange"));
  tokenExchangePending = false;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  // Set up the HTTPS client
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  Serial.println(F("[NETATMO] Connecting to Netatmo token endpoint"));
  if (!https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    Serial.println(F("[NETATMO] Error - Failed to connect to token endpoint"));
    return;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Construct the POST data
  String redirectUri = "http://" + WiFi.localIP().toString() + "/api/netatmo/callback";
  String postData = "grant_type=authorization_code";
  postData += "&client_id=" + urlEncode(netatmoClientId);
  postData += "&client_secret=" + urlEncode(netatmoClientSecret);
  postData += "&code=" + urlEncode(pendingCode);
  postData += "&redirect_uri=" + urlEncode(redirectUri);
  
  Serial.println(F("[NETATMO] Sending token exchange request"));
  int httpCode = https.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - Token exchange failed with code: "));
    Serial.println(httpCode);
    if (httpCode > 0) {
      Serial.print(F("[NETATMO] Response: "));
      Serial.println(https.getString());
    }
    https.end();
    return;
  }
  
  // Get the response
  Serial.println(F("[NETATMO] Token exchange successful, parsing response"));
  String response = https.getString();
  https.end();
  
  // Parse the response
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.print(F("[NETATMO] Error parsing JSON: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Extract the tokens
  if (!doc.containsKey("access_token") || !doc.containsKey("refresh_token")) {
    Serial.println(F("[NETATMO] Error - Response missing required tokens"));
    return;
  }
  
  String accessToken = doc["access_token"].as<String>();
  String refreshToken = doc["refresh_token"].as<String>();
  
  // Save the tokens
  Serial.println(F("[NETATMO] Saving tokens to config"));
  strlcpy(netatmoAccessToken, accessToken.c_str(), sizeof(netatmoAccessToken));
  strlcpy(netatmoRefreshToken, refreshToken.c_str(), sizeof(netatmoRefreshToken));
  
  saveTokensToConfig();
  Serial.println(F("[NETATMO] Token exchange complete"));
  
  // Clear the pending code
  pendingCode = "";
}
