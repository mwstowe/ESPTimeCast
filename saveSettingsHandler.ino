#include "saveSettingsState.h"

// Function to set up the save settings handler
void setupSaveSettingsHandler() {
  // Use a GET request instead of POST to avoid the body parsing issues
  server.on("/api/netatmo/save-settings-simple", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling simple save settings request"));
    
    // Extract parameters from URL query string
    if (request->hasParam("deviceId")) {
      String value = request->getParam("deviceId")->value();
      strlcpy(netatmoDeviceId, value.c_str(), sizeof(netatmoDeviceId));
      
      // Also update stationId if it's empty (for backward compatibility)
      if (strlen(netatmoStationId) == 0) {
        strlcpy(netatmoStationId, value.c_str(), sizeof(netatmoStationId));
      }
    }
    
    // Add support for explicitly setting stationId
    if (request->hasParam("stationId")) {
      String value = request->getParam("stationId")->value();
      if (value == "none") {
        netatmoStationId[0] = '\0'; // Empty string
      } else {
        strlcpy(netatmoStationId, value.c_str(), sizeof(netatmoStationId));
      }
    }
    
    if (request->hasParam("moduleId")) {
      String value = request->getParam("moduleId")->value();
      if (value == "none") {
        netatmoModuleId[0] = '\0'; // Empty string
      } else {
        strlcpy(netatmoModuleId, value.c_str(), sizeof(netatmoModuleId));
      }
    }
    
    if (request->hasParam("indoorModuleId")) {
      String value = request->getParam("indoorModuleId")->value();
      if (value == "none") {
        netatmoIndoorModuleId[0] = '\0'; // Empty string
      } else {
        strlcpy(netatmoIndoorModuleId, value.c_str(), sizeof(netatmoIndoorModuleId));
      }
    }
    
    if (request->hasParam("useOutdoor")) {
      String value = request->getParam("useOutdoor")->value();
      useNetatmoOutdoor = (value == "true" || value == "1");
    }
    
    if (request->hasParam("prioritizeIndoor")) {
      String value = request->getParam("prioritizeIndoor")->value();
      prioritizeNetatmoIndoor = (value == "true" || value == "1");
    }
    
    // Send response
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings received\"}");
    
    // Set flag to save settings in the main loop
    settingsSavePending = true;
    
    // Debug output
    Serial.println(F("[NETATMO] Settings received:"));
    Serial.print(F("  netatmoDeviceId: "));
    Serial.println(netatmoDeviceId);
    Serial.print(F("  netatmoStationId: "));
    Serial.println(netatmoStationId);
    Serial.print(F("  netatmoModuleId: "));
    Serial.println(netatmoModuleId);
    Serial.print(F("  netatmoIndoorModuleId: "));
    Serial.println(netatmoIndoorModuleId);
    Serial.print(F("  useNetatmoOutdoor: "));
    Serial.println(useNetatmoOutdoor ? "true" : "false");
    Serial.print(F("  prioritizeNetatmoIndoor: "));
    Serial.println(prioritizeNetatmoIndoor ? "true" : "false");
  });
  
  // Keep the original endpoint for compatibility, but make it do nothing
  server.on("/api/netatmo/save-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling original save settings request"));
    request->send(200, "application/json", "{\"success\":false,\"message\":\"Please use the simple endpoint\"}");
  });
}
