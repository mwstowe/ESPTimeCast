// Function to log the token when the API call is made - simplified version
void logToken() {
  // Simplified logging - just log the first 5 and last 5 chars
  Serial.print(F("[TOKEN] "));
  if (strlen(netatmoAccessToken) > 10) {
    Serial.print(String(netatmoAccessToken).substring(0, 5));
    Serial.print(F("..."));
    Serial.println(String(netatmoAccessToken).substring(strlen(netatmoAccessToken) - 5));
  } else {
    Serial.println(F("(token too short)"));
  }
}

// Hook into the loop function to log the token periodically - disabled
void logTokenPeriodically() {
  // Disabled to reduce logging
  return;
}
