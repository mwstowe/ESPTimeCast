#include "NetatmoHandler.h"

NetatmoHandler::NetatmoHandler(AsyncWebServer* webServer, Config* configManager) : logger("Handler") {
  server = webServer;
  config = configManager;
  oauth = new NetatmoOAuth(config);
}

void NetatmoHandler::begin() {
  logger.log("Setting up Netatmo API endpoints");
  
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
  
  logger.log("Netatmo API endpoints configured");
}

void NetatmoHandler::handleSaveCredentials(AsyncWebServerRequest *request) {
  logger.log("Handling save credentials request");
  
  if (!request->hasParam("clientId", true) || !request->hasParam("clientSecret", true)) {
    logger.log("Error - Missing required parameters");
    request->send(400, "text/plain", "Missing required parameters");
    return;
  }
  
  String clientId = request->getParam("clientId", true)->value();
  String clientSecret = request->getParam("clientSecret", true)->value();
  
  logger.log("Saving Netatmo credentials");
  if (oauth->saveCredentials(clientId, clientSecret)) {
    logger.log("Credentials saved, redirecting to auth");
    request->redirect("/api/netatmo/auth");
  } else {
    logger.log("Error saving credentials");
    request->send(500, "text/plain", "Failed to save credentials");
  }
}

void NetatmoHandler::handleAuth(AsyncWebServerRequest *request) {
  logger.log("Handling auth request");
  
  if (!oauth->hasValidCredentials()) {
    logger.log("Error - No valid credentials");
    request->send(400, "text/plain", "No valid Netatmo credentials found");
    return;
  }
  
  String authUrl = oauth->getAuthorizationUrl();
  if (authUrl.isEmpty()) {
    logger.log("Error generating authorization URL");
    request->send(500, "text/plain", "Failed to generate authorization URL");
    return;
  }
  
  logger.log("Redirecting to Netatmo authorization page");
  logger.log("URL: " + authUrl);
  request->redirect(authUrl);
}

void NetatmoHandler::handleCallback(AsyncWebServerRequest *request) {
  logger.log("Handling OAuth callback");
  
  if (!request->hasParam("code")) {
    logger.log("Error - No authorization code in callback");
    if (request->hasParam("error")) {
      String error = request->getParam("error")->value();
      String errorDescription = request->hasParam("error_description") ? 
                               request->getParam("error_description")->value() : "Unknown error";
      logger.log("OAuth error: " + error + " - " + errorDescription);
    }
    request->send(400, "text/plain", "Authorization failed");
    return;
  }
  
  String code = request->getParam("code")->value();
  logger.log("Received authorization code, exchanging for tokens");
  
  if (oauth->exchangeAuthorizationCode(code)) {
    logger.log("Token exchange successful, redirecting to Netatmo page");
    request->redirect("/netatmo.html");
  } else {
    logger.log("Token exchange failed");
    request->send(500, "text/plain", "Failed to exchange authorization code for tokens");
  }
}

void NetatmoHandler::handleGetDevices(AsyncWebServerRequest *request) {
  logger.log("Handling get devices request");
  
  if (!oauth->hasValidTokens()) {
    logger.log("Error - No valid tokens");
    request->send(401, "application/json", "{\"error\":\"Not authenticated with Netatmo\"}");
    return;
  }
  
  // Check if token is expired and refresh if needed
  if (oauth->isTokenExpired()) {
    logger.log("Access token expired, refreshing");
    if (!oauth->refreshAccessToken()) {
      logger.log("Failed to refresh token");
      request->send(401, "application/json", "{\"error\":\"Failed to refresh access token\"}");
      return;
    }
    logger.log("Token refreshed successfully");
  }
  
  // Set up the HTTPS client
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  logger.log("Connecting to Netatmo API");
  if (!https.begin(*client, "https://api.netatmo.com/api/getstationsdata")) {
    logger.log("Error - Failed to connect to Netatmo API");
    request->send(500, "application/json", "{\"error\":\"Failed to connect to Netatmo API\"}");
    return;
  }
  
  String accessToken = config->getNetatmoAccessToken();
  https.addHeader("Authorization", "Bearer " + accessToken);
  
  logger.log("Sending request to Netatmo API");
  int httpCode = https.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    logger.log("Error - Netatmo API request failed with code: " + String(httpCode));
    String response = https.getString();
    logger.log("Response: " + response);
    https.end();
    request->send(httpCode, "application/json", "{\"error\":\"Failed to get devices from Netatmo\"}");
    return;
  }
  
  // Get the response
  logger.log("Netatmo API request successful");
  String response = https.getString();
  https.end();
  
  // Forward the response to the client
  request->send(200, "application/json", response);
}
