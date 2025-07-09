// Function to log the full token before making an API call
void logFullToken() {
  Serial.println(F("!!!!! FULL TOKEN LOGGING START !!!!!"));
  Serial.print(F("Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  Serial.print(F("FULL ACCESS TOKEN: "));
  Serial.println(netatmoAccessToken);
  Serial.print(F("Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
  Serial.print(F("FULL REFRESH TOKEN: "));
  Serial.println(netatmoRefreshToken);
  
  // Log the Authorization header exactly as it will be sent
  Serial.println(F("Authorization header that will be sent:"));
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  Serial.println(authHeader);
  
  Serial.println(F("!!!!! FULL TOKEN LOGGING END !!!!!"));
}
