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
  
  // Check indoor module ID if set
  if (strlen(netatmoIndoorModuleId) > 0) {
    Serial.print(F("[NETATMO] Indoor Module ID: "));
    Serial.println(netatmoIndoorModuleId);
  } else {
    Serial.println(F("[NETATMO] Indoor Module ID is not set"));
  }
  
  // Check tokens
  if (strlen(netatmoAccessToken) > 0) {
    Serial.println(F("[NETATMO] Access token is set"));
  } else {
    Serial.println(F("[NETATMO] Access token is not set (will be obtained through OAuth)"));
  }
  
  if (strlen(netatmoRefreshToken) > 0) {
    Serial.println(F("[NETATMO] Refresh token is set"));
  } else {
    Serial.println(F("[NETATMO] Refresh token is not set (will be obtained through OAuth)"));
  }
  
  // Check temperature settings
  Serial.print(F("[NETATMO] Use Netatmo for outdoor temperature: "));
  Serial.println(useNetatmoOutdoor ? "Yes" : "No");
  
  Serial.print(F("[NETATMO] Prioritize Netatmo for indoor temperature: "));
  Serial.println(prioritizeNetatmoIndoor ? "Yes" : "No");
  
  if (configOK) {
    Serial.println(F("[NETATMO] Configuration looks good!"));
  } else {
    Serial.println(F("[NETATMO] Configuration is incomplete - outdoor temperature will not be available"));
  }
  Serial.println();
}
