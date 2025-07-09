// Function to log detailed API request and response information - simplified version
int logDetailedApiRequest(HTTPClient &https, const String &apiUrl) {
  Serial.println(F("[API] Making request to: ") + apiUrl);
  
  // Make the request and log the response
  int httpCode = https.GET();
  Serial.print(F("[API] Response code: "));
  Serial.println(httpCode);
  
  // Only log the response body if there's an error
  if (httpCode != HTTP_CODE_OK) {
    String payload = https.getString();
    Serial.println(F("[API] Error response:"));
    Serial.println(payload);
  } else {
    Serial.println(F("[API] Request successful"));
  }
  
  return httpCode;
}
