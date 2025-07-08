// Function to fetch Netatmo stations and devices - Redirects to the new implementation
String fetchNetatmoDevices() {
  Serial.println(F("[NETATMO] fetchNetatmoDevices() called - triggering async fetch"));
  
  // Set the flag to trigger the async fetch in the main loop
  fetchDevicesPending = true;
  
  // Return cached data if available
  if (deviceData.length() > 0) {
    return deviceData;
  }
  
  // Return a placeholder response
  return "{\"body\":{\"devices\":[]},\"status\":\"fetching\"}";
}

// Helper function to parse Netatmo JSON response
bool parseNetatmoJson(String &payload, JsonDocument &doc) {
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("[NETATMO] JSON parsing failed: "));
    Serial.println(error.c_str());
    return false;
  }
  return true;
}
