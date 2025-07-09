// A simple, minimal implementation to call the Netatmo API
void simpleNetatmoCall() {
  Serial.println(F("[NETATMO] Making simple API call"));
  
  // Create a secure client
  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  https.setTimeout(5000); // Reduce timeout to 5 seconds to avoid watchdog issues
  
  // Try the homesdata endpoint
  String apiUrl = "https://api.netatmo.com/api/homesdata";
  Serial.print(F("[NETATMO] Trying endpoint: "));
  Serial.println(apiUrl);
  
  Serial.println(F("========== FULL REQUEST (SIMPLE) =========="));
  Serial.print(F("GET "));
  Serial.print(apiUrl);
  Serial.println(F(" HTTP/1.1"));
  Serial.println(F("Host: api.netatmo.com"));
  Serial.print(F("Authorization: Bearer "));
  Serial.println(netatmoAccessToken); // Print full token for debugging
  Serial.println(F("Accept: application/json"));
  Serial.println(F("=================================="));
  
  if (!https.begin(client, apiUrl)) {
    Serial.println(F("[NETATMO] Error - Failed to connect"));
    return;
  }
  
  // Add authorization header - as simple as possible
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  https.addHeader("Authorization", authHeader);
  https.addHeader("Accept", "application/json");
  
  // Make the request
  Serial.println(F("[NETATMO] Sending request..."));
  
  // Add yield before making the request
  yield();
  
  int httpCode = https.GET();
  
  // Add yield immediately after making the request
  yield();
  
  Serial.print(F("[NETATMO] HTTP response code: "));
  Serial.println(httpCode);
  
  Serial.println(F("========== FULL RESPONSE (SIMPLE) =========="));
  
  if (httpCode == HTTP_CODE_OK) {
    // Create the devices directory if it doesn't exist
    if (!LittleFS.exists("/devices")) {
      LittleFS.mkdir("/devices");
    }
    
    // Open a file to save the response
    File file = LittleFS.open("/devices/netatmo_simple.json", "w");
    if (!file) {
      Serial.println(F("[NETATMO] Failed to open file for writing"));
      https.end();
      return;
    }
    
    // Get the response in chunks with yield calls
    WiFiClient* stream = https.getStreamPtr();
    if (stream) {
      const size_t bufSize = 128; // Smaller buffer size to avoid memory issues
      uint8_t buf[bufSize];
      int totalRead = 0;
      String preview = "";
      bool previewComplete = false;
      
      // Read data in chunks with yield calls
      while (https.connected()) {
        // Feed the watchdog
        yield();
        
        size_t available = stream->available();
        if (available) {
          // Read up to buffer size
          size_t readBytes = available > bufSize ? bufSize : available;
          int bytesRead = stream->readBytes(buf, readBytes);
          
          if (bytesRead > 0) {
            // Write to file
            file.write(buf, bytesRead);
            totalRead += bytesRead;
            
            // Capture preview for logging
            if (!previewComplete) {
              for (int i = 0; i < bytesRead && preview.length() < 200; i++) {
                preview += (char)buf[i];
              }
              if (preview.length() >= 200) {
                previewComplete = true;
              }
            }
            
            // Print progress and feed watchdog
            if (totalRead % 256 == 0) {
              Serial.print(F("[NETATMO] Read "));
              Serial.print(totalRead);
              Serial.println(F(" bytes"));
              yield();
            }
          }
        } else if (!https.connected()) {
          break;
        } else {
          // No data available but still connected, wait a bit and feed watchdog
          delay(1);
          yield();
        }
      }
      
      file.close();
      
      Serial.print(F("[NETATMO] Total bytes read: "));
      Serial.println(totalRead);
      
      Serial.println(F("[NETATMO] Response preview:"));
      Serial.println(preview);
    } else {
      // Fallback to getString() if streaming doesn't work
      Serial.println(F("[NETATMO] Using getString() fallback"));
      
      // Get the response with yield calls
      String payload = "";
      size_t chunkSize = 128;
      size_t totalSize = https.getSize();
      size_t remaining = totalSize;
      
      while (remaining > 0) {
        // Get a chunk of the response
        size_t toRead = remaining > chunkSize ? chunkSize : remaining;
        String chunk = https.getString();
        
        // Append to payload
        payload += chunk;
        remaining -= chunk.length();
        
        // Feed the watchdog
        yield();
      }
      
      // Save to file
      file.print(payload);
      file.close();
      
      Serial.println(F("[NETATMO] Response preview:"));
      if (payload.length() > 200) {
        Serial.println(payload.substring(0, 200) + "...");
      } else {
        Serial.println(payload);
      }
    }
  } else {
    Serial.println(F("[NETATMO] Error response:"));
    
    // Get error response with yield
    String payload = "";
    size_t chunkSize = 128;
    
    // Get the response in smaller chunks
    while (https.connected()) {
      String chunk = https.getString().substring(0, chunkSize);
      if (chunk.length() == 0) break;
      
      payload += chunk;
      
      // Feed the watchdog
      yield();
    }
    
    Serial.println(payload);
  }
  
  Serial.println(F("==================================="));
  https.end();
  
  // Final yield to ensure watchdog is fed
  yield();
}
