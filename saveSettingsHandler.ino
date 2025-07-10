#include "saveSettingsState.h"

// Function to set up the save settings handler
void setupSaveSettingsHandler() {
  server.on("/api/netatmo/save-settings", HTTP_POST, 
    // Response handler - called after the request is complete
    [](AsyncWebServerRequest *request) {
      // This is called after the request body has been fully processed
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings received\"}");
    },
    // Upload handler - not used for this endpoint
    NULL,
    // Body handler - processes the request body in chunks
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // This is called for each chunk of the request body
      // We're not actually processing the data, just acknowledging it
      
      // Log the request progress
      Serial.print(F("[NETATMO] Received body chunk: "));
      Serial.print(index);
      Serial.print(F("/"));
      Serial.println(total);
      
      // Don't do any processing that might cause a yield
    }
  );
}
