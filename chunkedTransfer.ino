// Functions for handling chunked transfer encoding

// Function to check if a response is chunked
bool isChunkedResponse(HTTPClient& http) {
  // Check if the Transfer-Encoding header contains "chunked"
  String transferEncoding = http.header("Transfer-Encoding");
  bool isChunked = transferEncoding.indexOf("chunked") >= 0;
  
  Serial.print(F("[CHUNKED] Transfer-Encoding header: "));
  Serial.println(transferEncoding);
  
  // If Content-Length is -1, it's likely chunked
  int contentLength = http.getSize();
  Serial.print(F("[CHUNKED] Content-Length: "));
  Serial.println(contentLength);
  
  if (contentLength == -1) {
    Serial.println(F("[CHUNKED] Content-Length is -1, likely chunked encoding"));
    isChunked = true;
  }
  
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
      
      Serial.print(F("[CHUNKED] First bytes of stream: "));
      for (int j = 0; j < i; j++) {
        Serial.print((char)peek[j]);
      }
      Serial.println();
      
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
  
  Serial.print(F("[CHUNKED] Is chunked response: "));
  Serial.println(isChunked ? "Yes" : "No");
  
  return isChunked;
}

// Completely rewritten memory-efficient chunked encoding handler
bool writeChunkedResponseToFile(WiFiClient* stream, File& file) {
  Serial.println(F("[CHUNKED] Starting chunked response processing"));
  
  int totalBytes = 0;
  unsigned long startTime = millis();
  const unsigned long timeout = 10000; // 10 second timeout
  
  // Process chunks until we get a zero-length chunk
  while (true) {
    // Check for timeout
    unsigned long currentTime = millis();
    if (currentTime - startTime > timeout) {
      Serial.println(F("[CHUNKED] Timeout reading chunked response"));
      return false;
    }
    
    // Log progress every 2 seconds
    if (currentTime % 2000 < 10) {
      Serial.print(F("[CHUNKED] Still processing after "));
      Serial.print((currentTime - startTime) / 1000);
      Serial.println(F(" seconds"));
    }
    
    // Read the chunk size (hex digits)
    int chunkSize = 0;
    String chunkSizeStr = ""; // For logging only
    
    // Wait for data to be available
    while (!stream->available()) {
      yield();
      if (millis() - startTime > timeout) {
        Serial.println(F("[CHUNKED] Timeout waiting for chunk size"));
        return false;
      }
    }
    
    // Read hex digits until CR
    bool foundCR = false;
    while (!foundCR) {
      if (stream->available()) {
        char c = stream->read();
        
        if (c == '\r') {
          foundCR = true;
          // Skip the LF that should follow
          while (!stream->available()) {
            yield();
            if (millis() - startTime > timeout) {
              Serial.println(F("[CHUNKED] Timeout waiting for LF after CR"));
              return false;
            }
          }
          char lf = stream->read();
          if (lf != '\n') {
            Serial.println(F("[CHUNKED] Warning: Expected LF after CR"));
          }
        } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
          // For logging
          chunkSizeStr += c;
          
          // Convert hex digit to integer
          if (c >= '0' && c <= '9') {
            chunkSize = (chunkSize << 4) | (c - '0');
          } else if (c >= 'a' && c <= 'f') {
            chunkSize = (chunkSize << 4) | (c - 'a' + 10);
          } else { // c >= 'A' && c <= 'F'
            chunkSize = (chunkSize << 4) | (c - 'A' + 10);
          }
        }
        // Ignore any other characters (like chunk extensions)
      } else {
        yield();
        if (millis() - startTime > timeout) {
          Serial.println(F("[CHUNKED] Timeout reading chunk size"));
          return false;
        }
      }
    }
    
    Serial.print(F("[CHUNKED] Chunk size: 0x"));
    Serial.print(chunkSizeStr);
    Serial.print(F(" ("));
    Serial.print(chunkSize);
    Serial.println(F(" bytes)"));
    
    // A chunk size of 0 means we're done
    if (chunkSize == 0) {
      Serial.println(F("[CHUNKED] Found end chunk (size 0), transfer complete"));
      
      // Read the final CRLF
      while (!stream->available()) {
        yield();
        if (millis() - startTime > timeout) {
          Serial.println(F("[CHUNKED] Timeout waiting for final CRLF"));
          break;
        }
      }
      
      // Read and discard the final CRLF
      if (stream->available()) {
        char cr = stream->read();
        if (cr != '\r') {
          Serial.println(F("[CHUNKED] Warning: Expected CR at end of transfer"));
        }
        
        while (!stream->available()) {
          yield();
          if (millis() - startTime > timeout) break;
        }
        
        if (stream->available()) {
          char lf = stream->read();
          if (lf != '\n') {
            Serial.println(F("[CHUNKED] Warning: Expected LF at end of transfer"));
          }
        }
      }
      
      // We're done!
      break;
    }
    
    // Read the chunk data
    int bytesRemaining = chunkSize;
    const size_t bufSize = 64; // Small buffer size to save memory
    uint8_t buf[bufSize];
    
    while (bytesRemaining > 0) {
      // Check for timeout
      if (millis() - startTime > timeout) {
        Serial.println(F("[CHUNKED] Timeout reading chunk data"));
        return false;
      }
      
      // Wait for data to be available
      while (!stream->available()) {
        yield();
        if (millis() - startTime > timeout) {
          Serial.println(F("[CHUNKED] Timeout waiting for chunk data"));
          return false;
        }
      }
      
      // Reset timeout when data is available
      startTime = millis();
      
      // Read up to buffer size or remaining bytes in chunk
      size_t available = stream->available();
      size_t readBytes = (available < bufSize) ? available : bufSize;
      readBytes = (readBytes < (size_t)bytesRemaining) ? readBytes : (size_t)bytesRemaining;
      int bytesRead = stream->readBytes(buf, readBytes);
      
      if (bytesRead > 0) {
        // Write to file
        file.write(buf, bytesRead);
        totalBytes += bytesRead;
        bytesRemaining -= bytesRead;
        
        // Print progress every 1KB
        if (totalBytes % 1024 == 0) {
          Serial.print(F("[CHUNKED] Read "));
          Serial.print(totalBytes);
          Serial.println(F(" bytes"));
        }
      }
      
      yield(); // Allow the watchdog to be fed
    }
    
    // Read the CRLF at the end of the chunk
    while (!stream->available()) {
      yield();
      if (millis() - startTime > timeout) {
        Serial.println(F("[CHUNKED] Timeout waiting for CRLF after chunk"));
        break;
      }
    }
    
    if (stream->available()) {
      char cr = stream->read();
      if (cr != '\r') {
        Serial.println(F("[CHUNKED] Warning: Expected CR after chunk"));
      }
      
      while (!stream->available()) {
        yield();
        if (millis() - startTime > timeout) break;
      }
      
      if (stream->available()) {
        char lf = stream->read();
        if (lf != '\n') {
          Serial.println(F("[CHUNKED] Warning: Expected LF after chunk"));
        }
      }
    }
  }
  
  unsigned long duration = millis() - startTime;
  Serial.print(F("[CHUNKED] Finished processing chunked response: "));
  Serial.print(totalBytes);
  Serial.print(F(" bytes in "));
  Serial.print(duration);
  Serial.println(F(" ms"));
  
  return true;
}
