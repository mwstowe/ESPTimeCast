// Functions for handling chunked transfer encoding

// Function to check if a response is chunked
bool isChunkedResponse(HTTPClient& http) {
  // Check if the Transfer-Encoding header contains "chunked"
  String transferEncoding = http.header("Transfer-Encoding");
  bool isChunked = transferEncoding.indexOf("chunked") >= 0;
  
  Serial.print(F("[CHUNKED] Transfer-Encoding header: "));
  Serial.println(transferEncoding);
  Serial.print(F("[CHUNKED] Is chunked response: "));
  Serial.println(isChunked ? "Yes" : "No");
  
  // If header doesn't indicate chunking but we see a chunk size at the start of the stream,
  // we should still treat it as chunked
  if (!isChunked) {
    WiFiClient* stream = http.getStreamPtr();
    if (stream && stream->available() >= 5) { // Need at least a few bytes to check
      // Peek at the first few bytes without consuming them
      char peek[6];
      int i = 0;
      while (i < 5 && stream->available()) {
        peek[i] = stream->peek();
        i++;
      }
      peek[i] = '\0';
      
      // Check if it looks like a hex chunk size followed by CRLF
      bool looksLikeChunk = false;
      for (int j = 0; j < i; j++) {
        if ((peek[j] >= '0' && peek[j] <= '9') || 
            (peek[j] >= 'a' && peek[j] <= 'f') || 
            (peek[j] >= 'A' && peek[j] <= 'F')) {
          // This is a hex digit, continue
          continue;
        } else if (peek[j] == '\r' || peek[j] == '\n') {
          // Found CR or LF after hex digits, likely a chunk
          looksLikeChunk = true;
          break;
        } else {
          // Not a hex digit or CRLF, not a chunk
          break;
        }
      }
      
      if (looksLikeChunk) {
        Serial.println(F("[CHUNKED] Stream appears to be chunked based on content inspection"));
        isChunked = true;
      }
    }
  }
  
  return isChunked;
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
        bool foundCR = false;
        
        // Read until we find CRLF or buffer is full
        while (i < bufSize - 1 && stream->available()) {
          c = stream->read();
          
          if (c == '\r') {
            foundCR = true;
            continue; // Skip CR
          }
          
          if (c == '\n') {
            if (foundCR) {
              // End of chunk size line
              break;
            }
          }
          
          // Only add hex digits to the buffer
          if ((c >= '0' && c <= '9') || 
              (c >= 'a' && c <= 'f') || 
              (c >= 'A' && c <= 'F')) {
            buf[i++] = c;
          }
        }
        
        buf[i] = '\0'; // Null-terminate
        
        // Parse the chunk size (hex)
        chunkSize = strtol(buf, NULL, 16);
        Serial.print(F("[CHUNKED] Chunk size: 0x"));
        Serial.print(buf);
        Serial.print(F(" ("));
        Serial.print(chunkSize);
        Serial.println(F(" bytes)"));
        
        if (chunkSize == 0) {
          // Last chunk (zero-length chunk)
          Serial.println(F("[CHUNKED] Found zero-length chunk, end of transfer"));
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
            // Read the trailing CRLF
            while (stream->available()) {
              char c = stream->read();
              if (c == '\n') {
                break; // Found LF, move to next chunk
              }
            }
            state = CHUNK_SIZE;
          }
        }
        break;
      }
      
      case CHUNK_END: {
        // Read the final CRLF after the 0-length chunk
        bool foundCRLF = false;
        while (stream->available() && !foundCRLF) {
          char c = stream->read();
          if (c == '\n') {
            foundCRLF = true;
          }
        }
        
        // Read any trailing headers (if present)
        bool endOfHeaders = false;
        while (stream->available() && !endOfHeaders) {
          // Look for empty line (CRLFCRLF)
          int consecutiveNewlines = 0;
          while (stream->available()) {
            char c = stream->read();
            if (c == '\r' || c == '\n') {
              consecutiveNewlines++;
              if (consecutiveNewlines >= 2) {
                endOfHeaders = true;
                break;
              }
            } else {
              consecutiveNewlines = 0;
            }
          }
        }
        
        Serial.println(F("[CHUNKED] End of chunked transfer detected"));
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
