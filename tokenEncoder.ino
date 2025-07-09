// Function to URL-encode the pipe character in the token
String encodeToken(const char* token) {
  String encodedToken = token;
  encodedToken.replace("|", "%7C");
  return encodedToken;
}

// Function to modify the fetchStationsDataImproved function to use URL-encoded tokens
void patchFetchStationsDataImproved() {
  // This function will be called from setup() to patch the fetchStationsDataImproved function
  // to use URL-encoded tokens in the Authorization header
  Serial.println(F("[NETATMO] Patching fetchStationsDataImproved to use URL-encoded tokens"));
  
  // We can't directly modify the function, but we can hook into it by modifying the token
  // before it's used in the Authorization header
  String originalToken = netatmoAccessToken;
  String encodedToken = encodeToken(netatmoAccessToken);
  
  // Log the original and encoded tokens
  Serial.println(F("[NETATMO] Original token: "));
  Serial.println(originalToken);
  Serial.println(F("[NETATMO] Encoded token: "));
  Serial.println(encodedToken);
  
  // Replace the token in memory with the encoded version
  strcpy(netatmoAccessToken, encodedToken.c_str());
  
  Serial.println(F("[NETATMO] Token patched successfully"));
}
