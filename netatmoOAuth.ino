// Function to generate a random state string for OAuth2
String generateRandomState() {
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  String state = "";
  for (int i = 0; i < 16; i++) {
    state += charset[random(0, sizeof(charset) - 1)];
  }
  return state;
}

// Function to handle Netatmo OAuth2 authorization
void handleNetatmoAuth(AsyncWebServerRequest *request) {
  Serial.println(F("[NETATMO] Handling OAuth2 authorization request"));
  
  // Check if we have client ID
  if (strlen(netatmoClientId) == 0) {
    request->send(400, "application/json", "{\"error\":\"Client ID not configured\"}");
    return;
  }
  
  // Generate a random state for CSRF protection
  String state = generateRandomState();
  
  // Store the state in a file for later verification
  File stateFile = LittleFS.open("/oauth_state.txt", "w");
  if (!stateFile) {
    Serial.println(F("[NETATMO] Failed to create state file"));
    request->send(500, "application/json", "{\"error\":\"Failed to create state file\"}");
    return;
  }
  stateFile.print(state);
  stateFile.close();
  
  // Build the authorization URL
  String authUrl = "https://api.netatmo.com/oauth2/authorize?";
  authUrl += "client_id=" + String(netatmoClientId);
  authUrl += "&redirect_uri=http://" + WiFi.localIP().toString() + "/oauth2/callback";
  authUrl += "&scope=read_station%20read_thermostat%20read_homecoach";
  authUrl += "&state=" + state;
  authUrl += "&response_type=code";
  
  Serial.print(F("[NETATMO] Authorization URL: "));
  Serial.println(authUrl);
  
  // Send the URL to the client
  String response = "{\"auth_url\":\"" + authUrl + "\"}";
  request->send(200, "application/json", response);
}

// Function to handle the OAuth2 callback
void handleNetatmoCallback(AsyncWebServerRequest *request) {
  Serial.println(F("[NETATMO] Handling OAuth2 callback"));
  
  // Check for error parameter
  if (request->hasParam("error")) {
    String error = request->getParam("error")->value();
    Serial.print(F("[NETATMO] OAuth2 error: "));
    Serial.println(error);
    
    // Redirect to the configuration page with error
    request->redirect("/?oauth_error=" + error);
    return;
  }
  
  // Check for required parameters
  if (!request->hasParam("code") || !request->hasParam("state")) {
    Serial.println(F("[NETATMO] Missing required parameters"));
    request->redirect("/?oauth_error=missing_parameters");
    return;
  }
  
  // Get the authorization code and state
  String code = request->getParam("code")->value();
  String state = request->getParam("state")->value();
  
  // Verify the state
  File stateFile = LittleFS.open("/oauth_state.txt", "r");
  if (!stateFile) {
    Serial.println(F("[NETATMO] Failed to open state file"));
    request->redirect("/?oauth_error=state_verification_failed");
    return;
  }
  
  String savedState = stateFile.readString();
  stateFile.close();
  
  if (state != savedState) {
    Serial.println(F("[NETATMO] State mismatch, possible CSRF attack"));
    request->redirect("/?oauth_error=state_mismatch");
    return;
  }
  
  // Exchange the code for tokens
  bool success = exchangeCodeForTokens(code);
  
  if (success) {
    // Redirect to the configuration page with success message
    request->redirect("/?oauth_success=true");
  } else {
    // Redirect to the configuration page with error
    request->redirect("/?oauth_error=token_exchange_failed");
  }
}

// Function to exchange the authorization code for tokens
bool exchangeCodeForTokens(String code) {
  Serial.println(F("[NETATMO] Exchanging code for tokens"));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] WiFi not connected"));
    return false;
  }
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  
  if (https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "ESPTimeCast/1.0");
    
    String postData = "grant_type=authorization_code";
    postData += "&client_id=" + String(netatmoClientId);
    postData += "&client_secret=" + String(netatmoClientSecret);
    postData += "&code=" + code;
    postData += "&redirect_uri=http://" + WiFi.localIP().toString() + "/oauth2/callback";
    
    Serial.println(F("[NETATMO] Sending token request..."));
    int httpCode = https.POST(postData);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      Serial.println(F("[NETATMO] Token response received"));
      
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if (doc.containsKey("access_token") && doc.containsKey("refresh_token")) {
          String accessToken = doc["access_token"].as<String>();
          String refreshToken = doc["refresh_token"].as<String>();
          
          // Store the tokens
          strlcpy(netatmoAccessToken, accessToken.c_str(), sizeof(netatmoAccessToken));
          strlcpy(netatmoRefreshToken, refreshToken.c_str(), sizeof(netatmoRefreshToken));
          
          // Save the tokens to config
          saveTokensToConfig();
          
          Serial.println(F("[NETATMO] Tokens obtained and saved successfully"));
          return true;
        } else {
          Serial.println(F("[NETATMO] Missing tokens in response"));
        }
      } else {
        Serial.print(F("[NETATMO] JSON parse error: "));
        Serial.println(error.c_str());
      }
    } else {
      Serial.print(F("[NETATMO] HTTP error: "));
      Serial.println(httpCode);
      
      String errorPayload = https.getString();
      Serial.println(F("[NETATMO] Error response:"));
      Serial.println(errorPayload);
    }
    
    https.end();
  } else {
    Serial.println(F("[NETATMO] Unable to connect to Netatmo API"));
  }
  
  return false;
}
