// Function to log the complete API request details
void logApiRequest(const char* url, const char* token) {
  Serial.println(F("!!!!! FULL API REQUEST LOGGING START !!!!!"));
  Serial.print(F("URL: "));
  Serial.println(url);
  Serial.println(F("Method: GET"));
  Serial.println(F("Headers:"));
  Serial.print(F("Authorization: Bearer "));
  Serial.println(token); // Print the full token
  Serial.println(F("Accept: application/json"));
  Serial.println(F("!!!!! FULL API REQUEST LOGGING END !!!!!"));
}
