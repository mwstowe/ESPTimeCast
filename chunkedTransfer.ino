// Functions for handling chunked transfer encoding

// Function to check if a response is chunked
bool isChunkedResponse(HTTPClient& http) {
  // Check if the Transfer-Encoding header contains "chunked"
  String transferEncoding = http.header("Transfer-Encoding");
  return transferEncoding.indexOf("chunked") >= 0;
}

// Function to handle chunked transfer encoding
bool writeChunkedResponseToFile(WiFiClient* stream, File& file) {
  Serial.println(F("[CHUNKED] Starting chunked response processing"));
  
  const size_t bufSize = 64; // Small buffer for reading chunk headers
  char buf[bufSize];
  int totalBytes = 0;
  unsigned long startTime = millis();
  unsigned long lastDataTime = millis();
  const unsigned long timeout = 5000; // 5 second timeout
  
  // State machine for chunked encoding
  enum ChunkState { CHUNK_SIZE, CHUNK_DATA, CHUNK_END, FINISHED };
  ChunkState state = CHUNK_SIZE;
  int chunkSize = 0;
  int bytesRead = 0;
  
  while (state != FINISHED) {
    // Check for timeout
    if (millis() - lastDataTime > timeout) {
      Serial.println(F("[CHUNKED] Timeout waiting for data"));
      return false;
    }
    
    // Check if data is available
    if (!stream->available()) {
      delay(10);
      yield();
      continue;
    }
    
    // Reset timeout timer when data is available
    lastDataTime = millis();
    
    switch (state) {
      case CHUNK_SIZE: {
        // Read the chunk size line
        int i = 0;
        char c;
        while (i < bufSize - 1 && stream->available()) {
          c = stream->read();
          if (c == '\r') {
            // Skip the \r
            continue;
          }
          if (c == '\n') {
            // End of chunk size line
            buf[i] = '\0';
            break;
          }
          buf[i++] = c;
        }
        
        // Parse the chunk size (hex)
        chunkSize = strtol(buf, NULL, 16);
        Serial.print(F("[CHUNKED] Chunk size: 0x"));
        Serial.print(buf);
        Serial.print(F(" ("));
        Serial.print(chunkSize);
        Serial.println(F(" bytes)"));
        
        if (chunkSize == 0) {
          // Last chunk
          state = CHUNK_END;
        } else {
          state = CHUNK_DATA;
          bytesRead = 0;
        }
        break;
      }
      
      case CHUNK_DATA: {
        // Read and write the chunk data
        const size_t dataBufferSize = 256;
        uint8_t dataBuffer[dataBufferSize];
        
        // Read up to the chunk size or buffer size
        int toRead = min(chunkSize - bytesRead, (int)dataBufferSize);
        int readNow = stream->readBytes(dataBuffer, toRead);
        
        if (readNow > 0) {
          // Write to file
          file.write(dataBuffer, readNow);
          bytesRead += readNow;
          totalBytes += readNow;
          
          // Check if we've read the entire chunk
          if (bytesRead >= chunkSize) {
            // Read the trailing \r\n
            while (stream->available() && stream->read() != '\n') {
              // Skip until we find \n
            }
            state = CHUNK_SIZE;
          }
        }
        break;
      }
      
      case CHUNK_END: {
        // Read the final \r\n\r\n
        while (stream->available()) {
          stream->read();
        }
        state = FINISHED;
        break;
      }
      
      case FINISHED:
        break;
    }
    
    yield(); // Allow the watchdog to be fed
  }
  
  unsigned long duration = millis() - startTime;
  Serial.print(F("[CHUNKED] Finished processing chunked response: "));
  Serial.print(totalBytes);
  Serial.print(F(" bytes in "));
  Serial.print(duration);
  Serial.println(F(" ms"));
  
  return true;
}
