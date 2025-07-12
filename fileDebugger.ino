// Define DEBUG_FILE_DUMP to enable file dumping
// Comment this out in production to save memory
// #define DEBUG_FILE_DUMP

// Function to read and dump file contents to serial log
void dumpFileContents(const char* filePath) {
#ifdef DEBUG_FILE_DUMP
  Serial.print(F("[DEBUG] Dumping contents of file: "));
  Serial.println(filePath);
  
  if (!LittleFS.exists(filePath)) {
    Serial.println(F("[DEBUG] Error - File not found"));
    return;
  }
  
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    Serial.println(F("[DEBUG] Error - Failed to open file for reading"));
    return;
  }
  
  Serial.println(F("[DEBUG] File content start >>>"));
  
  // Read and print the file in smaller chunks to reduce memory usage
  const size_t bufSize = 32; // Reduced buffer size
  char buf[bufSize];
  size_t totalRead = 0;
  
  while (file.available()) {
    size_t bytesRead = file.readBytes(buf, bufSize - 1);
    if (bytesRead > 0) {
      buf[bytesRead] = '\0'; // Null-terminate
      Serial.print(buf);
      totalRead += bytesRead;
      yield(); // Feed the watchdog
    }
  }
  
  Serial.println();
  Serial.println(F("<<< [DEBUG] File content end"));
  Serial.print(F("[DEBUG] Total bytes read: "));
  Serial.println(totalRead);
  
  file.close();
#else
  // In production mode, just log that we're skipping the dump
  Serial.print(F("[DEBUG] File dump disabled for: "));
  Serial.println(filePath);
#endif
}
