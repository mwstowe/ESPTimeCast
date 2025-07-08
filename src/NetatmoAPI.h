#ifndef NETATMO_API_H
#define NETATMO_API_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Config.h"
#include "Logger.h"
#include "NetatmoOAuth.h"

class NetatmoAPI {
private:
  AsyncWebServer* server;
  Config* config;
  Logger* logger;
  NetatmoOAuth* oauth;
  
public:
  NetatmoAPI(AsyncWebServer* webServer, Config* configManager, Logger* logManager);
  void begin();
  
private:
  // API endpoint handlers
  void handleSaveCredentials(AsyncWebServerRequest *request);
  void handleAuth(AsyncWebServerRequest *request);
  void handleCallback(AsyncWebServerRequest *request);
  void handleGetDevices(AsyncWebServerRequest *request);
  void handleGetModules(AsyncWebServerRequest *request);
  void handleGetLogs(AsyncWebServerRequest *request);
};

#endif // NETATMO_API_H
