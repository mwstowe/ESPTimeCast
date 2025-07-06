// Function to parse Netatmo JSON with proper error handling and memory management
bool parseNetatmoJson(String &payload, JsonDocument &doc) {
  Serial.println(F("[NETATMO] Parsing JSON response..."));
  
  // Try with the provided document first
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print(F("[NETATMO] JSON parse error: "));
    Serial.println(error.c_str());
    
    // If it's a memory error, try to extract just the essential parts
    if (error == DeserializationError::NoMemory) {
      Serial.println(F("[NETATMO] Memory error - trying to extract only essential data"));
      
      // Create a filter to only extract the parts we need
      StaticJsonDocument<512> filter;
      
      // For station data
      JsonObject filter_body = filter.createNestedObject("body");
      JsonArray filter_devices = filter_body.createNestedArray("devices");
      
      JsonObject device = filter_devices.createNestedObject();
      device["_id"] = true;
      device["station_name"] = true;
      
      JsonArray modules = device.createNestedArray("modules");
      JsonObject module = modules.createNestedObject();
      module["_id"] = true;
      module["module_name"] = true;
      
      JsonObject dashboard = module.createNestedObject("dashboard_data");
      dashboard["Temperature"] = true;
      
      // Try parsing with the filter
      error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
      
      if (error) {
        Serial.print(F("[NETATMO] Filtered JSON parse still failed: "));
        Serial.println(error.c_str());
        return false;
      } else {
        Serial.println(F("[NETATMO] Successfully parsed filtered JSON"));
        return true;
      }
    }
    
    return false;
  }
  
  Serial.println(F("[NETATMO] JSON parsed successfully"));
  return true;
}
