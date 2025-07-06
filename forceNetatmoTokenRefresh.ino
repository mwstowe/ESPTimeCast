// Function to force a refresh of the Netatmo token on next API call
void forceNetatmoTokenRefresh() {
  Serial.println(F("[NETATMO] Forcing token refresh on next API call"));
  netatmoAccessToken[0] = '\0';  // Clear access token but keep refresh token
  
  // If we've been having persistent issues, also clear the refresh token
  // to force a full re-authentication
  static int refreshAttempts = 0;
  refreshAttempts++;
  
  if (refreshAttempts >= 3) {
    Serial.println(F("[NETATMO] Multiple refresh attempts detected, clearing refresh token too"));
    netatmoRefreshToken[0] = '\0';
    refreshAttempts = 0;
    saveTokensToConfig();
  }
  
  needTokenRefresh = true;  // Set flag to retry soon
  Serial.println(F("[NETATMO] Will retry in 30 seconds"));
}
