// Function to list all files in the LittleFS filesystem
void listAllFiles() {
  Serial.println(F("[FS] Listing all files in LittleFS:"));
  
  // List files in root directory
  Serial.println(F("[FS] Root directory:"));
  Dir rootDir = LittleFS.openDir("/");
  while (rootDir.next()) {
    Serial.print(F("  "));
    Serial.print(rootDir.fileName());
    Serial.print(F(" - "));
    Serial.println(rootDir.fileSize());
  }
  
  // Check if devices directory exists
  Serial.print(F("[FS] Devices directory exists: "));
  Serial.println(LittleFS.exists("/devices") ? F("Yes") : F("No"));
  
  // If devices directory exists, list its contents
  if (LittleFS.exists("/devices")) {
    Serial.println(F("[FS] Files in /devices:"));
    Dir devDir = LittleFS.openDir("/devices");
    while (devDir.next()) {
      Serial.print(F("  "));
      Serial.print(devDir.fileName());
      Serial.print(F(" - "));
      Serial.println(devDir.fileSize());
    }
  }
}
