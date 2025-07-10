#include "saveSettingsState.h"

// Global variables to store form data temporarily
char tempDeviceId[64] = "";
char tempModuleId[64] = "";
char tempIndoorModuleId[64] = "";
bool tempUseNetatmoOutdoor = false;
bool tempPrioritizeNetatmoIndoor = false;
bool formDataReceived = false;

// Function to set up the save settings handler
void setupSaveSettingsHandler() {
  server.on("/api/netatmo/save-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[NETATMO] Handling save settings request"));
    
    // Send response immediately without processing parameters
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings received\"}");
    
    // Set flag for main loop to process parameters
    formDataReceived = true;
    
    // Store parameter count for processing in main loop
    int paramCount = request->params();
    Serial.print(F("[NETATMO] Received "));
    Serial.print(paramCount);
    Serial.println(F(" parameters"));
  });
}

// Function to process form data in the main loop
void processFormData() {
  if (!formDataReceived) {
    return;
  }
  
  Serial.println(F("[NETATMO] Processing form data"));
  formDataReceived = false;
  
  // Copy temporary values to actual variables
  strlcpy(netatmoDeviceId, tempDeviceId, sizeof(netatmoDeviceId));
  strlcpy(netatmoModuleId, tempModuleId, sizeof(netatmoModuleId));
  strlcpy(netatmoIndoorModuleId, tempIndoorModuleId, sizeof(netatmoIndoorModuleId));
  useNetatmoOutdoor = tempUseNetatmoOutdoor;
  prioritizeNetatmoIndoor = tempPrioritizeNetatmoIndoor;
  
  // Schedule the save operation
  settingsSavePending = true;
  
  Serial.println(F("[NETATMO] Form data processed"));
}
