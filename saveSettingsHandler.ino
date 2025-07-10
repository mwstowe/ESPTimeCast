#include "saveSettingsState.h"

// Function to set up the save settings handler
void setupSaveSettingsHandler() {
  server.on("/api/netatmo/save-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling save settings request"));
    
    // Send response immediately without doing anything
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings received\"}");
  });
}
