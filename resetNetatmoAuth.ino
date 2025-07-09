// Function to completely reset Netatmo authentication
void resetNetatmoAuth() {
  Serial.println(F("[NETATMO] Resetting Netatmo authentication"));
  
  // Clear all tokens
  memset(netatmoAccessToken, 0, sizeof(netatmoAccessToken));
  memset(netatmoRefreshToken, 0, sizeof(netatmoRefreshToken));
  
  // Save the cleared tokens to config
  saveTokensToConfig();
  
  Serial.println(F("[NETATMO] Netatmo authentication reset complete"));
}
