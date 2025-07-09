// Function to log the complete API request details
void logApiRequest(const char* url, const char* token) {
  Serial.println(F("[NETATMO] ========== COMPLETE API REQUEST =========="));
  Serial.print(F("[NETATMO] URL: "));
  Serial.println(url);
  Serial.println(F("[NETATMO] Method: GET"));
  Serial.println(F("[NETATMO] Headers:"));
  Serial.print(F("[NETATMO] Authorization: Bearer "));
  Serial.println(token);
  Serial.println(F("[NETATMO] Accept: application/json"));
  Serial.println(F("[NETATMO] ========================================="));
}
