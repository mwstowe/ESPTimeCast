// Function to check if the config file is valid and repair it if needed
bool checkAndRepairConfig() {
  Serial.println(F("[CONFIG] Checking config file integrity"));
  
  if (!LittleFS.begin()) {
    Serial.println(F("[CONFIG] Failed to mount file system"));
    return false;
  }
  
  // Check if config file exists
  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] Config file does not exist"));
    return false;
  }
  
  // Try to parse the config file
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config file"));
    return false;
  }
  
  // Check file size
  size_t fileSize = configFile.size();
  Serial.print(F("[CONFIG] Config file size: "));
  Serial.println(fileSize);
  
  if (fileSize == 0) {
    Serial.println(F("[CONFIG] Config file is empty"));
    configFile.close();
    
    // Try to restore from backup
    if (LittleFS.exists("/config.json.bak")) {
      Serial.println(F("[CONFIG] Attempting to restore from backup"));
      LittleFS.remove("/config.json");
      if (LittleFS.rename("/config.json.bak", "/config.json")) {
        Serial.println(F("[CONFIG] Restored from backup"));
        return true;
      } else {
        Serial.println(F("[CONFIG] Failed to restore from backup"));
        return false;
      }
    }
    
    return false;
  }
  
  // Try to parse the JSON
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.print(F("[CONFIG] JSON parse error: "));
    Serial.println(error.c_str());
    
    // Try to restore from backup
    if (LittleFS.exists("/config.json.bak")) {
      Serial.println(F("[CONFIG] Attempting to restore from backup"));
      LittleFS.remove("/config.json");
      if (LittleFS.rename("/config.json.bak", "/config.json")) {
        Serial.println(F("[CONFIG] Restored from backup"));
        return true;
      } else {
        Serial.println(F("[CONFIG] Failed to restore from backup"));
        return false;
      }
    }
    
    return false;
  }
  
  // Check if the config has essential fields
  bool hasEssentialFields = true;
  
  // Check for essential fields
  if (!doc.containsKey("weatherUnits")) {
    Serial.println(F("[CONFIG] Missing weatherUnits"));
    hasEssentialFields = false;
    doc["weatherUnits"] = "metric"; // Set default
  }
  
  if (!doc.containsKey("brightness")) {
    Serial.println(F("[CONFIG] Missing brightness"));
    hasEssentialFields = false;
    doc["brightness"] = 7; // Set default
  }
  
  if (!doc.containsKey("tempAdjust")) {
    Serial.println(F("[CONFIG] Missing tempAdjust"));
    hasEssentialFields = false;
    doc["tempAdjust"] = 0; // Set default
  }
  
  // If missing essential fields, save the repaired config
  if (!hasEssentialFields) {
    Serial.println(F("[CONFIG] Repairing config file"));
    
    // Create a new config file
    File outFile = LittleFS.open("/config.json.new", "w");
    if (!outFile) {
      Serial.println(F("[CONFIG] Failed to open temp config file for writing"));
      return false;
    }
    
    // Serialize the updated JSON document to the file
    if (serializeJson(doc, outFile) == 0) {
      Serial.println(F("[CONFIG] Failed to write to config file"));
      outFile.close();
      return false;
    }
    
    outFile.close();
    
    // Replace the old config file with the new one
    if (LittleFS.exists("/config.json")) {
      LittleFS.remove("/config.json");
    }
    
    if (LittleFS.rename("/config.json.new", "/config.json")) {
      Serial.println(F("[CONFIG] Config file repaired successfully"));
      return true;
    } else {
      Serial.println(F("[CONFIG] Failed to rename config file"));
      return false;
    }
  }
  
  Serial.println(F("[CONFIG] Config file is valid"));
  return true;
}
