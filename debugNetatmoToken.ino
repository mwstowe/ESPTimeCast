// Function to debug the Netatmo token
void debugNetatmoToken() {
  Serial.println(F("[NETATMO] Debugging token issues"));
  
  // Print token length
  Serial.print(F("[NETATMO] Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  
  // Print a masked version of the token
  Serial.print(F("[NETATMO] Access token: "));
  if (strlen(netatmoAccessToken) > 10) {
    // Print first 5 chars
    Serial.print(String(netatmoAccessToken).substring(0, 5));
    Serial.print(F("..."));
    // Print last 5 chars
    Serial.println(String(netatmoAccessToken).substring(strlen(netatmoAccessToken) - 5));
  } else {
    Serial.println(F("(token too short)"));
  }
  
  // Print refresh token length
  Serial.print(F("[NETATMO] Refresh token length: "));
  Serial.println(strlen(netatmoRefreshToken));
  
  // Check if token contains a pipe character
  bool hasPipe = false;
  for (size_t i = 0; i < strlen(netatmoAccessToken); i++) {
    if (netatmoAccessToken[i] == '|') {
      hasPipe = true;
      Serial.print(F("[NETATMO] Pipe character found at position: "));
      Serial.println(i);
      break;
    }
  }
  
  if (!hasPipe) {
    Serial.println(F("[NETATMO] WARNING: Token does not contain a pipe character!"));
  }
}
