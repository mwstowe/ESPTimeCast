// Function to try different authorization header formats
void tryDifferentAuthHeaders() {
  Serial.println(F("[NETATMO] Trying different authorization header formats"));
  
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
    Serial.print(F("[NETATMO] Trying format "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.println(formats[i]);
    
    if (!https.begin(*client, apiUrl)) {
      Serial.println(F("[NETATMO] Error - Failed to connect"));
      continue;
    }
    
    // Add the authorization header with the current format
    https.addHeader("Authorization", formats[i]);
    https.addHeader("Accept", "application/json");
    
    // Make the request
    int httpCode = https.GET();
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[NETATMO] HTTP response code: "));
    Serial.println(httpCode);
    
    // Check if we got a successful response
    if (httpCode == HTTP_CODE_OK) {
      Serial.println(F("[NETATMO] Success! Found working format:"));
      Serial.println(formats[i]);
      
      // Log the response
      String payload = https.getString();
      Serial.println(F("[NETATMO] Response:"));
      Serial.println(payload);
      
      // Save the working format for future use
      Serial.println(F("[NETATMO] Saving working format for future use"));
      // TODO: Save the working format
      
      https.end();
      return;
    } else {
      // Log the error
      String payload = https.getString();
      Serial.print(F("[NETATMO] Error - HTTP code: "));
      Serial.println(httpCode);
      Serial.print(F("[NETATMO] Error payload: "));
      Serial.println(payload);
    }
    
    https.end();
    delay(1000); // Wait a bit before trying the next format
  }
  
  Serial.println(F("[NETATMO] All formats failed"));
}
