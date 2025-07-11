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
  
  // OAuth2 tokens can contain many special characters including |, ~, etc.
  // Instead of checking for valid characters, just log the token format
  Serial.print(F("[NETATMO] Access token format check: length="));
  Serial.println(strlen(netatmoAccessToken));
  
  return true;
}
