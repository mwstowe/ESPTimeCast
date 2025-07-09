// Function to check if the error payload indicates an invalid token
bool isInvalidTokenError(const String &errorPayload) {
  // Check if the error payload contains "Invalid access token"
  if (errorPayload.indexOf("Invalid access token") >= 0) {
    Serial.println(F("[NETATMO] Error: Invalid access token detected in response"));
    return true;
  }
  return false;
}
