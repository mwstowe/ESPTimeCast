#include "NetatmoOAuth.h"

NetatmoOAuth::NetatmoOAuth(Config* configManager) : logger("OAuth") {
  config = configManager;
}

String NetatmoOAuth::urlEncode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String result = "";
  result.reserve(input.length() * 3); // Reserve memory to avoid reallocations
  
  for (size_t i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      result += c;
    } else {
      result += '%';
      result += hex[c >> 4];
      result += hex[c & 0xF];
    }
  }
  return result;
}

bool NetatmoOAuth::saveCredentials(const String &clientId, const String &clientSecret) {
  logger.log("Saving Netatmo credentials");
  
  if (clientId.isEmpty() || clientSecret.isEmpty()) {
    logger.log("Error - Client ID or Secret is empty");
    return false;
  }
  
  config->setNetatmoClientId(clientId);
  config->setNetatmoClientSecret(clientSecret);
  bool success = config->saveConfig();
  
  if (success) {
    logger.log("Credentials saved successfully");
  } else {
    logger.log("Failed to save credentials");
  }
  
  return success;
}

String NetatmoOAuth::getAuthorizationUrl() {
  logger.log("Generating authorization URL");
  
  String clientId = config->getNetatmoClientId();
  if (clientId.isEmpty()) {
    logger.log("Error - Client ID not set");
    return "";
  }
  
  // Use a minimal redirect URI on the ESP8266
  String redirectUri = "http://" + WiFi.localIP().toString() + "/api/netatmo/callback";
  logger.log("Redirect URI: " + redirectUri);
  
  // Construct the authorization URL with minimal scope for weather station data
  String authUrl = "https://api.netatmo.com/oauth2/authorize";
  authUrl += "?client_id=" + urlEncode(clientId);
  authUrl += "&redirect_uri=" + urlEncode(redirectUri);
  authUrl += "&scope=" + urlEncode("read_station");
  authUrl += "&response_type=code";
  
  logger.log("Authorization URL generated");
  return authUrl;
}

bool NetatmoOAuth::exchangeAuthorizationCode(const String &code) {
  logger.log("Exchanging authorization code for tokens");
  
  if (code.isEmpty()) {
    logger.log("Error - Authorization code is empty");
    return false;
  }
  
  String clientId = config->getNetatmoClientId();
  String clientSecret = config->getNetatmoClientSecret();
  
  if (clientId.isEmpty() || clientSecret.isEmpty()) {
    logger.log("Error - Client credentials not set");
    return false;
  }
  
  // Prepare for the token exchange request
  logger.log("Preparing token exchange request");
  
  // Use a minimal redirect URI on the ESP8266
  String redirectUri = "http://" + WiFi.localIP().toString() + "/api/netatmo/callback";
  
  // Construct the POST data
  String postData = "grant_type=authorization_code";
  postData += "&client_id=" + urlEncode(clientId);
  postData += "&client_secret=" + urlEncode(clientSecret);
  postData += "&code=" + urlEncode(code);
  postData += "&redirect_uri=" + urlEncode(redirectUri);
  
  // Set up the HTTPS client
  logger.log("Setting up HTTPS client");
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  logger.log("Connecting to Netatmo token endpoint");
  if (!https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    logger.log("Error - Failed to connect to token endpoint");
    return false;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Make the POST request
  logger.log("Sending token exchange request");
  int httpCode = https.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    logger.log("Error - Token exchange failed with code: " + String(httpCode));
    if (httpCode > 0) {
      logger.log("Response: " + https.getString());
    }
    https.end();
    return false;
  }
  
  // Get the response
  logger.log("Token exchange successful, parsing response");
  String response = https.getString();
  https.end();
  
  // Parse the response and save tokens
  return parseTokenResponse(response);
}

bool NetatmoOAuth::parseTokenResponse(const String &response) {
  logger.log("Parsing token response");
  
  // Use ArduinoJson to parse the response
  // Calculate the capacity based on expected response size
  const size_t capacity = JSON_OBJECT_SIZE(6) + 500;
  
  // Use DynamicJsonDocument for parsing
  DynamicJsonDocument doc(capacity);
  
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    logger.log("Error parsing JSON: " + String(error.c_str()));
    return false;
  }
  
  // Extract the tokens
  if (!doc.containsKey("access_token") || !doc.containsKey("refresh_token")) {
    logger.log("Error - Response missing required tokens");
    return false;
  }
  
  String accessToken = doc["access_token"].as<String>();
  String refreshToken = doc["refresh_token"].as<String>();
  long expiresIn = doc["expires_in"].as<long>();
  
  // Calculate expiration time
  unsigned long expirationTime = millis() + (expiresIn * 1000);
  
  // Save the tokens
  logger.log("Saving tokens to config");
  config->setNetatmoAccessToken(accessToken);
  config->setNetatmoRefreshToken(refreshToken);
  config->setNetatmoTokenExpiration(expirationTime);
  
  bool success = config->saveConfig();
  if (success) {
    logger.log("Tokens saved successfully");
  } else {
    logger.log("Failed to save tokens");
  }
  
  return success;
}

bool NetatmoOAuth::refreshAccessToken() {
  logger.log("Refreshing access token");
  
  String clientId = config->getNetatmoClientId();
  String clientSecret = config->getNetatmoClientSecret();
  String refreshToken = config->getNetatmoRefreshToken();
  
  if (clientId.isEmpty() || clientSecret.isEmpty() || refreshToken.isEmpty()) {
    logger.log("Error - Missing credentials for token refresh");
    return false;
  }
  
  // Construct the POST data
  String postData = "grant_type=refresh_token";
  postData += "&client_id=" + urlEncode(clientId);
  postData += "&client_secret=" + urlEncode(clientSecret);
  postData += "&refresh_token=" + urlEncode(refreshToken);
  
  // Set up the HTTPS client
  logger.log("Setting up HTTPS client for token refresh");
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation to save memory
  
  HTTPClient https;
  https.setTimeout(10000); // 10 second timeout
  
  logger.log("Connecting to Netatmo token endpoint");
  if (!https.begin(*client, "https://api.netatmo.com/oauth2/token")) {
    logger.log("Error - Failed to connect to token endpoint");
    return false;
  }
  
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Make the POST request
  logger.log("Sending token refresh request");
  int httpCode = https.POST(postData);
  
  if (httpCode != HTTP_CODE_OK) {
    logger.log("Error - Token refresh failed with code: " + String(httpCode));
    if (httpCode > 0) {
      logger.log("Response: " + https.getString());
    }
    https.end();
    return false;
  }
  
  // Get the response
  logger.log("Token refresh successful, parsing response");
  String response = https.getString();
  https.end();
  
  // Parse the response and save tokens
  return parseTokenResponse(response);
}

bool NetatmoOAuth::hasValidCredentials() {
  String clientId = config->getNetatmoClientId();
  String clientSecret = config->getNetatmoClientSecret();
  return !clientId.isEmpty() && !clientSecret.isEmpty();
}

bool NetatmoOAuth::hasValidTokens() {
  String accessToken = config->getNetatmoAccessToken();
  String refreshToken = config->getNetatmoRefreshToken();
  return !accessToken.isEmpty() && !refreshToken.isEmpty();
}

bool NetatmoOAuth::isTokenExpired() {
  unsigned long expirationTime = config->getNetatmoTokenExpiration();
  // Consider token expired if within 5 minutes of expiration
  return (expirationTime - millis()) < (5 * 60 * 1000);
}
