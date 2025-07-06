// Function to fetch indoor temperature from Netatmo
void fetchNetatmoIndoorTemperature() {
  Serial.println(F("\n[NETATMO] Fetching indoor temperature..."));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETATMO] Skipped: WiFi not connected"));
    netatmoIndoorTempAvailable = false;
    return;
  }
  
  if (strlen(netatmoDeviceId) == 0 || strlen(netatmoIndoorModuleId) == 0) {
    Serial.println(F("[NETATMO] Skipped: Missing device or indoor module ID"));
    Serial.print(F("[NETATMO] Device ID: '"));
    Serial.print(netatmoDeviceId);
    Serial.println(F("'"));
    Serial.print(F("[NETATMO] Indoor Module ID: '"));
    Serial.print(netatmoIndoorModuleId);
    Serial.println(F("'"));
    netatmoIndoorTempAvailable = false;
    return;
  }
  
  String token = getNetatmoToken();
  if (token.length() == 0) {
    Serial.println(F("[NETATMO] Skipped: No access token available"));
    netatmoIndoorTempAvailable = false;
    return;
  }
  
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Skip certificate validation
  
  HTTPClient https;
  
  String url = "https://api.netatmo.com/api/getmeasure?device_id=";
  url += netatmoDeviceId;
  url += "&module_id=";
  url += netatmoIndoorModuleId;
  url += "&scale=max&type=temperature&date_end=last";
  
  Serial.print(F("[NETATMO] GET "));
  Serial.println(url);
  
  if (https.begin(*client, url)) {
    https.addHeader("Authorization", "Bearer " + token);
    
    int httpCode = https.GET();
    Serial.print(F("[NETATMO] HTTP response code: "));
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      Serial.println(F("[NETATMO] Response:"));
      Serial.println(payload);
      
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if (doc.containsKey("body") && doc["body"].containsKey("0")) {
          JsonArray values = doc["body"]["0"];
          if (values.size() > 0) {
            float temp = values[0];
            netatmoIndoorTemp = temp;
            netatmoIndoorTempAvailable = true;
            Serial.print(F("[NETATMO] Indoor temperature: "));
            Serial.print(netatmoIndoorTemp);
            Serial.println(F("°C"));
            return;
          }
        }
        Serial.println(F("[NETATMO] Error: Temperature data not found in response"));
      } else {
        Serial.print(F("[NETATMO] JSON parse error: "));
        Serial.println(error.c_str());
      }
    } else {
      // Get the error response
      String errorPayload = https.getString();
      Serial.println(F("[NETATMO] Error response:"));
      Serial.println(errorPayload);
      
      // If we get a 403 error (Forbidden), the token is expired
      if (httpCode == 403) {
        Serial.println(F("[NETATMO] 403 Forbidden error - token expired"));
        forceNetatmoTokenRefresh();  // Force token refresh on next API call
      }
      
      netatmoIndoorTempAvailable = false;
    }
    
    https.end();
  } else {
    Serial.println(F("[NETATMO] Unable to connect to Netatmo API"));
    netatmoIndoorTempAvailable = false;
  }
}
