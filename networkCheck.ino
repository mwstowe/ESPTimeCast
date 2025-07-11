// Function to check network connectivity
bool checkNetworkConnectivity() {
  Serial.println(F("[NETWORK] Checking network connectivity..."));
  
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[NETWORK] WiFi not connected"));
    return false;
  }
  
  // Print WiFi information
  Serial.print(F("[NETWORK] WiFi connected to: "));
  Serial.println(WiFi.SSID());
  Serial.print(F("[NETWORK] IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("[NETWORK] Signal strength: "));
  Serial.print(WiFi.RSSI());
  Serial.println(F(" dBm"));
  
  // Try to resolve a domain name
  Serial.print(F("[NETWORK] Resolving api.netatmo.com... "));
  IPAddress netatmoIp;
  bool dnsResult = WiFi.hostByName("api.netatmo.com", netatmoIp);
  
  if (dnsResult) {
    Serial.print(F("Success: "));
    Serial.println(netatmoIp.toString());
  } else {
    Serial.println(F("Failed"));
  }
  
  return dnsResult;
}
