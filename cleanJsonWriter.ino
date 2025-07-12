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

// Global flag to indicate when a chunked transfer is complete
bool chunkedTransferComplete = false;

// Function to write clean JSON to a file from a buffer
bool writeCleanJsonFromBuffer(const uint8_t* buffer, size_t bufferSize, File &file) {
  // Debug: Print the first few bytes of the buffer to help diagnose chunked encoding
  static bool firstChunk = true;
  static bool isChunkedEncoding = false;
  static bool inChunkHeader = false;
  static bool skipCRLF = false;
  static int chunkSize = 0;
  static int bytesReadInChunk = 0;
  
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
    
    // Check if this looks like a chunked response
    // Chunked responses start with a hex number followed by CR LF
    if (bufferSize >= 3) {
      char ch1 = (char)buffer[0];
      char ch2 = (char)buffer[1];
      char ch3 = (char)buffer[2];
      
      if (((ch1 >= '0' && ch1 <= '9') || (ch1 >= 'a' && ch1 <= 'f') || (ch1 >= 'A' && ch1 <= 'F')) &&
          ((ch2 >= '0' && ch2 <= '9') || (ch2 >= 'a' && ch2 <= 'f') || (ch2 >= 'A' && ch2 <= 'F')) &&
          ((ch3 >= '0' && ch3 <= '9') || (ch3 >= 'a' && ch3 <= 'f') || (ch3 >= 'A' && ch3 <= 'F'))) {
        isChunkedEncoding = true;
        inChunkHeader = true;
        chunkSize = 0;
        bytesReadInChunk = 0;
        chunkedTransferComplete = false; // Reset the flag
        Serial.println(F("[JSON] Detected chunked encoding in first chunk"));
      }
    }
    
    firstChunk = false;
  }

  // Process the buffer
  for (size_t i = 0; i < bufferSize; i++) {
    char ch = (char)buffer[i];
    
    // Handle chunked encoding
    if (isChunkedEncoding) {
      // Skip chunk headers and chunk boundaries
      if (inChunkHeader) {
        // Parse the chunk size
        if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
          // Convert hex digit to integer
          if (ch >= '0' && ch <= '9') {
            chunkSize = (chunkSize << 4) | (ch - '0');
          } else if (ch >= 'a' && ch <= 'f') {
            chunkSize = (chunkSize << 4) | (ch - 'a' + 10);
          } else { // ch >= 'A' && ch <= 'F'
            chunkSize = (chunkSize << 4) | (ch - 'A' + 10);
          }
        } else if (ch == '\r') {
          skipCRLF = true;
        } else if (ch == '\n' && skipCRLF) {
          inChunkHeader = false;
          skipCRLF = false;
          
          // If chunk size is 0, we're done
          if (chunkSize == 0) {
            Serial.println(F("[JSON] Found end chunk (size 0)"));
            chunkedTransferComplete = true; // Set the flag
            return true; // Return true to indicate transfer is complete
          }
          
          Serial.print(F("[JSON] Chunk size: "));
          Serial.println(chunkSize);
        }
        continue;
      }
      
      // Check if we've read the entire chunk
      if (bytesReadInChunk >= chunkSize) {
        // We've read the entire chunk, look for the CRLF
        if (ch == '\r') {
          skipCRLF = true;
          continue;
        } else if (ch == '\n' && skipCRLF) {
          // Start of a new chunk
          inChunkHeader = true;
          chunkSize = 0;
          bytesReadInChunk = 0;
          skipCRLF = false;
          continue;
        }
      }
      
      // Count bytes read in the current chunk
      bytesReadInChunk++;
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
  
  return false; // Return false to indicate transfer is not complete
}
