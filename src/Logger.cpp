#include "Logger.h"

Logger::Logger() {
  // Constructor
}

void Logger::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: Failed to initialize SPIFFS for logging");
    return;
  }
  
  // Clear old log file on startup to prevent it from growing too large
  SPIFFS.remove(logFilePath);
  
  // Create a new log file
  File logFile = SPIFFS.open(logFilePath, FILE_WRITE);
  if (!logFile) {
    Serial.println("ERROR: Failed to create log file");
    return;
  }
  
  logFile.println("=== ESPTimeCast Netatmo Log Started ===");
  logFile.println("Time\tMessage");
  logFile.close();
  
  Serial.println("Logging system initialized");
}

void Logger::log(const String &message) {
  // Get current time since boot in milliseconds
  unsigned long now = millis();
  
  // Format: [HH:MM:SS.mmm] Message
  unsigned long ms = now % 1000;
  unsigned long s = (now / 1000) % 60;
  unsigned long m = (now / 60000) % 60;
  unsigned long h = (now / 3600000);
  
  char timeStr[16];
  sprintf(timeStr, "[%02lu:%02lu:%02lu.%03lu]", h, m, s, ms);
  
  String logEntry = String(timeStr) + " " + message;
  
  // Print to serial
  Serial.println(logEntry);
  
  // Store in memory buffer
  recentLogs.push_back(logEntry);
  if (recentLogs.size() > maxRecentLogs) {
    recentLogs.erase(recentLogs.begin());
  }
  
  // Check if it's time to flush logs to file
  if (now - lastFlushTime >= flushInterval) {
    flushToFile();
    lastFlushTime = now;
  }
}

void Logger::flushToFile() {
  if (recentLogs.empty()) {
    return;
  }
  
  File logFile = SPIFFS.open(logFilePath, FILE_APPEND);
  if (!logFile) {
    Serial.println("ERROR: Failed to open log file for writing");
    return;
  }
  
  for (const String &entry : recentLogs) {
    logFile.println(entry);
  }
  
  logFile.close();
}

String Logger::getRecentLogs() {
  String result = "";
  
  // First, flush any pending logs to file
  flushToFile();
  
  // Then read the entire log file
  File logFile = SPIFFS.open(logFilePath, FILE_READ);
  if (!logFile) {
    return "Error: Could not open log file";
  }
  
  while (logFile.available()) {
    result += logFile.readStringUntil('\n');
    result += "\n";
  }
  
  logFile.close();
  return result;
}

void Logger::clearLogs() {
  recentLogs.clear();
  SPIFFS.remove(logFilePath);
  
  // Create a new empty log file
  File logFile = SPIFFS.open(logFilePath, FILE_WRITE);
  if (logFile) {
    logFile.println("=== ESPTimeCast Netatmo Log Cleared ===");
    logFile.println("Time\tMessage");
    logFile.close();
  }
  
  lastFlushTime = millis();
}
