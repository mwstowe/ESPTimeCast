// Function to log the full token before making an API call
void logFullToken() {
  Serial.println(F("[NETATMO] ========== FULL TOKEN LOGGING =========="));
  Serial.print(F("[NETATMO] Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  Serial.print(F("[NETATMO] FULL ACCESS TOKEN: "));
  Serial.println(netatmoAccessToken);
  Serial.print(F("[NETATMO] Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
  Serial.print(F("[NETATMO] FULL REFRESH TOKEN: "));
  Serial.println(netatmoRefreshToken);
  
  // Log the Authorization header exactly as it will be sent
  Serial.println(F("[NETATMO] Authorization header that will be sent:"));
  String authHeader = "Bearer ";
  authHeader += netatmoAccessToken;
  Serial.println(authHeader);
  
  Serial.println(F("[NETATMO] ========================================"));
}
