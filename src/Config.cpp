#include "Config.h"

Config::Config() {
  // Constructor
}

void Config::begin() {
  if (!LittleFS.begin()) {
    Serial.println("ERROR: Failed to initialize LittleFS for config");
    return;
  }
  
  if (!loadConfigFile()) {
    Serial.println("Failed to load config file, using defaults");
    saveConfig(); // Create default config file
  }
}

bool Config::loadConfigFile() {
  if (!LittleFS.exists(configFilePath)) {
    Serial.println("Config file does not exist");
    return false;
  }
  
  File configFile = LittleFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return false;
  }
  
  size_t size = configFile.size();
  if (size > 2048) {
    Serial.println("Config file size is too large");
    configFile.close();
    return false;
  }
  
  // Allocate a buffer to store contents of the file
  std::unique_ptr<char[]> buf(new char[size + 1]);
  configFile.readBytes(buf.get(), size);
  buf[size] = '\0'; // Ensure null termination
  configFile.close();
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, buf.get());
  
  if (error) {
    Serial.print("Failed to parse config file: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Load WiFi settings
  if (doc.containsKey("ssid")) ssid = doc["ssid"].as<String>();
  if (doc.containsKey("password")) password = doc["password"].as<String>();
  if (doc.containsKey("mdnsHostname")) mdnsHostname = doc["mdnsHostname"].as<String>();
  
  // Load display settings
  if (doc.containsKey("brightness")) brightness = doc["brightness"].as<int>();
  if (doc.containsKey("tempAdjust")) tempAdjust = doc["tempAdjust"].as<int>();
  if (doc.containsKey("flipDisplay")) flipDisplay = doc["flipDisplay"].as<bool>();
  if (doc.containsKey("twelveHourToggle")) twelveHourToggle = doc["twelveHourToggle"].as<bool>();
  if (doc.containsKey("showDayOfWeek")) showDayOfWeek = doc["showDayOfWeek"].as<bool>();
  if (doc.containsKey("showIndoorTemp")) showIndoorTemp = doc["showIndoorTemp"].as<bool>();
  if (doc.containsKey("showOutdoorTemp")) showOutdoorTemp = doc["showOutdoorTemp"].as<bool>();
  
  // Load weather settings
  if (doc.containsKey("weatherCity")) weatherCity = doc["weatherCity"].as<String>();
  if (doc.containsKey("weatherApiKey")) weatherApiKey = doc["weatherApiKey"].as<String>();
  if (doc.containsKey("weatherUnits")) weatherUnits = doc["weatherUnits"].as<String>();
  if (doc.containsKey("timeZone")) timeZone = doc["timeZone"].as<String>();
  
  // Load Netatmo settings
  if (doc.containsKey("netatmoClientId")) netatmoClientId = doc["netatmoClientId"].as<String>();
  if (doc.containsKey("netatmoClientSecret")) netatmoClientSecret = doc["netatmoClientSecret"].as<String>();
  if (doc.containsKey("netatmoAccessToken")) netatmoAccessToken = doc["netatmoAccessToken"].as<String>();
  if (doc.containsKey("netatmoRefreshToken")) netatmoRefreshToken = doc["netatmoRefreshToken"].as<String>();
  if (doc.containsKey("netatmoTokenExpiration")) netatmoTokenExpiration = doc["netatmoTokenExpiration"].as<unsigned long>();
  if (doc.containsKey("netatmoDeviceId")) netatmoDeviceId = doc["netatmoDeviceId"].as<String>();
  if (doc.containsKey("netatmoModuleId")) netatmoModuleId = doc["netatmoModuleId"].as<String>();
  if (doc.containsKey("netatmoIndoorModuleId")) netatmoIndoorModuleId = doc["netatmoIndoorModuleId"].as<String>();
  if (doc.containsKey("useNetatmoOutdoor")) useNetatmoOutdoor = doc["useNetatmoOutdoor"].as<bool>();
  if (doc.containsKey("prioritizeNetatmoIndoor")) prioritizeNetatmoIndoor = doc["prioritizeNetatmoIndoor"].as<bool>();
  
  return true;
}

bool Config::saveConfig() {
  DynamicJsonDocument doc(2048);
  
  // Save WiFi settings
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["mdnsHostname"] = mdnsHostname;
  
  // Save display settings
  doc["brightness"] = brightness;
  doc["tempAdjust"] = tempAdjust;
  doc["flipDisplay"] = flipDisplay;
  doc["twelveHourToggle"] = twelveHourToggle;
  doc["showDayOfWeek"] = showDayOfWeek;
  doc["showIndoorTemp"] = showIndoorTemp;
  doc["showOutdoorTemp"] = showOutdoorTemp;
  
  // Save weather settings
  doc["weatherCity"] = weatherCity;
  doc["weatherApiKey"] = weatherApiKey;
  doc["weatherUnits"] = weatherUnits;
  doc["timeZone"] = timeZone;
  
  // Save Netatmo settings
  doc["netatmoClientId"] = netatmoClientId;
  doc["netatmoClientSecret"] = netatmoClientSecret;
  doc["netatmoAccessToken"] = netatmoAccessToken;
  doc["netatmoRefreshToken"] = netatmoRefreshToken;
  doc["netatmoTokenExpiration"] = netatmoTokenExpiration;
  doc["netatmoDeviceId"] = netatmoDeviceId;
  doc["netatmoModuleId"] = netatmoModuleId;
  doc["netatmoIndoorModuleId"] = netatmoIndoorModuleId;
  doc["useNetatmoOutdoor"] = useNetatmoOutdoor;
  doc["prioritizeNetatmoIndoor"] = prioritizeNetatmoIndoor;
  
  File configFile = LittleFS.open(configFilePath, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to config file");
    configFile.close();
    return false;
  }
  
  configFile.close();
  return true;
}

String Config::toJSON() {
  DynamicJsonDocument doc(2048);
  
  // Add all config values except password and sensitive tokens
  doc["ssid"] = ssid;
  doc["mdnsHostname"] = mdnsHostname;
  doc["brightness"] = brightness;
  doc["tempAdjust"] = tempAdjust;
  doc["flipDisplay"] = flipDisplay;
  doc["twelveHourToggle"] = twelveHourToggle;
  doc["showDayOfWeek"] = showDayOfWeek;
  doc["showIndoorTemp"] = showIndoorTemp;
  doc["showOutdoorTemp"] = showOutdoorTemp;
  doc["weatherCity"] = weatherCity;
  doc["weatherApiKey"] = weatherApiKey;
  doc["weatherUnits"] = weatherUnits;
  doc["timeZone"] = timeZone;
  doc["netatmoClientId"] = netatmoClientId;
  doc["netatmoClientSecret"] = netatmoClientSecret;
  doc["netatmoAccessToken"] = netatmoAccessToken ? "set" : "";
  doc["netatmoRefreshToken"] = netatmoRefreshToken ? "set" : "";
  doc["netatmoDeviceId"] = netatmoDeviceId;
  doc["netatmoModuleId"] = netatmoModuleId;
  doc["netatmoIndoorModuleId"] = netatmoIndoorModuleId;
  doc["useNetatmoOutdoor"] = useNetatmoOutdoor;
  doc["prioritizeNetatmoIndoor"] = prioritizeNetatmoIndoor;
  
  String output;
  serializeJson(doc, output);
  return output;
}
