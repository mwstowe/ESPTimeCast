// Function to debug the Netatmo token
void debugNetatmoToken() {
  Serial.println(F("[NETATMO] Debugging token issues"));
  
  // Print token length
  Serial.print(F("[NETATMO] Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  
  // Print the FULL token for debugging
  Serial.print(F("[NETATMO] FULL Access token: "));
  Serial.println(netatmoAccessToken);
  
  // Print refresh token length
  Serial.print(F("[NETATMO] Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
  
  // Print the FULL refresh token for debugging
  Serial.print(F("[NETATMO] FULL Refresh token: "));
  Serial.println(netatmoRefreshToken);
}
