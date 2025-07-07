// Function to debug Netatmo authentication issues
void debugNetatmoAuth() {
  Serial.println(F("\n[NETATMO DEBUG] Starting authentication debug..."));
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO DEBUG] ERROR: WiFi not connected"));
    return;
  } else {
    Serial.print(F("[NETATMO DEBUG] WiFi connected, IP: "));
    Serial.println(WiFi.localIP());
  }
  
  // Check credentials
  Serial.println(F("[NETATMO DEBUG] Checking credentials:"));
  
  Serial.print(F("[NETATMO DEBUG] Client ID length: "));
  Serial.println(strlen(netatmoClientId));
  if (strlen(netatmoClientId) < 10) {
    Serial.println(F("[NETATMO DEBUG] ERROR: Client ID missing or too short"));
  }
  
  Serial.print(F("[NETATMO DEBUG] Client Secret length: "));
  Serial.println(strlen(netatmoClientSecret));
  if (strlen(netatmoClientSecret) < 10) {
    Serial.println(F("[NETATMO DEBUG] ERROR: Client Secret missing or too short"));
  }
  
  Serial.print(F("[NETATMO DEBUG] Username length: "));
  Serial.println(strlen(netatmoUsername));
  if (strlen(netatmoUsername) < 5) {
    Serial.println(F("[NETATMO DEBUG] ERROR: Username missing or too short"));
  }
  
  Serial.print(F("[NETATMO DEBUG] Password length: "));
  Serial.println(strlen(netatmoPassword));
  if (strlen(netatmoPassword) < 5) {
    Serial.println(F("[NETATMO DEBUG] ERROR: Password missing or too short"));
  }
  
  // Try direct token request with verbose logging
  Serial.println(F("[NETATMO DEBUG] Attempting direct token request..."));
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  
  if (https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "ESPTimeCast/1.0");
    
    // Use password flow
    String postData = "grant_type=password&client_id=";
    postData += netatmoClientId;
    postData += "&client_secret=";
    postData += netatmoClientSecret;
    postData += "&username=";
    postData += netatmoUsername;
    postData += "&password=";
    postData += netatmoPassword;
    postData += "&scope=read_station read_thermostat read_homecoach";
    
    Serial.println(F("[NETATMO DEBUG] Sending POST request..."));
    int httpCode = https.POST(postData);
    
    Serial.print(F("[NETATMO DEBUG] HTTP response code: "));
    Serial.println(httpCode);
    
    String payload = https.getString();
    Serial.println(F("[NETATMO DEBUG] Response:"));
    Serial.println(payload);
    
    if (httpCode == HTTP_CODE_OK) {
      Serial.println(F("[NETATMO DEBUG] SUCCESS: Token request successful"));
      
      // Try to parse the response
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if (doc.containsKey("access_token")) {
          String token = doc["access_token"].as<String>();
          Serial.print(F("[NETATMO DEBUG] Access token received (length: "));
          Serial.print(token.length());
          Serial.println(F(")"));
          
          // Store the tokens
          strlcpy(netatmoAccessToken, token.c_str(), sizeof(netatmoAccessToken));
          
          if (doc.containsKey("refresh_token")) {
            String refreshToken = doc["refresh_token"].as<String>();
            Serial.print(F("[NETATMO DEBUG] Refresh token received (length: "));
            Serial.print(refreshToken.length());
            Serial.println(F(")"));
            
            strlcpy(netatmoRefreshToken, refreshToken.c_str(), sizeof(netatmoRefreshToken));
            
            // Save the tokens to config.json
            saveTokensToConfig();
            Serial.println(F("[NETATMO DEBUG] Tokens saved to config"));
          } else {
            Serial.println(F("[NETATMO DEBUG] WARNING: No refresh token in response"));
          }
        } else {
          Serial.println(F("[NETATMO DEBUG] ERROR: No access token in response"));
        }
      } else {
        Serial.print(F("[NETATMO DEBUG] ERROR: JSON parse error: "));
        Serial.println(error.c_str());
      }
    } else {
      Serial.println(F("[NETATMO DEBUG] ERROR: Token request failed"));
      
      // Try to parse the error
      DynamicJsonDocument errorDoc(2048);
      DeserializationError errorParseError = deserializeJson(errorDoc, payload);
      
      if (!errorParseError && errorDoc.containsKey("error")) {
        String errorType = errorDoc["error"].as<String>();
        String errorMessage = errorDoc.containsKey("error_description") ? 
                             errorDoc["error_description"].as<String>() : "Unknown error";
        
        Serial.print(F("[NETATMO DEBUG] Error type: "));
        Serial.println(errorType);
        Serial.print(F("[NETATMO DEBUG] Error message: "));
        Serial.println(errorMessage);
        
        if (errorType == "invalid_client") {
          Serial.println(F("[NETATMO DEBUG] ERROR: Invalid client ID or client secret"));
        } else if (errorType == "invalid_grant") {
          Serial.println(F("[NETATMO DEBUG] ERROR: Invalid username or password"));
        } else if (errorType == "invalid_scope") {
          Serial.println(F("[NETATMO DEBUG] ERROR: Invalid scope requested"));
        }
      }
    }
    
    https.end();
  } else {
    Serial.println(F("[NETATMO DEBUG] ERROR: Failed to connect to Netatmo API"));
  }
  
  Serial.println(F("[NETATMO DEBUG] Authentication debug complete"));
}
