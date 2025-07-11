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
  static bool inChunkHeader = false;
  static bool skipCRLF = false;
  
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
    
    // Check if this looks like a chunked response
    // Chunked responses start with a hex number followed by CR LF
    if (bufferSize >= 3) {
      char ch1 = (char)buffer[0];
      char ch2 = (char)buffer[1];
      char ch3 = (char)buffer[2];
      
      if (((ch1 >= '0' && ch1 <= '9') || (ch1 >= 'a' && ch1 <= 'f') || (ch1 >= 'A' && ch1 <= 'F')) &&
          ((ch2 >= '0' && ch2 <= '9') || (ch2 >= 'a' && ch2 <= 'f') || (ch2 >= 'A' && ch2 <= 'F')) &&
          (ch3 == '\r' || ch3 == '\n')) {
        Serial.println(F("[JSON] Detected chunked encoding in buffer"));
        inChunkHeader = true;
      }
    }
  }

  // Process the buffer
  for (size_t i = 0; i < bufferSize; i++) {
    char ch = (char)buffer[i];
    
    // Handle chunked encoding
    if (inChunkHeader) {
      // Look for the end of the chunk header (CR LF)
      if (ch == '\r' || ch == '\n') {
        if (ch == '\r') {
          skipCRLF = true; // Skip the LF that follows
        } else {
          inChunkHeader = false; // End of chunk header
        }
      }
      // Skip the chunk header
      continue;
    }
    
    if (skipCRLF) {
      if (ch == '\n') {
        skipCRLF = false;
        inChunkHeader = false;
      }
      continue;
    }
    
    // Check for the start of a new chunk
    // Chunks are separated by CR LF, then the chunk size, then CR LF
    if (ch == '\r' || ch == '\n') {
      // This might be the end of a chunk
      // We'll check the next character in the next iteration
      if (ch == '\r') {
        skipCRLF = true;
      }
      continue;
    }
    
    // Only write valid JSON characters to the file
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
