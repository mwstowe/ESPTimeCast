// Global variables for deferred proxy request
static bool proxyPending = false;
static String proxyEndpoint = "";
static AsyncWebServerRequest* proxyRequest = nullptr;

// Function to process deferred proxy requests with memory optimization
void processProxyRequest() {
  if (!proxyPending || proxyRequest == nullptr) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing deferred proxy request"));
  
  // Print memory stats before processing
  Serial.println(F("[NETATMO] Memory stats before proxy request:"));
  printMemoryStats();
  
  // Defragment heap if needed
  if (shouldDefragment()) {
    Serial.println(F("[NETATMO] Defragmenting heap before proxy request"));
    defragmentHeap();
  }
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  proxyPending = false;
  String endpoint = proxyEndpoint;
  AsyncWebServerRequest* request = proxyRequest;
  proxyRequest = nullptr;
  proxyEndpoint = "";
  
  // Create a new client for the API call
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  // Use a smaller buffer size for the HTTP client
  HTTPClient https;
  https.setTimeout(5000); // Reduced timeout to avoid watchdog issues
  
  String apiUrl = "https://api.netatmo.com/api/" + endpoint;
  Serial.print(F("[NETATMO] Proxying request to: "));
  Serial.println(apiUrl);
  
  if (!https.begin(*client, apiUrl)) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API\"}");
    return;
  }
  
  // Add authorization header
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  https.addHeader("Authorization", authHeader);
  https.addHeader("Accept", "application/json");
  
  // Make the request with yield to avoid watchdog issues
  Serial.println(F("[NETATMO] Sending request..."));
  int httpCode = https.GET();
  yield(); // Allow the watchdog to be fed
  
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] Error - HTTP code: "));
    Serial.println(httpCode);
    
    // Get error payload with yield - use streaming to avoid memory issues
    String errorPayload = "";
    if (https.getSize() > 0) {
      const size_t bufSize = 128;
      char buf[bufSize];
      WiFiClient* stream = https.getStreamPtr();
      
      // Read first chunk only for error message
      size_t len = stream->available();
      if (len > bufSize) len = bufSize;
      if (len > 0) {
        int c = stream->readBytes(buf, len);
        if (c > 0) {
          buf[c] = 0;
          errorPayload = String(buf);
        }
      }
    } else {
      errorPayload = https.getString();
    }
    
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    
    // If we get a 401 or 403, check if it's an invalid token error
    if (httpCode == 401 || httpCode == 403) {
      // Check if the error payload indicates an invalid token
      if (isInvalidTokenError(errorPayload)) {
        Serial.println(F("[NETATMO] Token is invalid, need new API keys"));
        String errorResponse = "{\"error\":\"invalid_token\",\"message\":\"Invalid API keys. Please reconfigure Netatmo credentials.\"}";
        proxyRequest->send(401, "application/json", errorResponse);
        proxyPending = false;
        proxyRequest = nullptr;
        return;
      } else {
        Serial.println(F("[NETATMO] Token expired, trying to refresh"));
      }
      
      // Try to refresh the token
      if (refreshNetatmoToken()) {
        Serial.println(F("[NETATMO] Token refreshed, retrying request"));
        
        // Create a new client for the retry
        std::unique_ptr<BearSSL::WiFiClientSecure> client2(new BearSSL::WiFiClientSecure);
        client2->setInsecure(); // Skip certificate validation to save memory
        
        HTTPClient https2;
        https2.setTimeout(5000); // Reduced timeout
        
        if (!https2.begin(*client2, apiUrl)) {
          Serial.println(F("[NETATMO] Error - Failed to connect on retry"));
          request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API on retry\"}");
          return;
        }
        
        // Add authorization header with new token
        String newAuthHeader = "Bearer ";
        newAuthHeader += netatmoAccessToken;
        https2.addHeader("Authorization", newAuthHeader);
        https2.addHeader("Accept", "application/json");
        
        // Make the request again with yield
        Serial.println(F("[NETATMO] Sending retry request..."));
        int httpCode2 = https2.GET();
        yield(); // Allow the watchdog to be fed
        
        if (httpCode2 != HTTP_CODE_OK) {
          Serial.print(F("[NETATMO] Error on retry - HTTP code: "));
          Serial.println(httpCode2);
          
          // Get error payload with yield - use streaming to avoid memory issues
          String errorPayload2 = "";
          if (https2.getSize() > 0) {
            const size_t bufSize = 128;
            char buf[bufSize];
            WiFiClient* stream = https2.getStreamPtr();
            
            // Read first chunk only for error message
            size_t len = stream->available();
            if (len > bufSize) len = bufSize;
            if (len > 0) {
              int c = stream->readBytes(buf, len);
              if (c > 0) {
                buf[c] = 0;
                errorPayload2 = String(buf);
              }
            }
          } else {
            errorPayload2 = https2.getString();
          }
          
          yield(); // Allow the watchdog to be fed
          
          Serial.print(F("[NETATMO] Error payload on retry: "));
          Serial.println(errorPayload2);
          https2.end();
          request->send(httpCode2, "application/json", errorPayload2);
          return;
        }
        
        // Stream the response directly to the client to avoid memory issues
        WiFiClient* stream = https2.getStreamPtr();
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        
        // Use a small buffer to stream the data
        const size_t bufSize = 256;
        uint8_t buf[bufSize];
        int totalRead = 0;
        
        while (https2.connected() && (totalRead < https2.getSize())) {
          // Read available data
          size_t available = stream->available();
          if (available) {
            // Read up to buffer size
            size_t readBytes = available > bufSize ? bufSize : available;
            int bytesRead = stream->readBytes(buf, readBytes);
            
            if (bytesRead > 0) {
              // Write to response
              response->write(buf, bytesRead);
              totalRead += bytesRead;
            }
            
            yield(); // Allow the watchdog to be fed
          } else {
            // No data available, wait a bit
            delay(1);
            yield();
          }
        }
        
        https2.end();
        request->send(response);
        Serial.println(F("[NETATMO] Response streamed to client after token refresh"));
        return;
      } else {
        Serial.println(F("[NETATMO] Failed to refresh token"));
      }
    }
    
    request->send(httpCode, "application/json", errorPayload);
    return;
  }
  
  // Stream the response directly to the client to avoid memory issues
  WiFiClient* stream = https.getStreamPtr();
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  
  // Use a small buffer to stream the data
  const size_t bufSize = 256;
  uint8_t buf[bufSize];
  int totalRead = 0;
  
  while (https.connected() && (totalRead < https.getSize())) {
    // Read available data
    size_t available = stream->available();
    if (available) {
      // Read up to buffer size
      size_t readBytes = available > bufSize ? bufSize : available;
      int bytesRead = stream->readBytes(buf, readBytes);
      
      if (bytesRead > 0) {
        // Write to response
        response->write(buf, bytesRead);
        totalRead += bytesRead;
      }
      
      yield(); // Allow the watchdog to be fed
    } else {
      // No data available, wait a bit
      delay(1);
      yield();
    }
  }
  
  https.end();
  request->send(response);
  Serial.println(F("[NETATMO] Response streamed to client"));
}
