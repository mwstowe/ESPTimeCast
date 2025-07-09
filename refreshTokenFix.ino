// Function to diagnose and fix token refresh issues
void diagnoseTokenRefreshIssue() {
  Serial.println(F("[NETATMO] Diagnosing token refresh issues"));
  
  // Check WiFi connection
  Serial.print(F("[NETATMO] WiFi status: "));
  switch (WiFi.status()) {
    case WL_CONNECTED:
      Serial.println(F("Connected"));
      Serial.print(F("[NETATMO] IP address: "));
      Serial.println(WiFi.localIP());
      Serial.print(F("[NETATMO] Signal strength: "));
      Serial.print(WiFi.RSSI());
      Serial.println(F(" dBm"));
      break;
    case WL_DISCONNECTED:
      Serial.println(F("Disconnected"));
      break;
    case WL_IDLE_STATUS:
      Serial.println(F("Idle"));
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println(F("No SSID available"));
      break;
    case WL_CONNECT_FAILED:
      Serial.println(F("Connection failed"));
      break;
    default:
      Serial.print(F("Unknown ("));
      Serial.print(WiFi.status());
      Serial.println(F(")"));
      break;
  }
  
  // Test DNS resolution
  Serial.println(F("[NETATMO] Testing DNS resolution for api.netatmo.com"));
  IPAddress ip;
  bool dnsSuccess = WiFi.hostByName("api.netatmo.com", ip);
  if (dnsSuccess) {
    Serial.print(F("[NETATMO] DNS resolution successful: "));
    Serial.println(ip.toString());
  } else {
    Serial.println(F("[NETATMO] DNS resolution failed"));
  }
  
  // Check token format
  Serial.println(F("[NETATMO] Checking token format"));
  Serial.print(F("[NETATMO] Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  
  // Check for pipe character
  bool hasPipe = false;
  int pipePosition = -1;
  for (size_t i = 0; i < strlen(netatmoAccessToken); i++) {
    if (netatmoAccessToken[i] == '|') {
      hasPipe = true;
      pipePosition = i;
      break;
    }
  }
  
  if (hasPipe) {
    Serial.print(F("[NETATMO] Pipe character found at position: "));
    Serial.println(pipePosition);
    Serial.println(F("[NETATMO] Token format appears correct"));
  } else {
    Serial.println(F("[NETATMO] WARNING: Token does not contain a pipe character!"));
    Serial.println(F("[NETATMO] This is likely the cause of the authentication failure"));
  }
  
  // Print the first and second parts of the token if pipe is found
  if (hasPipe) {
    Serial.println(F("[NETATMO] Token parts:"));
    Serial.print(F("[NETATMO] Part 1 (before pipe): "));
    for (int i = 0; i < pipePosition; i++) {
      Serial.print(netatmoAccessToken[i]);
    }
    Serial.println();
    
    Serial.print(F("[NETATMO] Part 2 (after pipe): "));
    for (size_t i = pipePosition + 1; i < strlen(netatmoAccessToken); i++) {
      Serial.print(netatmoAccessToken[i]);
    }
    Serial.println();
  }
  
  // Check refresh token format
  Serial.println(F("[NETATMO] Checking refresh token format"));
  Serial.print(F("[NETATMO] Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
  
  // Check for pipe character in refresh token
  bool refreshHasPipe = false;
  int refreshPipePosition = -1;
  for (size_t i = 0; i < strlen(netatmoRefreshToken); i++) {
    if (netatmoRefreshToken[i] == '|') {
      refreshHasPipe = true;
      refreshPipePosition = i;
      break;
    }
  }
  
  if (refreshHasPipe) {
    Serial.print(F("[NETATMO] Refresh token pipe character found at position: "));
    Serial.println(refreshPipePosition);
    Serial.println(F("[NETATMO] Refresh token format appears correct"));
  } else {
    Serial.println(F("[NETATMO] WARNING: Refresh token does not contain a pipe character!"));
    Serial.println(F("[NETATMO] This is likely the cause of the token refresh failure"));
  }
}
