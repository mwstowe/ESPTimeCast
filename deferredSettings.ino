#include "saveSettingsState.h"

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
  
  // Debug: Print the values being saved
  Serial.println(F("[CONFIG] Values being saved to config.json:"));
  Serial.print(F("[CONFIG] netatmoDeviceId: '"));
  Serial.print(netatmoDeviceId);
  Serial.println(F("'"));
  Serial.print(F("[CONFIG] netatmoModuleId: '"));
  Serial.print(netatmoModuleId);
  Serial.println(F("'"));
  Serial.print(F("[CONFIG] netatmoIndoorModuleId: '"));
  Serial.print(netatmoIndoorModuleId);
  Serial.println(F("'"));
  
  // Save to config file with yield calls
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
      DeserializationError error = deserializeJson(doc, configFile);
      configFile.close();
      
      yield(); // Feed the watchdog
      
      if (!error) {
        configExists = true;
        Serial.println(F("[CONFIG] Successfully read existing config"));
      } else {
        Serial.print(F("[CONFIG] Error parsing config file: "));
        Serial.println(error.c_str());
        // Continue with empty doc if parsing fails
      }
    }
  }
  
  yield(); // Feed the watchdog
  
  // Update only the Netatmo settings in the JSON document
  doc["netatmoDeviceId"] = netatmoDeviceId;
  doc["netatmoModuleId"] = netatmoModuleId;
  doc["netatmoIndoorModuleId"] = netatmoIndoorModuleId;
  doc["useNetatmoOutdoor"] = useNetatmoOutdoor;
  doc["prioritizeNetatmoIndoor"] = prioritizeNetatmoIndoor;
  
  // Debug output for Netatmo settings being saved
  Serial.println(F("[CONFIG] Saving Netatmo settings:"));
  Serial.print(F("[CONFIG] Device ID: '"));
  Serial.print(netatmoDeviceId);
  Serial.println(F("'"));
  Serial.print(F("[CONFIG] Module ID: '"));
  Serial.print(netatmoModuleId);
  Serial.println(F("'"));
  Serial.print(F("[CONFIG] Indoor Module ID: '"));
  Serial.print(netatmoIndoorModuleId);
  Serial.println(F("'"));
  
  yield(); // Feed the watchdog
  
  // Create a new config file
  File outFile = LittleFS.open("/config.json.new", "w");
  if (!outFile) {
    Serial.println(F("[CONFIG] Failed to open temp config file for writing"));
    return;
  }
  
  yield(); // Feed the watchdog
  
  // Serialize the updated JSON document to the file
  size_t bytesWritten = serializeJson(doc, outFile);
  if (bytesWritten == 0) {
    Serial.println(F("[CONFIG] Failed to write to config file"));
    outFile.close();
    return;
  }
  
  Serial.print(F("[CONFIG] Wrote "));
  Serial.print(bytesWritten);
  Serial.println(F(" bytes to config.json.new"));
  
  outFile.close();
  
  yield(); // Feed the watchdog
  
  // Replace the old config file with the new one
  if (LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] Removing old config.json file"));
    if (LittleFS.remove("/config.json")) {
      Serial.println(F("[CONFIG] Old config.json removed successfully"));
    } else {
      Serial.println(F("[CONFIG] Failed to remove old config.json"));
      return;
    }
  } else {
    Serial.println(F("[CONFIG] No existing config.json to remove"));
  }
  
  yield(); // Feed the watchdog
  
  Serial.println(F("[CONFIG] Renaming config.json.new to config.json"));
  if (LittleFS.rename("/config.json.new", "/config.json")) {
    Serial.println(F("[CONFIG] Settings saved successfully"));
    
    // Verify the file was saved correctly
    File verifyFile = LittleFS.open("/config.json", "r");
    if (verifyFile) {
      Serial.print(F("[CONFIG] Verification: config.json size is "));
      Serial.print(verifyFile.size());
      Serial.println(F(" bytes"));
      
      // Read and print the first 100 bytes
      if (verifyFile.size() > 0) {
        char buffer[101];
        size_t bytesRead = verifyFile.readBytes(buffer, 100);
        buffer[bytesRead] = '\0';
        Serial.println(F("[CONFIG] First 100 bytes of saved config:"));
        Serial.println(buffer);
      }
      
      verifyFile.close();
    } else {
      Serial.println(F("[CONFIG] Failed to open config.json for verification"));
    }
  } else {
    Serial.println(F("[CONFIG] Failed to rename config file"));
  }
  
  yield(); // Feed the watchdog
  
  Serial.println(F("[NETATMO] Settings saved successfully"));
}
