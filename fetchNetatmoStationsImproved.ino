/*
 * Improved Netatmo Stations Data Fetcher
 * 
 * This file fetches Netatmo stations data and saves it to /netatmo_stations_data.json
 * The file is used by the web interface to populate device selection dropdowns.
 * 
 * Note: This is separate from /netatmo_config.json which contains
 * the selected device configuration used by the clock system.
 */

// Improved function to fetch Netatmo stations data and save it as clean JSON
void fetchNetatmoStationsImproved() {
  Serial.println(F("[NETATMO] Fetching stations data (improved)"));
  
  // Check if an API call is already in progress
  if (apiCallInProgress) {
    Serial.println(F("[NETATMO] API call already in progress, skipping"));
    return;
  }
  
  // Set the flag to indicate an API call is in progress
  apiCallInProgress = true;
  
  // Get the access token
  String accessToken = getNetatmoToken();
  if (accessToken.isEmpty()) {
    Serial.println(F("[NETATMO] No access token available"));
    apiCallInProgress = false;
    return;
  }
  
  // Set up the HTTP client
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification
  
  HTTPClient https;
  setupHttpClientWithTimeout(https);
  
  // Set up the API URL
  String apiUrl = "https://api.netatmo.com/api/getstationsdata";
  
  if (!https.begin(client, apiUrl)) {
    Serial.println(F("[NETATMO] Failed to connect to API"));
    apiCallInProgress = false;
    return;
  }
  
  // Set the authorization header
  https.addHeader("Authorization", "Bearer " + accessToken);
  
  // Make the request
  int httpCode = logDetailedApiRequest(https, apiUrl);
  
  // Check if the request was successful
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[NETATMO] API request failed with code: "));
    Serial.println(httpCode);
    
    // Check if this is an invalid token error
    String errorPayload = https.getString();
    if (isInvalidTokenError(errorPayload)) {
      Serial.println(F("[NETATMO] Invalid token error detected, forcing refresh"));
      forceNetatmoTokenRefresh();
    }
    
    https.end();
    apiCallInProgress = false;
    return;
  }
  
  // Get the response as a string
  String payload = https.getString();
  
  // Close the connection
  https.end();
  
  // Log the response size
  Serial.print(F("[NETATMO] Response size: "));
  Serial.println(payload.length());
  
  // Preview the first part of the response
  Serial.println(F("[NETATMO] Response preview:"));
  Serial.println(payload.substring(0, 200));
  
  // Write the clean JSON to the file
  // This file is used by the web interface to populate device selection dropdowns
  writeCleanJsonToFile(payload, "/netatmo_stations_data.json");
  
  // Log that we've saved the file
  Serial.println(F("[NETATMO] Stations data saved to file"));
  
  // Reset the flag
  apiCallInProgress = false;
  
  // List all files to verify
  listAllFiles();
}
