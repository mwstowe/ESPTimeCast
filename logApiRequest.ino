// Function to log the complete API request details - simplified version
void logApiRequest(const char* url, const char* token) {
  Serial.println(F("[API] Request details:"));
  Serial.print(F("[API] URL: "));
  Serial.println(url);
  Serial.println(F("[API] Method: GET"));
  
  // Only log the first 5 and last 5 chars of the token
  Serial.print(F("[API] Token: "));
  if (strlen(token) > 10) {
    Serial.print(String(token).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(token).substring(strlen(token) - 5));
  } else {
    Serial.println(F("(token too short)"));
  }
}
