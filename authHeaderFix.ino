// Function to try different authorization header formats - simplified version
void tryDifferentAuthHeaders() {
  Serial.println(F("[AUTH] Testing different authorization methods"));
  
  // Create a new client for the API call with minimal buffer sizes
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  client->setBufferSizes(512, 512); // Reduce buffer sizes to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // Reduce timeout to 10 seconds
  
  // Use getstationsdata endpoint for weather stations
  String apiUrl = "https://api.netatmo.com/api/getstationsdata";
  
  // Try different authorization header formats
  String formats[] = {
    // Format 1: Bearer token (original)
    "Bearer " + String(netatmoAccessToken),
    
    // Format 2: Bearer token with URL-encoded pipe
    "Bearer " + String(encodeToken(netatmoAccessToken)),
    
    // Format 3: Just the token without Bearer
    String(netatmoAccessToken),
    
    // Format 4: Just the token with URL-encoded pipe
    String(encodeToken(netatmoAccessToken)),
    
    // Format 5: access_token=token
    "access_token=" + String(netatmoAccessToken)
  };
  
  for (int i = 0; i < 5; i++) {
    Serial.print(F("[AUTH] Testing format "));
    Serial.println(i + 1);
    
    if (!https.begin(*client, apiUrl)) {
      Serial.println(F("[AUTH] Connection failed"));
      continue;
    }
    
    // Add the authorization header with the current format
    https.addHeader("Authorization", formats[i]);
    https.addHeader("Accept", "application/json");
    
    // Make the request
    int httpCode = https.GET();
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[AUTH] Response code: "));
    Serial.println(httpCode);
    
    // Check if we got a successful response
    if (httpCode == HTTP_CODE_OK) {
      Serial.println(F("[AUTH] Success with format ") + String(i + 1));
      https.end();
      return;
    }
    
    https.end();
    delay(1000); // Wait a bit before trying the next format
  }
  
  Serial.println(F("[AUTH] All formats failed"));
}
