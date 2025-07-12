// Memory cleanup functions

// Function to clean up memory after API calls
void cleanupAfterAPICall() {
  Serial.println(F("[MEMORY] Cleaning up after API call"));
  
  // Force garbage collection
  forceGarbageCollection();
  
  // Defragment heap
  defragmentHeap();
  
  // Print memory stats
  printMemoryStats();
}

// Function to clean up memory before API calls
void prepareForAPICall() {
  Serial.println(F("[MEMORY] Preparing for API call"));
  
  // Defragment heap
  defragmentHeap();
  
  // Print memory stats
  printMemoryStats();
}

// Function to release unused memory
void releaseUnusedMemory() {
  // Allocate and free small blocks to trigger ESP8266 memory manager
  for (int i = 0; i < 10; i++) {
    void* p = malloc(128);
    if (p) {
      free(p);
    }
    yield();
  }
  
  // Reset heap to consolidate free blocks
  ESP.resetHeap();
}
