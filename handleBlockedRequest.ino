// Function to handle "request is blocked" errors from Netatmo
void handleBlockedRequest(String errorPayload) {
  Serial.println(F("[NETATMO] Detected 'request is blocked' error"));
  
  // Extract the error reference code if present
  String errorRef = "";
  int refStart = errorPayload.indexOf("errorref");
  if (refStart > 0) {
    refStart = errorPayload.indexOf(">", refStart) + 1;
    int refEnd = errorPayload.indexOf("<", refStart);
    if (refStart > 0 && refEnd > refStart) {
      errorRef = errorPayload.substring(refStart, refEnd);
      errorRef.trim();
      Serial.print(F("[NETATMO] Error reference: "));
      Serial.println(errorRef);
    }
  }
  
  Serial.println(F("[NETATMO] This error typically occurs due to:"));
  Serial.println(F("1. Incorrect app permissions in Netatmo developer portal"));
  Serial.println(F("2. Rate limiting by Netatmo API"));
  Serial.println(F("3. IP address restrictions or geoblocking"));
  
  // Clear tokens to force re-authentication on next attempt
  Serial.println(F("[NETATMO] Clearing tokens to force re-authentication"));
  netatmoAccessToken[0] = '\0';
  netatmoRefreshToken[0] = '\0';
  saveTokensToConfig();
  
  // Set a longer delay before next attempt
  Serial.println(F("[NETATMO] Will retry after a longer delay (2 minutes)"));
  needTokenRefresh = true;
  lastBlockedRequest = millis();
}
