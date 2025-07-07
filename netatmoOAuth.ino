// Netatmo OAuth2 implementation

void handleNetatmoAuth(AsyncWebServerRequest *request) {
  Serial.println(F("[NETATMO] Handling OAuth authorization request"));
  
  // Get client ID from config
  if (!LittleFS.begin()) {
    request->send(500, "text/html", "<html><body><h1>Error</h1><p>Failed to mount file system</p></body></html>");
    return;
  }
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    request->send(500, "text/html", "<html><body><h1>Error</h1><p>Failed to open config file</p></body></html>");
    return;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  
  if (err) {
    request->send(500, "text/html", "<html><body><h1>Error</h1><p>Failed to parse config file</p></body></html>");
    return;
  }
  
  String clientId = doc["netatmoClientId"].as<String>();
  if (clientId.length() == 0) {
    request->send(400, "text/html", "<html><body><h1>Error</h1><p>Netatmo Client ID not configured</p></body></html>");
    return;
  }
  
  // Get the device's IP address for the redirect URI
  String ipAddress = WiFi.localIP().toString();
  String redirectUri = "http://" + ipAddress + "/oauth2/callback";
  
  // Build the authorization URL
  String authUrl = "https://api.netatmo.com/oauth2/authorize?";
  authUrl += "client_id=" + clientId;
  authUrl += "&redirect_uri=" + redirectUri;
  authUrl += "&scope=read_station";
  authUrl += "&state=ESPTimeCast";
  
  Serial.print(F("[NETATMO] Redirecting to: "));
  Serial.println(authUrl);
  
  // Redirect to Netatmo authorization page
  request->redirect(authUrl.c_str());
}

void handleNetatmoCallback(AsyncWebServerRequest *request) {
  Serial.println(F("[NETATMO] Handling OAuth callback"));
  
  // Check for error
  if (request->hasParam("error")) {
    String error = request->getParam("error")->value();
    String errorDescription = request->hasParam("error_description") ? request->getParam("error_description")->value() : "Unknown error";
    
    Serial.print(F("[NETATMO] Authorization error: "));
    Serial.print(error);
    Serial.print(F(" - "));
    Serial.println(errorDescription);
    
    String html = "<html><body><h1>Authorization Failed</h1>";
    html += "<p>Error: " + error + "</p>";
    html += "<p>Description: " + errorDescription + "</p>";
    html += "<p><a href='/'>Return to configuration</a></p>";
    html += "</body></html>";
    
    request->send(400, "text/html", html);
    return;
  }
  
  // Check for authorization code
  if (!request->hasParam("code")) {
    Serial.println(F("[NETATMO] No authorization code received"));
    request->send(400, "text/html", "<html><body><h1>Error</h1><p>No authorization code received</p></body></html>");
    return;
  }
  
  String code = request->getParam("code")->value();
  Serial.print(F("[NETATMO] Received authorization code: "));
  Serial.println(code);
  
  // Send a response immediately to prevent timeout
  String html = "<html><body><h1>Processing Authorization</h1>";
  html += "<p>Processing your Netatmo authorization...</p>";
  html += "<p>This may take a moment. Please wait.</p>";
  html += "<script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>";
  html += "</body></html>";
  
  request->send(200, "text/html", html);
  
  // Process the authorization code asynchronously
  processNetatmoCode(code);
}

// Process Netatmo authorization code asynchronously
void processNetatmoCode(String code) {
  // Get client ID and secret from config
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] Failed to mount file system"));
    return;
  }
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println(F("[NETATMO] Failed to open config file"));
    return;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  
  if (err) {
    Serial.println(F("[NETATMO] Failed to parse config file"));
    return;
  }
  
  String clientId = doc["netatmoClientId"].as<String>();
  String clientSecret = doc["netatmoClientSecret"].as<String>();
  
  if (clientId.length() == 0 || clientSecret.length() == 0) {
    Serial.println(F("[NETATMO] Missing client credentials"));
    return;
  }
  
  // Get the device's IP address for the redirect URI
  String ipAddress = WiFi.localIP().toString();
  String redirectUri = "http://" + ipAddress + "/oauth2/callback";
  
  // Exchange the authorization code for tokens with timeout protection
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, "https://api.netatmo.com/oauth2/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(10000); // 10 second timeout
  
  String postData = "grant_type=authorization_code";
  postData += "&client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&code=" + code;
  postData += "&redirect_uri=" + redirectUri;
  postData += "&scope=read_station";
  
  Serial.println(F("[NETATMO] Exchanging code for tokens"));
  int httpCode = http.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    String error = "HTTP error: " + String(httpCode);
    String payload = http.getString();
    
    Serial.print(F("[NETATMO] Token exchange failed: "));
    Serial.println(error);
    Serial.print(F("[NETATMO] Response: "));
    Serial.println(payload);
    
    http.end();
    return;
  }
  
  String payload = http.getString();
  http.end();
  
  Serial.print(F("[NETATMO] Token response: "));
  Serial.println(payload);
  
  // Parse the response
  DynamicJsonDocument tokenDoc(1024);
  DeserializationError tokenErr = deserializeJson(tokenDoc, payload);
  
  if (tokenErr) {
    Serial.print(F("[NETATMO] Failed to parse token response: "));
    Serial.println(tokenErr.c_str());
    return;
  }
  
  // Extract tokens
  String accessToken = tokenDoc["access_token"].as<String>();
  String refreshToken = tokenDoc["refresh_token"].as<String>();
  
  if (accessToken.length() == 0 || refreshToken.length() == 0) {
    Serial.println(F("[NETATMO] Invalid token response - missing tokens"));
    return;
  }
  
  Serial.println(F("[NETATMO] Successfully obtained tokens"));
  
  // Save tokens to config
  doc["netatmoAccessToken"] = accessToken;
  doc["netatmoRefreshToken"] = refreshToken;
  
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("[NETATMO] Failed to open config file for writing"));
    return;
  }
  
  if (serializeJsonPretty(doc, configFile) == 0) {
    configFile.close();
    Serial.println(F("[NETATMO] Failed to write config file"));
    return;
  }
  
  configFile.close();
  Serial.println(F("[NETATMO] Tokens saved to config"));
}

// Function to refresh Netatmo token
bool refreshNetatmoToken() {
  Serial.println(F("[NETATMO] Refreshing token"));
  
  // Get refresh token from config
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] Failed to mount file system"));
    return false;
  }
  
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println(F("[NETATMO] Failed to open config file"));
    return false;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  
  if (err) {
    Serial.print(F("[NETATMO] Failed to parse config file: "));
    Serial.println(err.c_str());
    return false;
  }
  
  String clientId = doc["netatmoClientId"].as<String>();
  String clientSecret = doc["netatmoClientSecret"].as<String>();
  String refreshToken = doc["netatmoRefreshToken"].as<String>();
  
  if (clientId.length() == 0 || clientSecret.length() == 0 || refreshToken.length() == 0) {
    Serial.println(F("[NETATMO] Missing client credentials or refresh token"));
    return false;
  }
  
  // Exchange the refresh token for a new access token
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, "https://api.netatmo.com/oauth2/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(10000); // 10 second timeout
  
  String postData = "grant_type=refresh_token";
  postData += "&client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&refresh_token=" + refreshToken;
  
  int httpCode = http.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Token refresh failed: HTTP error "));
    Serial.println(httpCode);
    http.end();
    return false;
  }
  
  String payload = http.getString();
  http.end();
  
  // Parse the response
  DynamicJsonDocument tokenDoc(1024);
  DeserializationError tokenErr = deserializeJson(tokenDoc, payload);
  
  if (tokenErr) {
    Serial.print(F("[NETATMO] Failed to parse token response: "));
    Serial.println(tokenErr.c_str());
    return false;
  }
  
  // Extract tokens
  String accessToken = tokenDoc["access_token"].as<String>();
  String newRefreshToken = tokenDoc["refresh_token"].as<String>();
  
  if (accessToken.length() == 0) {
    Serial.println(F("[NETATMO] Invalid token response - missing access token"));
    return false;
  }
  
  // Save tokens to config
  doc["netatmoAccessToken"] = accessToken;
  if (newRefreshToken.length() > 0) {
    doc["netatmoRefreshToken"] = newRefreshToken;
  }
  
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("[NETATMO] Failed to open config file for writing"));
    return false;
  }
  
  if (serializeJsonPretty(doc, configFile) == 0) {
    configFile.close();
    Serial.println(F("[NETATMO] Failed to write config file"));
    return false;
  }
  
  configFile.close();
  Serial.println(F("[NETATMO] Token refreshed successfully"));
  return true;
}
