#include "saveSettingsState.h"

// Global variables for deferred operations
bool settingsSavePending = false;
bool apiCallInProgress = false;

// Global variables for state management
SaveSettingsState saveSettingsState = IDLE;
DynamicJsonDocument* settingsDoc = nullptr;
String jsonOutput;
File configFile;
unsigned long lastSaveStep = 0;
const unsigned long SAVE_STEP_INTERVAL = 50; // ms between steps

// Function to process pending settings save
void processSettingsSave() {
  if (!settingsSavePending && saveSettingsState == IDLE) {
    return;
  }
  
  // Initialize the save operation if it's just starting
  if (settingsSavePending && saveSettingsState == IDLE) {
    Serial.println(F("[NETATMO] Starting settings save process"));
    settingsSavePending = false;
    saveSettingsState = MOUNT_FS;
    lastSaveStep = millis();
    return;
  }
  
  // Check if enough time has passed since the last step
  if (millis() - lastSaveStep < SAVE_STEP_INTERVAL) {
    return;
  }
  
  lastSaveStep = millis();
  
  // Process the current state
  switch (saveSettingsState) {
    case MOUNT_FS:
      Serial.println(F("[CONFIG] Mounting filesystem"));
      if (!LittleFS.begin()) {
        Serial.println(F("[CONFIG] Failed to mount file system"));
        saveSettingsState = IDLE;
        return;
      }
      saveSettingsState = READ_CONFIG;
      break;
      
    case READ_CONFIG:
      Serial.println(F("[CONFIG] Reading existing config"));
      
      // Allocate memory for the document
      if (settingsDoc == nullptr) {
        settingsDoc = new DynamicJsonDocument(2048);
        if (settingsDoc == nullptr) {
          Serial.println(F("[CONFIG] Failed to allocate memory for JSON document"));
          saveSettingsState = IDLE;
          return;
        }
      }
      
      // Read existing config if it exists
      if (LittleFS.exists("/config.json")) {
        configFile = LittleFS.open("/config.json", "r");
        if (configFile) {
          DeserializationError error = deserializeJson(*settingsDoc, configFile);
          configFile.close();
          
          if (error) {
            Serial.print(F("[CONFIG] Error parsing config file: "));
            Serial.println(error.c_str());
            // Continue with empty doc if parsing fails
          } else {
            Serial.println(F("[CONFIG] Successfully read existing config"));
          }
        }
      }
      
      saveSettingsState = UPDATE_CONFIG;
      break;
      
    case UPDATE_CONFIG:
      Serial.println(F("[CONFIG] Updating config with new settings"));
      
      // Update only the Netatmo settings in the JSON document
      (*settingsDoc)["netatmoDeviceId"] = netatmoDeviceId;
      (*settingsDoc)["netatmoModuleId"] = netatmoModuleId;
      (*settingsDoc)["netatmoIndoorModuleId"] = netatmoIndoorModuleId;
      (*settingsDoc)["useNetatmoOutdoor"] = useNetatmoOutdoor;
      (*settingsDoc)["prioritizeNetatmoIndoor"] = prioritizeNetatmoIndoor;
      
      saveSettingsState = WRITE_CONFIG;
      break;
      
    case WRITE_CONFIG:
      Serial.println(F("[CONFIG] Writing config to file"));
      
      // Create a new config file
      configFile = LittleFS.open("/config.json.new", "w");
      if (!configFile) {
        Serial.println(F("[CONFIG] Failed to open temp config file for writing"));
        saveSettingsState = IDLE;
        if (settingsDoc != nullptr) {
          delete settingsDoc;
          settingsDoc = nullptr;
        }
        return;
      }
      
      // Write the JSON document to the file
      if (serializeJson(*settingsDoc, configFile) == 0) {
        Serial.println(F("[CONFIG] Failed to write to config file"));
        configFile.close();
        saveSettingsState = IDLE;
        if (settingsDoc != nullptr) {
          delete settingsDoc;
          settingsDoc = nullptr;
        }
        return;
      }
      
      configFile.close();
      saveSettingsState = FINALIZE;
      break;
      
    case FINALIZE:
      Serial.println(F("[CONFIG] Finalizing config save"));
      
      // Replace the old config file with the new one
      if (LittleFS.exists("/config.json")) {
        LittleFS.remove("/config.json");
      }
      
      if (LittleFS.rename("/config.json.new", "/config.json")) {
        Serial.println(F("[CONFIG] Settings saved successfully"));
      } else {
        Serial.println(F("[CONFIG] Failed to rename config file"));
      }
      
      // Clean up
      if (settingsDoc != nullptr) {
        delete settingsDoc;
        settingsDoc = nullptr;
      }
      
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
