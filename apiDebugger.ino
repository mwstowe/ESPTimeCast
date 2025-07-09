// Function to log detailed API request and response information
int logDetailedApiRequest(HTTPClient &https, const String &apiUrl) {
  Serial.println(F("!!!!! DETAILED API REQUEST LOGGING START !!!!!"));
  
  // Log the request URL and method
  Serial.print(F("Request URL: "));
  Serial.println(apiUrl);
  Serial.println(F("Request Method: GET"));
  
  // Log all request headers
  Serial.println(F("Request Headers:"));
  // We can't directly access the headers, but we know what we're adding
  Serial.print(F("  Authorization: Bearer "));
  
  // Show both the original and encoded token
  Serial.println(netatmoAccessToken);
  Serial.print(F("  Encoded token: "));
  String encodedToken = netatmoAccessToken;
  encodedToken.replace("|", "%7C");
  Serial.println(encodedToken);
  Serial.println(F("  Accept: application/json"));
  
  // Make the request and log the response
  Serial.println(F("Sending request..."));
  int httpCode = https.GET();
  Serial.print(F("Response Status Code: "));
  Serial.println(httpCode);
  
  // Log response headers
  Serial.println(F("Response Headers:"));
  for (int i = 0; i < https.headers(); i++) {
    Serial.print(F("  "));
    Serial.print(https.headerName(i));
    Serial.print(F(": "));
    Serial.println(https.header(i));
  }
  
  // Log response body
  String payload = https.getString();
  Serial.println(F("Response Body:"));
  Serial.println(payload);
  
  Serial.println(F("!!!!! DETAILED API REQUEST LOGGING END !!!!!"));
  
  // Return the HTTP code to the caller
  return httpCode;
}
