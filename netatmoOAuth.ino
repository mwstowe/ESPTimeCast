// Netatmo OAuth2 implementation

void handleNetatmoAuth(AsyncWebServerRequest *request) {
  Serial.println(F("[NETATMO] Handling OAuth authorization request"));
  Serial.print(F("[MEMORY] Free heap before auth: "));
  Serial.println(ESP.getFreeHeap());
  
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
  
  // Read the file in smaller chunks to avoid memory issues
  String clientId = "";
  bool foundClientId = false;
  
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (line.indexOf("\"netatmoClientId\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      clientId = line.substring(startPos, endPos);
      clientId.trim();
      
      // Remove quotes if present
      if (clientId.startsWith("\"") && clientId.endsWith("\"")) {
        clientId = clientId.substring(1, clientId.length() - 1);
      }
      
      foundClientId = true;
      break;
    }
  }
  
  f.close();
  
  if (!foundClientId || clientId.length() == 0) {
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
  
  Serial.print(F("[MEMORY] Free heap before redirect: "));
  Serial.println(ESP.getFreeHeap());
  
  // Redirect to Netatmo authorization page
  request->redirect(authUrl.c_str());
  
  Serial.print(F("[MEMORY] Free heap after redirect: "));
  Serial.println(ESP.getFreeHeap());
}

void handleNetatmoCallback(AsyncWebServerRequest *request) {
  Serial.println(F("[NETATMO] Handling OAuth callback"));
  Serial.print(F("[MEMORY] Free heap in callback: "));
  Serial.println(ESP.getFreeHeap());
  
  // Send a simple response immediately to prevent timeout
  String html = "<html><body><h1>Authorization Received</h1>";
  html += "<p>Your authorization code has been received.</p>";
  html += "<p>Please wait while we process it...</p>";
  html += "<p>You will be redirected to the configuration page shortly.</p>";
  html += "<script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>";
  html += "</body></html>";
  
  request->send(200, "text/html", html);
  
  // Extract the authorization code
  if (request->hasParam("code")) {
    String code = request->getParam("code")->value();
    Serial.print(F("[NETATMO] Received authorization code: "));
    Serial.println(code);
    
    // Schedule the token exchange for later to avoid memory issues
    scheduleTokenExchange(code);
  } else if (request->hasParam("error")) {
    String error = request->getParam("error")->value();
    String errorDescription = request->hasParam("error_description") ? request->getParam("error_description")->value() : "Unknown error";
    
    Serial.print(F("[NETATMO] Authorization error: "));
    Serial.print(error);
    Serial.print(F(" - "));
    Serial.println(errorDescription);
  } else {
    Serial.println(F("[NETATMO] No authorization code or error received"));
  }
}

// Variables for scheduled token exchange
String pendingAuthCode = "";
unsigned long tokenExchangeTime = 0;

// Schedule a token exchange for later
void scheduleTokenExchange(String code) {
  pendingAuthCode = code;
  tokenExchangeTime = millis() + 5000; // Schedule for 5 seconds later
  Serial.println(F("[NETATMO] Token exchange scheduled"));
}

// Check if there's a pending token exchange
void checkPendingTokenExchange() {
  if (pendingAuthCode.length() > 0 && millis() > tokenExchangeTime) {
    Serial.println(F("[NETATMO] Processing scheduled token exchange"));
    Serial.print(F("[MEMORY] Free heap before token exchange: "));
    Serial.println(ESP.getFreeHeap());
    
    processNetatmoCode(pendingAuthCode);
    pendingAuthCode = ""; // Clear the pending code
  }
}

// Process Netatmo authorization code
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
  
  // Read the file in smaller chunks to avoid memory issues
  String clientId = "";
  String clientSecret = "";
  bool foundClientId = false;
  bool foundClientSecret = false;
  
  while (f.available()) {
    String line = f.readStringUntil('\n');
    
    if (!foundClientId && line.indexOf("\"netatmoClientId\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      clientId = line.substring(startPos, endPos);
      clientId.trim();
      
      // Remove quotes if present
      if (clientId.startsWith("\"") && clientId.endsWith("\"")) {
        clientId = clientId.substring(1, clientId.length() - 1);
      }
      
      foundClientId = true;
    }
    
    if (!foundClientSecret && line.indexOf("\"netatmoClientSecret\"") >= 0) {
      int startPos = line.indexOf(":") + 1;
      int endPos = line.indexOf(",");
      if (endPos < 0) endPos = line.indexOf("}");
      if (endPos < 0) endPos = line.length() - 1;
      
      clientSecret = line.substring(startPos, endPos);
      clientSecret.trim();
      
      // Remove quotes if present
      if (clientSecret.startsWith("\"") && clientSecret.endsWith("\"")) {
        clientSecret = clientSecret.substring(1, clientSecret.length() - 1);
      }
      
      foundClientSecret = true;
    }
    
    if (foundClientId && foundClientSecret) break;
  }
  
  f.close();
  
  if (clientId.length() == 0 || clientSecret.length() == 0) {
    Serial.println(F("[NETATMO] Missing client credentials"));
    return;
  }
  
  // Get the device's IP address for the redirect URI
  String ipAddress = WiFi.localIP().toString();
  String redirectUri = "http://" + ipAddress + "/oauth2/callback";
  
  // Exchange the authorization code for tokens
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  Serial.println(F("[NETATMO] Connecting to api.netatmo.com..."));
  
  if (!client.connect("api.netatmo.com", 443)) {
    Serial.println(F("[NETATMO] Connection failed"));
    return;
  }
  
  Serial.println(F("[NETATMO] Connected to api.netatmo.com"));
  Serial.print(F("[MEMORY] Free heap after connection: "));
  Serial.println(ESP.getFreeHeap());
  
  // Prepare the POST data
  String postData = "grant_type=authorization_code";
  postData += "&client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&code=" + code;
  postData += "&redirect_uri=" + redirectUri;
  postData += "&scope=read_station";
  
  // Prepare the HTTP request
  String request = "POST /oauth2/token HTTP/1.1\r\n";
  request += "Host: api.netatmo.com\r\n";
  request += "Connection: close\r\n";
  request += "Content-Type: application/x-www-form-urlencoded\r\n";
  request += "Content-Length: " + String(postData.length()) + "\r\n";
  request += "\r\n";
  request += postData;
  
  Serial.println(F("[NETATMO] Sending token request..."));
  client.print(request);
  
  Serial.print(F("[MEMORY] Free heap after request: "));
  Serial.println(ESP.getFreeHeap());
  
  // Wait for the response
  unsigned long timeout = millis() + 10000; // 10 second timeout
  while (client.available() == 0) {
    if (millis() > timeout) {
      Serial.println(F("[NETATMO] Client timeout"));
      client.stop();
      return;
    }
    delay(10);
  }
  
  Serial.println(F("[NETATMO] Response received"));
  
  // Skip HTTP headers
  String line;
  do {
    line = client.readStringUntil('\n');
  } while (line != "\r");
  
  // Read the response body
  String response = client.readString();
  client.stop();
  
  Serial.print(F("[MEMORY] Free heap after response: "));
  Serial.println(ESP.getFreeHeap());
  
  // Parse the response
  int accessTokenStart = response.indexOf("\"access_token\":\"");
  int refreshTokenStart = response.indexOf("\"refresh_token\":\"");
  
  if (accessTokenStart < 0 || refreshTokenStart < 0) {
    Serial.println(F("[NETATMO] Failed to find tokens in response"));
    Serial.println(response);
    return;
  }
  
  accessTokenStart += 16; // Length of "\"access_token\":\""
  refreshTokenStart += 17; // Length of "\"refresh_token\":\""
  
  int accessTokenEnd = response.indexOf("\"", accessTokenStart);
  int refreshTokenEnd = response.indexOf("\"", refreshTokenStart);
  
  if (accessTokenEnd < 0 || refreshTokenEnd < 0) {
    Serial.println(F("[NETATMO] Failed to parse tokens"));
    return;
  }
  
  String accessToken = response.substring(accessTokenStart, accessTokenEnd);
  String refreshToken = response.substring(refreshTokenStart, refreshTokenEnd);
  
  Serial.println(F("[NETATMO] Successfully extracted tokens"));
  
  // Save tokens to config
  saveNetatmoTokens(accessToken, refreshToken);
}

// Save Netatmo tokens to config
void saveNetatmoTokens(String accessToken, String refreshToken) {
  Serial.println(F("[NETATMO] Saving tokens to config"));
  
  if (!LittleFS.begin()) {
    Serial.println(F("[NETATMO] Failed to mount file system"));
    return;
  }
  
  // Read the current config
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println(F("[NETATMO] Failed to open config file"));
    return;
  }
  
  String configData = f.readString();
  f.close();
  
  // Update the tokens using string replacement to avoid memory issues with JSON parsing
  int accessTokenStart = configData.indexOf("\"netatmoAccessToken\":");
  int refreshTokenStart = configData.indexOf("\"netatmoRefreshToken\":");
  
  if (accessTokenStart >= 0 && refreshTokenStart >= 0) {
    // Find the end of the current values
    int accessTokenValueStart = configData.indexOf("\"", accessTokenStart + 20) + 1;
    int accessTokenValueEnd = configData.indexOf("\"", accessTokenValueStart);
    
    int refreshTokenValueStart = configData.indexOf("\"", refreshTokenStart + 21) + 1;
    int refreshTokenValueEnd = configData.indexOf("\"", refreshTokenValueStart);
    
    if (accessTokenValueStart > 0 && accessTokenValueEnd > 0 && 
        refreshTokenValueStart > 0 && refreshTokenValueEnd > 0) {
      
      // Replace the values
      String newConfig = configData.substring(0, accessTokenValueStart);
      newConfig += accessToken;
      newConfig += configData.substring(accessTokenValueEnd, refreshTokenValueStart);
      newConfig += refreshToken;
      newConfig += configData.substring(refreshTokenValueEnd);
      
      // Write the updated config
      File outFile = LittleFS.open("/config.json", "w");
      if (!outFile) {
        Serial.println(F("[NETATMO] Failed to open config file for writing"));
        return;
      }
      
      outFile.print(newConfig);
      outFile.close();
      
      Serial.println(F("[NETATMO] Tokens saved successfully"));
    } else {
      Serial.println(F("[NETATMO] Failed to locate token values in config"));
    }
  } else {
    Serial.println(F("[NETATMO] Failed to locate token fields in config"));
  }
}
