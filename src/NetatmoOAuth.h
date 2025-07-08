#ifndef NETATMO_OAUTH_H
#define NETATMO_OAUTH_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Logger.h"

class NetatmoOAuth {
private:
  Config* config;
  Logger logger;
  
  // Memory-efficient string handling
  String urlEncode(const String &input);
  bool parseTokenResponse(const String &response);
  
public:
  NetatmoOAuth(Config* configManager);
  
  // OAuth flow handlers
  bool saveCredentials(const String &clientId, const String &clientSecret);
  String getAuthorizationUrl();
  bool exchangeAuthorizationCode(const String &code);
  bool refreshAccessToken();
  
  // Status checks
  bool hasValidCredentials();
  bool hasValidTokens();
  bool isTokenExpired();
};

#endif // NETATMO_OAUTH_H
