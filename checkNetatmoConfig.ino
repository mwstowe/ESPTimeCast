// Function to check Netatmo configuration
void checkNetatmoConfig() {
  Serial.println(F("\n[NETATMO] Checking Netatmo configuration..."));
  
  bool configOK = true;
  
  // Check client ID and secret
  if (strlen(netatmoClientId) == 0) {
    Serial.println(F("[NETATMO] ERROR: Client ID is missing"));
    configOK = false;
  } else {
    Serial.print(F("[NETATMO] Client ID: "));
    Serial.println(netatmoClientId);
  }
  
  if (strlen(netatmoClientSecret) == 0) {
    Serial.println(F("[NETATMO] ERROR: Client Secret is missing"));
    configOK = false;
  } else {
    Serial.println(F("[NETATMO] Client Secret is set"));
  }
  
  // Check username and password
  if (strlen(netatmoUsername) == 0) {
    Serial.println(F("[NETATMO] ERROR: Username is missing"));
    configOK = false;
  } else {
    Serial.print(F("[NETATMO] Username: "));
    Serial.println(netatmoUsername);
  }
  
  if (strlen(netatmoPassword) == 0) {
    Serial.println(F("[NETATMO] ERROR: Password is missing"));
    configOK = false;
  } else {
    Serial.println(F("[NETATMO] Password is set"));
  }
  
  // Check device and module IDs
  if (strlen(netatmoDeviceId) == 0) {
    Serial.println(F("[NETATMO] ERROR: Device ID is missing"));
    configOK = false;
  } else {
    Serial.print(F("[NETATMO] Device ID: "));
    Serial.println(netatmoDeviceId);
  }
  
  if (strlen(netatmoModuleId) == 0) {
    Serial.println(F("[NETATMO] ERROR: Module ID is missing"));
    configOK = false;
  } else {
    Serial.print(F("[NETATMO] Module ID: "));
    Serial.println(netatmoModuleId);
  }
  
  // Check tokens
  if (strlen(netatmoAccessToken) > 0) {
    Serial.println(F("[NETATMO] Access token is set"));
  } else {
    Serial.println(F("[NETATMO] Access token is not set (will be obtained during first request)"));
  }
  
  if (strlen(netatmoRefreshToken) > 0) {
    Serial.println(F("[NETATMO] Refresh token is set"));
  } else {
    Serial.println(F("[NETATMO] Refresh token is not set (will be obtained during first request)"));
  }
  
  if (configOK) {
    Serial.println(F("[NETATMO] Configuration looks good!"));
  } else {
    Serial.println(F("[NETATMO] Configuration is incomplete - outdoor temperature will not be available"));
  }
  Serial.println();
}
