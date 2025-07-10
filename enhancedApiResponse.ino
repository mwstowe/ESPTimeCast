// Function to enhance API responses with CORS headers and logging
void enhanceApiResponse(AsyncWebServerRequest *request, const char* contentType, const String &payload) {
  // Create a response
  AsyncResponseStream *response = request->beginResponseStream(contentType);
  
  // Add CORS headers
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  // Add the payload
  response->print(payload);
  
  // Send the response
  request->send(response);
}

// Function to stream a file with enhanced headers
void sendFileWithEnhancedHeaders(AsyncWebServerRequest *request, const char* filePath, const char* contentType) {
  // Check if the file exists
  if (!LittleFS.exists(filePath)) {
    enhanceApiResponse(request, "application/json", "{\"error\":\"File not found\"}");
    return;
  }
  
  // Use chunked response for files to reduce memory usage
  AsyncWebServerResponse *response = request->beginChunkedResponse(contentType, 
    [filePath](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      static File file;
      static size_t totalSent = 0;
      
      // Open file on first call
      if (index == 0) {
        file = LittleFS.open(filePath, "r");
        totalSent = 0;
        if (!file) return 0;
      }
      
      // Check if we've sent everything
      if (totalSent >= file.size()) {
        file.close();
        return 0; // End of file
      }
      
      // Read a chunk - use larger chunks (up to 1KB)
      size_t bytesToRead = min(maxLen, file.size() - totalSent);
      size_t bytesRead = file.read(buffer, bytesToRead);
      
      if (bytesRead > 0) {
        totalSent += bytesRead;
        return bytesRead;
      } else {
        file.close();
        return 0; // Error or end of file
      }
    }
  );
  
  // Add CORS headers
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
  response->addHeader("Cache-Control", "no-cache");
  
  request->send(response);
}
