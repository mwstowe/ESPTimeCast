// Optimized handlers for web requests to reduce memory usage

// Function to set up optimized handlers
void setupOptimizedHandlers() {
  // Replace standard handlers with optimized versions
  
  // Netatmo devices.js handler
  server.on("/netatmo-devices.js", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /netatmo-devices.js"));
    
    // Check if we should handle this request
    if (!shouldHandleWebRequest()) {
      request->send(503, "text/plain", "Server busy, try again later");
      return;
    }
    
    // Use streaming response to reduce memory usage
    sendFileWithEnhancedHeaders(request, "/netatmo-devices.js", "application/javascript");
  });
  
  // Netatmo HTML handler
  server.on("/netatmo.html", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /netatmo.html"));
    
    // Check if we should handle this request
    if (!shouldHandleWebRequest()) {
      request->send(503, "text/plain", "Server busy, try again later");
      return;
    }
    
    // Use streaming response to reduce memory usage
    sendFileWithEnhancedHeaders(request, "/netatmo.html", "text/html");
  });
  
  // Index HTML handler
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /"));
    
    // Check if we should handle this request
    if (!shouldHandleWebRequest()) {
      request->send(503, "text/plain", "Server busy, try again later");
      return;
    }
    
    // Use streaming response to reduce memory usage
    sendFileWithEnhancedHeaders(request, "/index.html", "text/html");
  });
  
  // CSS handler
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println(F("[WEBSERVER] Request: /style.css"));
    
    // Check if we should handle this request
    if (!shouldHandleWebRequest()) {
      request->send(503, "text/plain", "Server busy, try again later");
      return;
    }
    
    // Use streaming response to reduce memory usage
    sendFileWithEnhancedHeaders(request, "/style.css", "text/css");
  });
}
