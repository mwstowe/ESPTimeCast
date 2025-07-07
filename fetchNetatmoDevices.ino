// Function to fetch Netatmo stations and devices - Simplified version
String fetchNetatmoDevices() {
  Serial.println(F("[NETATMO] Fetching stations and devices..."));
  
  // Return a simple message instead of actually fetching
  return "{\"devices\":[{\"id\":\"manual\",\"name\":\"Manual Configuration\",\"type\":\"station\",\"modules\":[{\"id\":\"manual_main\",\"name\":\"Main Station\",\"type\":\"NAMain\"},{\"id\":\"manual_outdoor\",\"name\":\"Outdoor Module\",\"type\":\"NAModule1\"},{\"id\":\"manual_indoor\",\"name\":\"Indoor Module\",\"type\":\"NAModule4\"}]}]}";
}

// Helper function to parse Netatmo JSON response
bool parseNetatmoJson(String &payload, DynamicJsonDocument &doc) {
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("[NETATMO] JSON parsing failed: "));
    Serial.println(error.c_str());
    return false;
  }
  return true;
}
