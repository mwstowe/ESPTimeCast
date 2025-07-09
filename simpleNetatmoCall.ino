// A simple, minimal implementation to call the Netatmo API
void simpleNetatmoCall() {
  Serial.println(F("[NETATMO] Making simple API call"));
  
  // Create a secure client
  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
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
  int httpCode = https.GET();
  
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
    
    // Get the response and save to file
    String payload = https.getString();
    file.print(payload);
    file.close();
    
    Serial.println(F("[NETATMO] Response preview:"));
    if (payload.length() > 200) {
      Serial.println(payload.substring(0, 200) + "...");
    } else {
      Serial.println(payload);
    }
  } else {
    Serial.println(F("[NETATMO] Error response:"));
    String payload = https.getString();
    Serial.println(payload);
  }
  
  Serial.println(F("==================================="));
  https.end();
}
