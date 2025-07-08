// Function to save tokens to config.json
void saveTokensToConfig() {
  Serial.println(F("[CONFIG] Saving tokens to config.json"));
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return;
  }
  
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for reading"));
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.print(F("[CONFIG] Failed to parse config file: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Update the tokens
  doc["netatmoAccessToken"] = netatmoAccessToken;
  doc["netatmoRefreshToken"] = netatmoRefreshToken;
  doc["netatmoDeviceId"] = netatmoDeviceId;
  doc["netatmoModuleId"] = netatmoModuleId;
  doc["netatmoIndoorModuleId"] = netatmoIndoorModuleId;
  doc["useNetatmoOutdoor"] = useNetatmoOutdoor;
  doc["prioritizeNetatmoIndoor"] = prioritizeNetatmoIndoor;
  
  // Save the updated config
  configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for writing"));
    return;
  }
  
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("[CONFIG] Failed to write to config file"));
  } else {
    Serial.println(F("[CONFIG] Tokens saved successfully"));
  }
  
  configFile.close();
}
