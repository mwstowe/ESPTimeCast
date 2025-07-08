#ifndef NETATMO_HANDLER_H
#define NETATMO_HANDLER_H

#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "Config.h"
#include "Logger.h"
#include "NetatmoOAuth.h"

class NetatmoHandler {
private:
  AsyncWebServer* server;
  Config* config;
  Logger logger;
  NetatmoOAuth* oauth;
  
public:
  NetatmoHandler(AsyncWebServer* webServer, Config* configManager);
  void begin();
  
private:
  // Minimal API endpoint handlers
  void handleSaveCredentials(AsyncWebServerRequest *request);
  void handleAuth(AsyncWebServerRequest *request);
  void handleCallback(AsyncWebServerRequest *request);
  void handleGetDevices(AsyncWebServerRequest *request);
};

#endif // NETATMO_HANDLER_H
