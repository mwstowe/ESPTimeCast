// Configuration backup and recovery functions

// Function to backup the current configuration
bool backupConfig() {
  Serial.println(F("[CONFIG] Backing up configuration"));
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return false;
  }
  
  // Check if config.json exists
  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] No configuration file to backup"));
    return false;
  }
  
  // Read the current config
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for reading"));
    return false;
  }
  
  String configData = configFile.readString();
  configFile.close();
  
  // Write to backup file
  File backupFile = LittleFS.open("/config.bak", "w");
  if (!backupFile) {
    Serial.println(F("[CONFIG] Failed to open backup file for writing"));
    return false;
  }
  
  size_t bytesWritten = backupFile.print(configData);
  backupFile.close();
  
  if (bytesWritten == 0) {
    Serial.println(F("[CONFIG] Failed to write backup file"));
    return false;
  }
  
  Serial.println(F("[CONFIG] Configuration backup successful"));
  return true;
}

// Function to restore configuration from backup
bool restoreConfig() {
  Serial.println(F("[CONFIG] Restoring configuration from backup"));
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return false;
  }
  
  // Check if backup file exists
  if (!LittleFS.exists("/config.bak")) {
    Serial.println(F("[CONFIG] No backup file found"));
    return false;
  }
  
  // Read the backup file
  File backupFile = LittleFS.open("/config.bak", "r");
  if (!backupFile) {
    Serial.println(F("[CONFIG] Failed to open backup file for reading"));
    return false;
  }
  
  String backupData = backupFile.readString();
  backupFile.close();
  
  // Validate the backup data
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, backupData);
  
  if (err) {
    Serial.print(F("[CONFIG] Backup file is corrupted: "));
    Serial.println(err.c_str());
    return false;
  }
  
  // Write to config file
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for writing"));
    return false;
  }
  
  size_t bytesWritten = configFile.print(backupData);
  configFile.close();
  
  if (bytesWritten == 0) {
    Serial.println(F("[CONFIG] Failed to write config file"));
    return false;
  }
  
  Serial.println(F("[CONFIG] Configuration restored successfully"));
  return true;
}

// Function to check and repair configuration
bool checkAndRepairConfig() {
  Serial.println(F("[CONFIG] Checking configuration integrity"));
  
  yield(); // Feed the watchdog
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return false;
  }
  
  yield(); // Feed the watchdog
  
  // Check if config.json exists
  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] Configuration file not found"));
    
    yield(); // Feed the watchdog
    
    // Try to restore from backup
    if (LittleFS.exists("/config.bak")) {
      Serial.println(F("[CONFIG] Attempting to restore from backup"));
      if (restoreConfig()) {
        Serial.println(F("[CONFIG] Configuration restored from backup"));
        return true;
      }
    }
    
    yield(); // Feed the watchdog
    
    // Create default config if restore failed
    Serial.println(F("[CONFIG] Creating default configuration"));
    createDefaultConfig();
    return LittleFS.exists("/config.json");
  }
  
  yield(); // Feed the watchdog
  
  // Validate the config file
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file for reading"));
    return false;
  }
  
  yield(); // Feed the watchdog
  
  size_t fileSize = configFile.size();
  if (fileSize == 0) {
    configFile.close();
    Serial.println(F("[CONFIG] Configuration file is empty"));
    
    yield(); // Feed the watchdog
    
    // Try to restore from backup
    if (restoreConfig()) {
      Serial.println(F("[CONFIG] Configuration restored from backup"));
      return true;
    }
    
    yield(); // Feed the watchdog
    
    // Create default config if restore failed
    Serial.println(F("[CONFIG] Creating default configuration"));
    createDefaultConfig();
    return LittleFS.exists("/config.json");
  }
  
  yield(); // Feed the watchdog
  
  // Read the file content
  String configData = configFile.readString();
  configFile.close();
  
  yield(); // Feed the watchdog
  
  // Validate JSON
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, configData);
  
  yield(); // Feed the watchdog
  
  if (err) {
    Serial.print(F("[CONFIG] Configuration file is corrupted: "));
    Serial.println(err.c_str());
    
    yield(); // Feed the watchdog
    
    // Try to restore from backup
    if (restoreConfig()) {
      Serial.println(F("[CONFIG] Configuration restored from backup"));
      return true;
    }
    
    yield(); // Feed the watchdog
    
    // Create default config if restore failed
    Serial.println(F("[CONFIG] Creating default configuration"));
    createDefaultConfig();
    return LittleFS.exists("/config.json");
  }
  
  yield(); // Feed the watchdog
  
  // Configuration is valid, create a backup if one doesn't exist
  if (!LittleFS.exists("/config.bak")) {
    Serial.println(F("[CONFIG] Creating backup of valid configuration"));
    backupConfig();
  }
  
  yield(); // Feed the watchdog
  
  Serial.println(F("[CONFIG] Configuration is valid"));
  return true;
}
