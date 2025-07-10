#include "saveSettingsState.h"

// Function to set up the save settings handler
void setupSaveSettingsHandler() {
  server.on("/api/netatmo/save-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling save settings request"));
    
    // Check if a save operation is already in progress
    if (settingsSavePending) {
      request->send(409, "application/json", "{\"success\":false,\"message\":\"Settings save already in progress\"}");
      return;
    }
    
    // Extract parameters one by one with safety checks
    if (request->hasParam("netatmoDeviceId", true)) {
      String value = request->getParam("netatmoDeviceId", true)->value();
      strlcpy(netatmoDeviceId, value.c_str(), sizeof(netatmoDeviceId));
    }
    
    if (request->hasParam("netatmoModuleId", true)) {
      String value = request->getParam("netatmoModuleId", true)->value();
      if (value == "none") {
        netatmoModuleId[0] = '\0'; // Empty string
      } else {
        strlcpy(netatmoModuleId, value.c_str(), sizeof(netatmoModuleId));
      }
    }
    
    if (request->hasParam("netatmoIndoorModuleId", true)) {
      String value = request->getParam("netatmoIndoorModuleId", true)->value();
      if (value == "none") {
        netatmoIndoorModuleId[0] = '\0'; // Empty string
      } else {
        strlcpy(netatmoIndoorModuleId, value.c_str(), sizeof(netatmoIndoorModuleId));
      }
    }
    
    if (request->hasParam("useNetatmoOutdoor", true)) {
      String value = request->getParam("useNetatmoOutdoor", true)->value();
      useNetatmoOutdoor = (value == "true" || value == "on" || value == "1");
    }
    
    if (request->hasParam("prioritizeNetatmoIndoor", true)) {
      String value = request->getParam("prioritizeNetatmoIndoor", true)->value();
      prioritizeNetatmoIndoor = (value == "true" || value == "on" || value == "1");
    }
    
    // Send response immediately
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings received\"}");
    
    // Schedule the save operation for the next loop iteration
    settingsSavePending = true;
    
    // Debug output
    Serial.println(F("[NETATMO] Settings received:"));
    Serial.print(F("  netatmoDeviceId: "));
    Serial.println(netatmoDeviceId);
    Serial.print(F("  netatmoModuleId: "));
    Serial.println(netatmoModuleId);
    Serial.print(F("  netatmoIndoorModuleId: "));
    Serial.println(netatmoIndoorModuleId);
    Serial.print(F("  useNetatmoOutdoor: "));
    Serial.println(useNetatmoOutdoor ? "true" : "false");
    Serial.print(F("  prioritizeNetatmoIndoor: "));
    Serial.println(prioritizeNetatmoIndoor ? "true" : "false");
  });
}
