// Global variables for deferred operations
bool settingsSavePending = false;
bool apiCallInProgress = false;

// Function to process pending settings save
void processSettingsSave() {
  if (!settingsSavePending) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing pending settings save"));
  settingsSavePending = false;
  
  // Save settings to config file with yield calls
  Serial.println(F("[CONFIG] Saving settings to config.json"));
  
  yield(); // Feed the watchdog
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return;
  }
  
  yield(); // Feed the watchdog
  
  // First, read the existing config file to preserve all settings
  DynamicJsonDocument doc(2048);
  bool configExists = false;
  
  if (LittleFS.exists("/config.json")) {
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      yield(); // Feed the watchdog
      
      // Read the file in chunks to avoid memory issues
      String jsonString = "";
      while (configFile.available()) {
        jsonString += configFile.readString();
        yield(); // Feed the watchdog
      }
      configFile.close();
      
      yield(); // Feed the watchdog
      
      DeserializationError error = deserializeJson(doc, jsonString);
      
      yield(); // Feed the watchdog
      
      if (!error) {
        configExists = true;
        Serial.println(F("[CONFIG] Successfully read existing config"));
      } else {
        Serial.print(F("[CONFIG] Error parsing config file: "));
        Serial.println(error.c_str());
        // Continue with empty doc if parsing fails
      }
      
      // Clear the string to free memory
      jsonString = "";
      yield(); // Feed the watchdog
    }
  }
  
  yield(); // Feed the watchdog
  
  // Update only the Netatmo settings in the JSON document
  doc["netatmoDeviceId"] = netatmoDeviceId;
  doc["netatmoModuleId"] = netatmoModuleId;
  doc["netatmoIndoorModuleId"] = netatmoIndoorModuleId;
  doc["useNetatmoOutdoor"] = useNetatmoOutdoor;
  doc["prioritizeNetatmoIndoor"] = prioritizeNetatmoIndoor;
  
  yield(); // Feed the watchdog
  
  // Create a new config file
  File outFile = LittleFS.open("/config.json.new", "w");
  if (!outFile) {
    Serial.println(F("[CONFIG] Failed to open temp config file for writing"));
    return;
  }
  
  yield(); // Feed the watchdog
  
  // Serialize the updated JSON document to the file in chunks
  String jsonOutput;
  serializeJson(doc, jsonOutput);
  
  yield(); // Feed the watchdog
  
  // Write in chunks of 256 bytes
  const int chunkSize = 256;
  for (size_t i = 0; i < jsonOutput.length(); i += chunkSize) {
    size_t end = min(i + chunkSize, jsonOutput.length());
    outFile.print(jsonOutput.substring(i, end));
    yield(); // Feed the watchdog
  }
  
  outFile.close();
  
  yield(); // Feed the watchdog
  
  // Replace the old config file with the new one
  if (LittleFS.exists("/config.json")) {
    LittleFS.remove("/config.json");
    yield(); // Feed the watchdog
  }
  
  if (LittleFS.rename("/config.json.new", "/config.json")) {
    Serial.println(F("[CONFIG] Settings saved successfully"));
  } else {
    Serial.println(F("[CONFIG] Failed to rename config file"));
  }
  
  yield(); // Feed the watchdog
  
  Serial.println(F("[NETATMO] Settings saved successfully"));
}
