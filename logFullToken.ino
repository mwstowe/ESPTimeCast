// Function to log the full token before making an API call - simplified version
void logFullToken() {
  Serial.println(F("[TOKEN] Simplified token logging"));
  Serial.print(F("[TOKEN] Access token: "));
  if (strlen(netatmoAccessToken) > 10) {
    Serial.print(String(netatmoAccessToken).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(netatmoAccessToken).substring(strlen(netatmoAccessToken) - 5));
  } else {
    Serial.println(F("(token too short)"));
  }
  
  // Only log the token length for the refresh token
  Serial.print(F("[TOKEN] Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
}
