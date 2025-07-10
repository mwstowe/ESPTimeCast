// Function to write clean JSON to a file
void writeCleanJsonToFile(const String &jsonString, const char* filePath) {
  // Open the file for writing
  File file = LittleFS.open(filePath, "w");
  if (!file) {
    Serial.print(F("[JSON] Failed to open file for writing: "));
    Serial.println(filePath);
    return;
  }
  
  // Write only valid JSON characters to the file
  for (size_t i = 0; i < jsonString.length(); i++) {
    char ch = jsonString.charAt(i);
    // Only write valid JSON characters
    if (ch == '{' || ch == '}' || ch == '[' || ch == ']' || 
        ch == ',' || ch == ':' || ch == '"' || ch == '\'' || 
        ch == '.' || ch == '-' || ch == '+' || 
        (ch >= '0' && ch <= '9') || 
        (ch >= 'a' && ch <= 'z') || 
        (ch >= 'A' && ch <= 'Z') || 
        ch == '_' || ch == ' ' || ch == '\t' || 
        ch == '\r' || ch == '\n') {
      file.write(ch);
    }
  }
  
  // Close the file
  file.close();
  
  Serial.print(F("[JSON] Clean JSON written to file: "));
  Serial.println(filePath);
}

// Function to write clean JSON to a file from a buffer
void writeCleanJsonFromBuffer(const uint8_t* buffer, size_t bufferSize, File &file) {
  // Write only valid JSON characters to the file
  for (size_t i = 0; i < bufferSize; i++) {
    char ch = (char)buffer[i];
    // Only write valid JSON characters
    if (ch == '{' || ch == '}' || ch == '[' || ch == ']' || 
        ch == ',' || ch == ':' || ch == '"' || ch == '\'' || 
        ch == '.' || ch == '-' || ch == '+' || 
        (ch >= '0' && ch <= '9') || 
        (ch >= 'a' && ch <= 'z') || 
        (ch >= 'A' && ch <= 'Z') || 
        ch == '_' || ch == ' ' || ch == '\t' || 
        ch == '\r' || ch == '\n') {
      file.write(ch);
    }
  }
}
