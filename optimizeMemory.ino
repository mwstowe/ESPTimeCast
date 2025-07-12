// Memory optimization functions

// Function to optimize BearSSL client memory usage
void optimizeBearSSLClient(BearSSL::WiFiClientSecure* client) {
  // Set smaller buffer sizes to reduce memory usage
  client->setBufferSizes(512, 512);
  
  // Skip certificate validation to save memory
  client->setInsecure();
}

// Function to optimize HTTP client memory usage
void optimizeHTTPClient(HTTPClient& https) {
  // Set a reasonable timeout
  https.setTimeout(10000);
  
  // Disable chunked transfer encoding to save memory
  https.useHTTP10(true);
}

// Function to create a memory-efficient authorization header
void createAuthHeader(char* buffer, size_t bufferSize, const char* token) {
  snprintf(buffer, bufferSize, "Bearer %s", token);
}
