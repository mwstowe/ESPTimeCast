// Function to clear Netatmo tokens
void clearNetatmoTokens() {
  Serial.println(F("[NETATMO] Clearing access and refresh tokens"));
  netatmoAccessToken[0] = '\0';
  netatmoRefreshToken[0] = '\0';
  saveTokensToConfig();
}
