// Function to debug the Netatmo token - simplified version
void debugNetatmoToken() {
  Serial.println(F("[NETATMO] Debugging token issues"));
  
  // Print token length
  Serial.print(F("[NETATMO] Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  
  // Print truncated token
  Serial.print(F("[NETATMO] Access token: "));
  if (strlen(netatmoAccessToken) > 10) {
    Serial.print(String(netatmoAccessToken).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(netatmoAccessToken).substring(strlen(netatmoAccessToken) - 5));
  } else {
    Serial.println(F("(token too short)"));
  }
  
  // Print refresh token length
  Serial.print(F("[NETATMO] Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
  
  // Print truncated refresh token
  Serial.print(F("[NETATMO] Refresh token: "));
  if (strlen(netatmoRefreshToken) > 10) {
    Serial.print(String(netatmoRefreshToken).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(netatmoRefreshToken).substring(strlen(netatmoRefreshToken) - 5));
  } else {
    Serial.println(F("(token too short)"));
  }
}
