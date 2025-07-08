#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

class Logger {
private:
  const char* prefix;
  
public:
  Logger(const char* logPrefix = "ESPTimeCast");
  void log(const String &message);
  void log(const char* message);
};

#endif // LOGGER_H
