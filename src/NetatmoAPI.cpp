#include "NetatmoAPI.h"

NetatmoAPI::NetatmoAPI(AsyncWebServer* webServer, Config* configManager, Logger* logManager) {
  server = webServer;
  config = configManager;
  logger = logManager;
  oauth = new NetatmoOAuth(config, logger);
}

void NetatmoAPI::begin() {
  logger->log("API: Setting up Netatmo API endpoints");
  
  // Endpoint to save Netatmo credentials
  server->on("/api/netatmo/save-credentials", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->handleSaveCredentials(request);
  });
  
  // Endpoint to initiate OAuth flow
  server->on("/api/netatmo/auth", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleAuth(request);
  });
  
  // Endpoint to handle OAuth callback
  server->on("/api/netatmo/callback", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleCallback(request);
  });
  
  // Endpoint to get Netatmo devices
  server->on("/api/netatmo/devices", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetDevices(request);
  });
  
  // Endpoint to get modules for a device
  server->on("/api/netatmo/modules", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetModules(request);
  });
  
  // Endpoint to get logs
  server->on("/api/netatmo/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetLogs(request);
  });
  
  logger->log("API: Netatmo API endpoints configured");
}

void NetatmoAPI::handleSaveCredentials(AsyncWebServerRequest *request) {
  logger->log("API: Handling save credentials request");
  
  if (!request->hasParam("clientId", true) || !request->hasParam("clientSecret", true)) {
    logger->log("API: Error - Missing required parameters");
    request->send(400, "text/plain", "Missing required parameters");
    return;
  }
  
  String clientId = request->getParam("clientId", true)->value();
  String clientSecret = request->getParam("clientSecret", true)->value();
  
  logger->log("API: Saving Netatmo credentials");
  if (oauth->saveCredentials(clientId, clientSecret)) {
    logger->log("API: Credentials saved, redirecting to auth");
    request->redirect("/api/netatmo/auth");
  } else {
    logger->log("API: Error saving credentials");
    request->send(500, "text/plain", "Failed to save credentials");
  }
}

void NetatmoAPI::handleAuth(AsyncWebServerRequest *request) {
  logger->log("API: Handling auth request");
  
  if (!oauth->hasValidCredentials()) {
    logger->log("API: Error - No valid credentials");
    request->send(400, "text/plain", "No valid Netatmo credentials found");
    return;
  }
  
  String authUrl = oauth->getAuthorizationUrl();
  if (authUrl.isEmpty()) {
    logger->log("API: Error generating authorization URL");
    request->send(500, "text/plain", "Failed to generate authorization URL");
    return;
  }
  
  logger->log("API: Redirecting to Netatmo authorization page");
  request->redirect(authUrl);
}

void NetatmoAPI::handleCallback(AsyncWebServerRequest *request) {
  logger->log("API: Handling OAuth callback");
  
  if (!request->hasParam("code")) {
    logger->log("API: Error - No authorization code in callback");
    if (request->hasParam("error")) {
      String error = request->getParam("error")->value();
      String errorDescription = request->hasParam("error_description") ? 
                               request->getParam("error_description")->value() : "Unknown error";
      logger->log("API: OAuth error: " + error + " - " + errorDescription);
    }
    request->send(400, "text/plain", "Authorization failed");
    return;
  }
  
  String code = request->getParam("code")->value();
  logger->log("API: Received authorization code, exchanging for tokens");
  
  if (oauth->exchangeAuthorizationCode(code)) {
    logger->log("API: Token exchange successful, redirecting to Netatmo page");
    request->redirect("/netatmo.html");
  } else {
    logger->log("API: Token exchange failed");
    request->send(500, "text/plain", "Failed to exchange authorization code for tokens");
  }
}

void NetatmoAPI::handleGetDevices(AsyncWebServerRequest *request) {
  logger->log("API: Handling get devices request");
  
  if (!oauth->hasValidTokens()) {
    logger->log("API: Error - No valid tokens");
    request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
    return;
  }
  
  // Check if token is expired and refresh if needed
  if (oauth->isTokenExpired()) {
    logger->log("API: Access token expired, refreshing");
    if (!oauth->refreshAccessToken()) {
      logger->log("API: Failed to refresh token");
      request->send(401, "application/json", "{\"error\":\"Failed to refresh access token\"}");
      return;
    }
    logger->log("API: Token refreshed successfully");
  }
  
  // Set up the HTTPS client
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  logger->log("API: Connecting to Netatmo API");
  if (!https.begin(client, "https://api.netatmo.com/api/getstationsdata")) {
    logger->log("API: Error - Failed to connect to Netatmo API");
    request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API\"}");
    return;
  }
  
  String accessToken = config->getNetatmoAccessToken();
  https.addHeader("Authorization", "Bearer " + accessToken);
  
  logger->log("API: Sending request to Netatmo API");
  int httpCode = https.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    logger->log("API: Error - Netatmo API request failed with code: " + String(httpCode));
    String response = https.getString();
    logger->log("API: Response: " + response);
    https.end();
    request->send(httpCode, "application/json", "{\"error\":\"Failed to get devices from Netatmo\"}");
    return;
  }
  
  // Get the response
  logger->log("API: Netatmo API request successful");
  String response = https.getString();
  https.end();
  
  // Forward the response to the client
  request->send(200, "application/json", response);
}

void NetatmoAPI::handleGetModules(AsyncWebServerRequest *request) {
  logger->log("API: Handling get modules request");
  
  if (!request->hasParam("deviceId")) {
    logger->log("API: Error - Missing deviceId parameter");
    request->send(400, "application/json", "{\"error\":\"Missing deviceId parameter\"}");
    return;
  }
  
  String deviceId = request->getParam("deviceId")->value();
  logger->log("API: Getting modules for device: " + deviceId);
  
  if (!oauth->hasValidTokens()) {
    logger->log("API: Error - No valid tokens");
    request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
    return;
  }
  
  // Check if token is expired and refresh if needed
  if (oauth->isTokenExpired()) {
    logger->log("API: Access token expired, refreshing");
    if (!oauth->refreshAccessToken()) {
      logger->log("API: Failed to refresh token");
      request->send(401, "application/json", "{\"error\":\"Failed to refresh access token\"}");
      return;
    }
    logger->log("API: Token refreshed successfully");
  }
  
  // Set up the HTTPS client
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  logger->log("API: Connecting to Netatmo API");
  if (!https.begin(client, "https://api.netatmo.com/api/getstationsdata?device_id=" + deviceId)) {
    logger->log("API: Error - Failed to connect to Netatmo API");
    request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API\"}");
    return;
  }
  
  String accessToken = config->getNetatmoAccessToken();
  https.addHeader("Authorization", "Bearer " + accessToken);
  
  logger->log("API: Sending request to Netatmo API");
  int httpCode = https.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    logger->log("API: Error - Netatmo API request failed with code: " + String(httpCode));
    String response = https.getString();
    logger->log("API: Response: " + response);
    https.end();
    request->send(httpCode, "application/json", "{\"error\":\"Failed to get modules from Netatmo\"}");
    return;
  }
  
  // Get the response
  logger->log("API: Netatmo API request successful");
  String response = https.getString();
  https.end();
  
  // Forward the response to the client
  request->send(200, "application/json", response);
}

void NetatmoAPI::handleGetLogs(AsyncWebServerRequest *request) {
  logger->log("API: Handling get logs request");
  String logs = logger->getRecentLogs();
  request->send(200, "text/plain", logs);
}
