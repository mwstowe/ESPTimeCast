// Function to check if the Netatmo access token is valid
bool isNetatmoTokenValid() {
  // Check if the token exists
  if (strlen(netatmoAccessToken) == 0) {
    Serial.println(F("[NETATMO] No access token available"));
    return false;
  }
  
  // Check if the token is properly formatted (should be a long string)
  if (strlen(netatmoAccessToken) < 20) {
    Serial.println(F("[NETATMO] Access token appears to be invalid (too short)"));
    return false;
  }
  
  // Check if the token contains only valid characters
  for (size_t i = 0; i < strlen(netatmoAccessToken); i++) {
    char c = netatmoAccessToken[i];
    if (!isAlphaNumeric(c) && c != '-' && c != '_' && c != '.') {
      Serial.print(F("[NETATMO] Access token contains invalid character at position "));
      Serial.print(i);
      Serial.print(F(": '"));
      Serial.print(c);
      Serial.println(F("'"));
      return false;
    }
  }
  
  return true;
}
