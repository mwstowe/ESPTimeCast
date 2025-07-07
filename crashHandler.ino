// Crash handler for ESP8266

#include <user_interface.h>

// Global variables to store crash information
String lastResetReason = "";
String crashDetails = "";

// Function to initialize crash handler
void setupCrashHandler() {
  // Get the last reset reason
  rst_info *resetInfo = ESP.getResetInfoPtr();
  
  switch (resetInfo->reason) {
    case REASON_DEFAULT_RST:
      lastResetReason = "Normal startup by power on";
      break;
    case REASON_WDT_RST:
      lastResetReason = "Hardware watchdog reset";
      break;
    case REASON_EXCEPTION_RST:
      lastResetReason = "Exception reset";
      crashDetails = getExceptionDetails(resetInfo);
      break;
    case REASON_SOFT_WDT_RST:
      lastResetReason = "Software watchdog reset";
      break;
    case REASON_SOFT_RESTART:
      lastResetReason = "Software restart";
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      lastResetReason = "Deep sleep wake";
      break;
    case REASON_EXT_SYS_RST:
      lastResetReason = "External system reset";
      break;
    default:
      lastResetReason = "Unknown reason: " + String(resetInfo->reason);
  }
  
  Serial.println(F("[CRASH] Last reset reason: ") + lastResetReason);
  
  if (crashDetails.length() > 0) {
    Serial.println(F("[CRASH] Crash details: ") + crashDetails);
  }
  
  // Register exception handler
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);
}

// Function to get exception details
String getExceptionDetails(rst_info *resetInfo) {
  String details = "";
  
  // Exception cause
  switch (resetInfo->exccause) {
    case 0:
      details += "Illegal instruction";
      break;
    case 3:
      details += "Load/store alignment error";
      break;
    case 6:
      details += "Division by zero";
      break;
    case 9:
      details += "Unaligned load or store";
      break;
    case 28:
      details += "Load prohibited";
      break;
    case 29:
      details += "Store prohibited";
      break;
    default:
      details += "Unknown exception (" + String(resetInfo->exccause) + ")";
  }
  
  // Exception address
  details += " at 0x" + String(resetInfo->epc1, HEX);
  
  return details;
}

// Function to get crash information as HTML
String getCrashInfoHtml() {
  String html = "<h2>System Diagnostics</h2>";
  html += "<p><strong>Last Reset Reason:</strong> " + lastResetReason + "</p>";
  
  if (crashDetails.length() > 0) {
    html += "<p><strong>Crash Details:</strong> " + crashDetails + "</p>";
  }
  
  html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "<p><strong>Heap Fragmentation:</strong> " + String(ESP.getHeapFragmentation()) + "%</p>";
  html += "<p><strong>Free Stack:</strong> " + String(ESP.getFreeContStack()) + " bytes</p>";
  html += "<p><strong>Flash Size:</strong> " + String(ESP.getFlashChipSize() / 1024) + " KB</p>";
  html += "<p><strong>Sketch Size:</strong> " + String(ESP.getSketchSize() / 1024) + " KB</p>";
  html += "<p><strong>Free Sketch Space:</strong> " + String(ESP.getFreeSketchSpace() / 1024) + " KB</p>";
  
  return html;
}
