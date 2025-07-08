#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <vector>
#include <SPIFFS.h>

class Logger {
private:
  const char* logFilePath = "/netatmo_log.txt";
  std::vector<String> recentLogs;
  const size_t maxRecentLogs = 50; // Keep last 50 logs in memory
  unsigned long lastFlushTime = 0;
  const unsigned long flushInterval = 60000; // Flush to file every minute
  
  void flushToFile();
  
public:
  Logger();
  void begin();
  void log(const String &message);
  String getRecentLogs();
  void clearLogs();
};

#endif // LOGGER_H
