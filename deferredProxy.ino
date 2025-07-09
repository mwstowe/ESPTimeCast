// Global variables for deferred proxy request
static bool proxyPending = false;
static String proxyEndpoint = "";
static AsyncWebServerRequest* proxyRequest = nullptr;

// Function to process deferred proxy requests
void processProxyRequest() {
  if (!proxyPending || proxyRequest == nullptr) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing deferred proxy request"));
  
  // Clear the flag immediately to prevent repeated attempts if this fails
  proxyPending = false;
  String endpoint = proxyEndpoint;
  AsyncWebServerRequest* request = proxyRequest;
  proxyRequest = nullptr;
  proxyEndpoint = "";
  
  // Create a new client for the API call
  static BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(5000); // Reduced timeout to avoid watchdog issues
  
  String apiUrl = "https://api.netatmo.com/api/" + endpoint;
  Serial.print(F("[NETATMO] Proxying request to: "));
  Serial.println(apiUrl);
  
  if (!https.begin(client, apiUrl)) {
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
    
    // Get error payload with yield
    String errorPayload = https.getString();
    yield(); // Allow the watchdog to be fed
    
    Serial.print(F("[NETATMO] Error payload: "));
    Serial.println(errorPayload);
    https.end();
    
    // If we get a 401 or 403, try to refresh the token
    if (httpCode == 401 || httpCode == 403) {
      Serial.println(F("[NETATMO] Token expired, trying to refresh"));
      
      // Try to refresh the token
      if (refreshNetatmoToken()) {
        Serial.println(F("[NETATMO] Token refreshed, retrying request"));
        
        // Create a new client for the retry
        static BearSSL::WiFiClientSecure client2;
        client2.setInsecure(); // Skip certificate validation to save memory
        
        HTTPClient https2;
        https2.setTimeout(5000); // Reduced timeout
        
        if (!https2.begin(client2, apiUrl)) {
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
          
          // Get error payload with yield
          String errorPayload2 = https2.getString();
          yield(); // Allow the watchdog to be fed
          
          Serial.print(F("[NETATMO] Error payload on retry: "));
          Serial.println(errorPayload2);
          https2.end();
          request->send(httpCode2, "application/json", errorPayload2);
          return;
        }
        
        // Get the response with yield
        Serial.println(F("[NETATMO] Reading response..."));
        String payload2 = https2.getString();
        yield(); // Allow the watchdog to be fed
        
        https2.end();
        
        // Send the response back to the client
        Serial.println(F("[NETATMO] Sending response to client..."));
        request->send(200, "application/json", payload2);
        return;
      } else {
        Serial.println(F("[NETATMO] Failed to refresh token"));
      }
    }
    
    request->send(httpCode, "application/json", errorPayload);
    return;
  }
  
  // Get the response with yield
  Serial.println(F("[NETATMO] Reading response..."));
  String payload = https.getString();
  yield(); // Allow the watchdog to be fed
  
  https.end();
  
  // Send the response back to the client
  Serial.println(F("[NETATMO] Sending response to client..."));
  request->send(200, "application/json", payload);
}
