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
  
  // Open the file for writing
  File file = LittleFS.open("/netatmo_stations_data.json", "w");
  if (!file) {
    Serial.println(F("[NETATMO] Failed to open file for writing"));
    https.end();
    apiCallInProgress = false;
    return;
  }
  
  // Get the response size if available
  int contentLength = https.getSize();
  Serial.print(F("[NETATMO] Content length: "));
  Serial.println(contentLength);
  
  // Stream the response directly to the file
  WiFiClient *stream = https.getStreamPtr();
  
  // Buffer for reading data
  const size_t bufferSize = 256;
  uint8_t buffer[bufferSize];
  int totalBytes = 0;
  
  // Read all data from the stream
  while (https.connected() && (contentLength > 0 || contentLength == -1)) {
    // Feed the watchdog
    yield();
    
    // Get available data size
    size_t available = stream->available();
    if (available) {
      // Read up to buffer size
      size_t bytesToRead = available;
      if (bytesToRead > bufferSize) {
        bytesToRead = bufferSize;
      }
      
      // Read data into buffer
      int bytesRead = stream->readBytes(buffer, bytesToRead);
      
      // Write clean JSON to file
      writeCleanJsonFromBuffer(buffer, bytesRead, file);
      
      // Update total bytes and content length
      totalBytes += bytesRead;
      if (contentLength > 0) {
        contentLength -= bytesRead;
      }
    }
    
    // If no data available, wait a bit
    if (!available) {
      delay(10);
    }
  }
  
  // Close the file
  file.close();
  
  // Close the connection
  https.end();
  
  // Log that we've saved the file
  Serial.print(F("[NETATMO] Stations data saved to file, bytes: "));
  Serial.println(totalBytes);
  
  // Verify the file exists
  Serial.print(F("[NETATMO] File exists check: "));
  Serial.println(LittleFS.exists("/netatmo_stations_data.json") ? "Yes" : "No");
  
  // Verify that the file exists and can be read
  if (LittleFS.exists("/netatmo_stations_data.json")) {
    Serial.println(F("[NETATMO] File exists in filesystem"));
    
    // Try to open and read the first few bytes to verify it's accessible
    File readFile = LittleFS.open("/netatmo_stations_data.json", "r");
    if (readFile) {
      Serial.print(F("[NETATMO] File opened successfully, size: "));
      Serial.println(readFile.size());
      
      // Read and print the first 50 bytes
      char buffer[51];
      size_t bytesRead = readFile.readBytes(buffer, 50);
      buffer[bytesRead] = '\0'; // Null-terminate the string
      
      Serial.print(F("[NETATMO] First 50 bytes: "));
      Serial.println(buffer);
      
      readFile.close();
    } else {
      Serial.println(F("[NETATMO] Failed to open file for reading"));
    }
  } else {
    Serial.println(F("[NETATMO] File does not exist in filesystem after saving"));
  }
  
  // Reset the flag
  apiCallInProgress = false;
  
  // List all files to verify
  listAllFiles();
}
