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
  
  // Log that we're sending the response
  Serial.println(F("[API] Sending enhanced response"));
  Serial.print(F("[API] Response size: "));
  Serial.println(payload.length());
  
  // Send the response
  request->send(response);
}

// Function to send a file with enhanced headers
void sendFileWithEnhancedHeaders(AsyncWebServerRequest *request, const char* filePath, const char* contentType) {
  // Check if the file exists
  if (!LittleFS.exists(filePath)) {
    Serial.print(F("[API] File not found: "));
    Serial.println(filePath);
    enhanceApiResponse(request, "application/json", "{\"error\":\"File not found\"}");
    return;
  }
  
  // Open the file
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    Serial.print(F("[API] Failed to open file: "));
    Serial.println(filePath);
    enhanceApiResponse(request, "application/json", "{\"error\":\"Failed to open file\"}");
    return;
  }
  
  // Read the file content
  String fileContent = "";
  while (file.available()) {
    fileContent += (char)file.read();
    yield(); // Feed the watchdog
  }
  file.close();
  
  // Send the response with enhanced headers
  enhanceApiResponse(request, contentType, fileContent);
}
