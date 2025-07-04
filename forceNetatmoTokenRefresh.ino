// Function to force a refresh of the Netatmo token on next API call
void forceNetatmoTokenRefresh() {
  Serial.println(F("[NETATMO] Forcing token refresh on next API call"));
  netatmoAccessToken[0] = '\0';  // Clear access token but keep refresh token
  needTokenRefresh = true;  // Set flag to retry soon
  Serial.println(F("[NETATMO] Will retry in 30 seconds"));
}
