#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>  // Use LittleFS instead of SPIFFS for ESP8266

class Config {
private:
  const char* configFilePath = "/config.json";
  
  // WiFi settings
  String ssid;
  String password;
  String mdnsHostname = "esptime";
  
  // Display settings
  int brightness = 50;
  int tempAdjust = 0;
  bool flipDisplay = false;
  bool twelveHourToggle = false;
  bool showDayOfWeek = true;
  bool showIndoorTemp = true;
  bool showOutdoorTemp = true;
  
  // Weather settings
  String weatherCity;
  String weatherApiKey;
  String weatherUnits = "metric";
  String timeZone;
  
  // Netatmo settings
  String netatmoClientId;
  String netatmoClientSecret;
  String netatmoAccessToken;
  String netatmoRefreshToken;
  unsigned long netatmoTokenExpiration = 0;
  String netatmoDeviceId;
  String netatmoModuleId;
  String netatmoIndoorModuleId;
  bool useNetatmoOutdoor = false;
  bool prioritizeNetatmoIndoor = false;
  
  bool loadConfigFile();
  
public:
  Config();
  void begin();
  bool saveConfig();
  
  // Getters
  String getSSID() { return ssid; }
  String getPassword() { return password; }
  String getMDNSHostname() { return mdnsHostname; }
  int getBrightness() { return brightness; }
  int getTempAdjust() { return tempAdjust; }
  bool getFlipDisplay() { return flipDisplay; }
  bool getTwelveHourToggle() { return twelveHourToggle; }
  bool getShowDayOfWeek() { return showDayOfWeek; }
  bool getShowIndoorTemp() { return showIndoorTemp; }
  bool getShowOutdoorTemp() { return showOutdoorTemp; }
  String getWeatherCity() { return weatherCity; }
  String getWeatherApiKey() { return weatherApiKey; }
  String getWeatherUnits() { return weatherUnits; }
  String getTimeZone() { return timeZone; }
  String getNetatmoClientId() { return netatmoClientId; }
  String getNetatmoClientSecret() { return netatmoClientSecret; }
  String getNetatmoAccessToken() { return netatmoAccessToken; }
  String getNetatmoRefreshToken() { return netatmoRefreshToken; }
  unsigned long getNetatmoTokenExpiration() { return netatmoTokenExpiration; }
  String getNetatmoDeviceId() { return netatmoDeviceId; }
  String getNetatmoModuleId() { return netatmoModuleId; }
  String getNetatmoIndoorModuleId() { return netatmoIndoorModuleId; }
  bool getUseNetatmoOutdoor() { return useNetatmoOutdoor; }
  bool getPrioritizeNetatmoIndoor() { return prioritizeNetatmoIndoor; }
  
  // Setters
  void setSSID(const String &value) { ssid = value; }
  void setPassword(const String &value) { password = value; }
  void setMDNSHostname(const String &value) { mdnsHostname = value; }
  void setBrightness(int value) { brightness = value; }
  void setTempAdjust(int value) { tempAdjust = value; }
  void setFlipDisplay(bool value) { flipDisplay = value; }
  void setTwelveHourToggle(bool value) { twelveHourToggle = value; }
  void setShowDayOfWeek(bool value) { showDayOfWeek = value; }
  void setShowIndoorTemp(bool value) { showIndoorTemp = value; }
  void setShowOutdoorTemp(bool value) { showOutdoorTemp = value; }
  void setWeatherCity(const String &value) { weatherCity = value; }
  void setWeatherApiKey(const String &value) { weatherApiKey = value; }
  void setWeatherUnits(const String &value) { weatherUnits = value; }
  void setTimeZone(const String &value) { timeZone = value; }
  void setNetatmoClientId(const String &value) { netatmoClientId = value; }
  void setNetatmoClientSecret(const String &value) { netatmoClientSecret = value; }
  void setNetatmoAccessToken(const String &value) { netatmoAccessToken = value; }
  void setNetatmoRefreshToken(const String &value) { netatmoRefreshToken = value; }
  void setNetatmoTokenExpiration(unsigned long value) { netatmoTokenExpiration = value; }
  void setNetatmoDeviceId(const String &value) { netatmoDeviceId = value; }
  void setNetatmoModuleId(const String &value) { netatmoModuleId = value; }
  void setNetatmoIndoorModuleId(const String &value) { netatmoIndoorModuleId = value; }
  void setUseNetatmoOutdoor(bool value) { useNetatmoOutdoor = value; }
  void setPrioritizeNetatmoIndoor(bool value) { prioritizeNetatmoIndoor = value; }
  
  // JSON serialization
  String toJSON();
};

#endif // CONFIG_H
