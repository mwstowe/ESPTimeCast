// Function to set up HTTP client with improved timeout settings
void setupHttpClientWithTimeout(HTTPClient &https) {
  // Set a longer timeout for the HTTP client
  https.setTimeout(15000); // 15 seconds timeout
  
  // Log the timeout setting
  Serial.println(F("[HTTP] Setting HTTP client timeout to 15 seconds"));
}
