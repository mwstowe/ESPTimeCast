// Function to setup Netatmo OAuth handler
void setupNetatmoHandler() {
  Serial.println(F("[NETATMO] Setting up Netatmo OAuth handler..."));
  
  // Create Config instance
  Config* config = new Config();
  config->begin();
  
  // Create NetatmoHandler instance
  NetatmoHandler* netatmoHandler = new NetatmoHandler(&server, config);
  netatmoHandler->begin();
  
  Serial.println(F("[NETATMO] OAuth handler setup complete"));
}
