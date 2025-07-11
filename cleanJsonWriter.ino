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
  // Debug: Print the first few bytes of the buffer to help diagnose chunked encoding
  static bool firstChunk = true;
  if (firstChunk && bufferSize > 0) {
    Serial.print(F("[JSON] First bytes of buffer: "));
    for (size_t i = 0; i < min(bufferSize, (size_t)16); i++) {
      char ch = (char)buffer[i];
      if (ch >= 32 && ch <= 126) { // Printable ASCII
        Serial.print(ch);
      } else {
        Serial.print(F("\\x"));
        Serial.print(buffer[i] < 16 ? "0" : "");
        Serial.print(buffer[i], HEX);
      }
    }
    Serial.println();
    firstChunk = false;
  }

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
