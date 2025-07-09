// Simplified token exchange function
void processTokenExchangeSimple() {
  if (!tokenExchangePending || pendingCode.isEmpty()) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing token exchange (simplified)"));
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  tokenExchangePending = false;
  String code = pendingCode;
  pendingCode = "";
  
  // Report memory status before token exchange
  Serial.println(F("[MEMORY] Memory status before token exchange:"));
  printMemoryStats();
  
  // Defragment heap before token exchange
  Serial.println(F("[MEMORY] Defragmenting heap before token exchange"));
  defragmentHeap();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Error - WiFi not connected"));
    return;
  }
  
  // Create a new client for the API call
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  Serial.println(F("[NETATMO] Connecting to token endpoint"));
  if (!https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Build the redirect URI
  String redirectUri = "http://";
  redirectUri += WiFi.localIP().toString();
  redirectUri += "/api/netatmo/callback";
  
  // Build the POST data
  String postData = "grant_type=authorization_code";
  postData += "&client_id=";
  postData += netatmoClientId;
  postData += "&client_secret=";
  postData += netatmoClientSecret;
  postData += "&code=";
  postData += code;
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
  
  // Get the response as a string
  String response = https.getString();
  https.end();
  
  Serial.println(F("[NETATMO] Parsing response"));
  
  // Use a larger JSON buffer
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.print(F("[NETATMO] JSON parse error: "));
    Serial.println(error.c_str());
    
    // Log the response for debugging
    Serial.print(F("[NETATMO] Response: "));
    Serial.println(response);
    return;
  }
  
  // Extract the tokens
  if (!doc.containsKey("access_token") || !doc.containsKey("refresh_token")) {
    Serial.println(F("[NETATMO] Missing tokens in response"));
    return;
  }
  
  // Save the tokens
  Serial.println(F("[NETATMO] Saving tokens"));
  strlcpy(netatmoAccessToken, doc["access_token"], sizeof(netatmoAccessToken));
  strlcpy(netatmoRefreshToken, doc["refresh_token"], sizeof(netatmoRefreshToken));
  
  // Save the tokens to config
  saveTokensToConfig();
  
  Serial.println(F("[NETATMO] Token exchange complete"));
  
  // Set a flag to fetch stations data
  fetchStationsDataPending = true;
}
