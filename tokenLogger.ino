// Function to log the token when the API call is made
void logToken() {
  Serial.println(F("!!!!! TOKEN LOGGER CALLED !!!!!"));
  Serial.print(F("Access token length: "));
  Serial.println(strlen(netatmoAccessToken));
  Serial.print(F("FULL ACCESS TOKEN: "));
  Serial.println(netatmoAccessToken);
  Serial.println(F("!!!!! TOKEN LOGGER END !!!!!"));
}

// Hook into the loop function to log the token periodically
void logTokenPeriodically() {
  static unsigned long lastTokenLog = 0;
  if (millis() - lastTokenLog > 5000) { // Log every 5 seconds
    lastTokenLog = millis();
    logToken();
  }
}
