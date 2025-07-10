#include "saveSettingsState.h"

// Global variables for deferred operations
bool settingsSavePending = false;
bool apiCallInProgress = false;

// Global variables for state management
SaveSettingsState saveSettingsState = IDLE;
File settingsFile;

// Function to process pending settings save
void processSettingsSave() {
  if (!settingsSavePending && saveSettingsState == IDLE) {
    return;
  }
  
  // Initialize the save operation if it's just starting
  if (settingsSavePending && saveSettingsState == IDLE) {
    Serial.println(F("[NETATMO] Starting settings save process"));
    settingsSavePending = false;
    saveSettingsState = PREPARE;
    return;
  }
  
  // Process the current state
  switch (saveSettingsState) {
    case PREPARE:
      Serial.println(F("[CONFIG] Preparing to save settings"));
      
      // Mount filesystem
      if (!LittleFS.begin()) {
        Serial.println(F("[CONFIG] Failed to mount file system"));
        saveSettingsState = IDLE;
        return;
      }
      
      // Create a new settings file
      settingsFile = LittleFS.open("/netatmo_settings.txt", "w");
      if (!settingsFile) {
        Serial.println(F("[CONFIG] Failed to open settings file for writing"));
        saveSettingsState = IDLE;
        return;
      }
      
      saveSettingsState = WRITE_SETTINGS;
      break;
      
    case WRITE_SETTINGS:
      Serial.println(F("[CONFIG] Writing settings to file"));
      
      // Write settings in a simple key=value format
      settingsFile.println(String("netatmoDeviceId=") + netatmoDeviceId);
      yield();
      
      settingsFile.println(String("netatmoModuleId=") + netatmoModuleId);
      yield();
      
      settingsFile.println(String("netatmoIndoorModuleId=") + netatmoIndoorModuleId);
      yield();
      
      settingsFile.println(String("useNetatmoOutdoor=") + (useNetatmoOutdoor ? "1" : "0"));
      yield();
      
      settingsFile.println(String("prioritizeNetatmoIndoor=") + (prioritizeNetatmoIndoor ? "1" : "0"));
      yield();
      
      settingsFile.close();
      saveSettingsState = FINALIZE;
      break;
      
    case FINALIZE:
      Serial.println(F("[CONFIG] Finalizing settings save"));
      
      // Update the main config.json file with a flag indicating settings are in the separate file
      updateConfigWithSettingsFlag();
      
      saveSettingsState = IDLE;
      Serial.println(F("[NETATMO] Settings save process complete"));
      break;
      
    default:
      saveSettingsState = IDLE;
      break;
  }
  
  // Always yield after each step
  yield();
}

// Function to update the main config file with a flag indicating settings are in a separate file
void updateConfigWithSettingsFlag() {
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return;
  }
  
  yield();
  
  // Create a minimal JSON document just to update the flag
  DynamicJsonDocument doc(256);
  
  // Read existing config if it exists
  if (LittleFS.exists("/config.json")) {
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      DeserializationError error = deserializeJson(doc, configFile);
      configFile.close();
      
      yield();
      
      if (error) {
        Serial.print(F("[CONFIG] Error parsing config file: "));
        Serial.println(error.c_str());
        // Continue with empty doc if parsing fails
      }
    }
  }
  
  yield();
  
  // Add or update the flag
  doc["useNetatmoSettingsFile"] = true;
  
  yield();
  
  // Write the updated config
  File outFile = LittleFS.open("/config.json.new", "w");
  if (!outFile) {
    Serial.println(F("[CONFIG] Failed to open temp config file for writing"));
    return;
  }
  
  yield();
  
  if (serializeJson(doc, outFile) == 0) {
    Serial.println(F("[CONFIG] Failed to write to config file"));
    outFile.close();
    return;
  }
  
  outFile.close();
  
  yield();
  
  // Replace the old config file with the new one
  if (LittleFS.exists("/config.json")) {
    LittleFS.remove("/config.json");
    yield();
  }
  
  if (LittleFS.rename("/config.json.new", "/config.json")) {
    Serial.println(F("[CONFIG] Config updated with settings flag"));
  } else {
    Serial.println(F("[CONFIG] Failed to rename config file"));
  }
  
  yield();
}

// Function to load settings from the separate file
void loadNetatmoSettings() {
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return;
  }
  
  yield();
  
  if (!LittleFS.exists("/netatmo_settings.txt")) {
    Serial.println(F("[CONFIG] Netatmo settings file not found"));
    return;
  }
  
  File settingsFile = LittleFS.open("/netatmo_settings.txt", "r");
  if (!settingsFile) {
    Serial.println(F("[CONFIG] Failed to open settings file for reading"));
    return;
  }
  
  yield();
  
  // Read settings line by line
  while (settingsFile.available()) {
    String line = settingsFile.readStringUntil('\n');
    line.trim();
    
    int separatorPos = line.indexOf('=');
    if (separatorPos > 0) {
      String key = line.substring(0, separatorPos);
      String value = line.substring(separatorPos + 1);
      
      if (key == "netatmoDeviceId") {
        strlcpy(netatmoDeviceId, value.c_str(), sizeof(netatmoDeviceId));
      }
      else if (key == "netatmoModuleId") {
        strlcpy(netatmoModuleId, value.c_str(), sizeof(netatmoModuleId));
      }
      else if (key == "netatmoIndoorModuleId") {
        strlcpy(netatmoIndoorModuleId, value.c_str(), sizeof(netatmoIndoorModuleId));
      }
      else if (key == "useNetatmoOutdoor") {
        useNetatmoOutdoor = (value == "1");
      }
      else if (key == "prioritizeNetatmoIndoor") {
        prioritizeNetatmoIndoor = (value == "1");
      }
    }
    
    yield();
  }
  
  settingsFile.close();
  Serial.println(F("[CONFIG] Netatmo settings loaded from file"));
}
