#include "Logger.h"

Logger::Logger(const char* logPrefix) {
  prefix = logPrefix;
}

void Logger::log(const String &message) {
  // Get current time since boot in milliseconds
  unsigned long now = millis();
  
  // Format: [HH:MM:SS.mmm] Prefix: Message
  unsigned long ms = now % 1000;
  unsigned long s = (now / 1000) % 60;
  unsigned long m = (now / 60000) % 60;
  unsigned long h = (now / 3600000);
  
  char timeStr[16];
  sprintf(timeStr, "[%02lu:%02lu:%02lu.%03lu]", h, m, s, ms);
  
  Serial.print(timeStr);
  Serial.print(" ");
  Serial.print(prefix);
  Serial.print(": ");
  Serial.println(message);
}

void Logger::log(const char* message) {
  log(String(message));
}
