// Memory defragmentation functions

// Function to print memory statistics
void printMemoryStats() {
  Serial.print(F("[MEMORY] Free heap: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  
  Serial.print(F("[MEMORY] Heap fragmentation: "));
  Serial.print(ESP.getHeapFragmentation());
  Serial.println(F("%"));
  
  Serial.print(F("[MEMORY] Largest free block: "));
  Serial.print(ESP.getMaxFreeBlockSize());
  Serial.println(F(" bytes"));
  
  // Add stack information
  Serial.print(F("[MEMORY] Free stack: "));
  Serial.print(ESP.getFreeContStack());
  Serial.println(F(" bytes"));
}

// Function to defragment the heap with safer yield handling
void defragmentHeap() {
  Serial.println(F("[MEMORY] Starting heap defragmentation"));
  printMemoryStats();
  
  // Get current free heap
  uint32_t freeHeap = ESP.getFreeHeap();
  
  // Try to allocate a large block (70% of free heap) - reduced from 80% to be safer
  uint32_t blockSize = (freeHeap * 70) / 100;
  char* block = nullptr;
  
  // Try to allocate with decreasing sizes
  while (blockSize > 1024 && block == nullptr) {
    block = (char*)malloc(blockSize);
    if (block == nullptr) {
      blockSize = (blockSize * 90) / 100; // Reduce by 10%
    }
    
    // Use delay instead of yield to avoid potential yield-related crashes
    delay(1);
  }
  
  // If allocation succeeded, fill with pattern and free
  if (block != nullptr) {
    // Fill with pattern to ensure physical allocation
    // Use smaller chunks to avoid blocking for too long
    const size_t chunkSize = 1024;
    for (size_t i = 0; i < blockSize; i += chunkSize) {
      size_t fillSize = min(chunkSize, blockSize - i);
      memset(block + i, 0xAA, fillSize);
      
      // Use delay instead of yield
      delay(1);
    }
    
    free(block);
    Serial.print(F("[MEMORY] Defragmented "));
    Serial.print(blockSize);
    Serial.println(F(" bytes"));
  } else {
    Serial.println(F("[MEMORY] Failed to allocate block for defragmentation"));
  }
  
  // Force garbage collection with safer approach
  safeGarbageCollection();
  
  Serial.println(F("[MEMORY] Heap defragmentation complete"));
  printMemoryStats();
}

// Function to force garbage collection with safer yield handling
void forceGarbageCollection() {
  Serial.println(F("[MEMORY] Forcing garbage collection"));
  ESP.resetHeap();
}
  Serial.println(F("[MEMORY] Forcing garbage collection"));
  
  // Allocate and free small blocks with delays instead of yields
  for (int i = 0; i < 5; i++) {
    void* p = malloc(128);
    if (p) {
      // Touch the memory to ensure it's physically allocated
      memset(p, 0, 128);
      free(p);
    }
    // Use delay instead of yield
    delay(5);
  }
  
  // One final delay to allow system tasks
  delay(10);
}

// Function to check if defragmentation is needed
bool shouldDefragment() {
  int fragmentation = ESP.getHeapFragmentation();
  int freeStack = ESP.getFreeContStack();
  
  // Check both fragmentation and stack space
  return (fragmentation > 50 || freeStack < 2048);
}
// Safe garbage collection function
void safeGarbageCollection() {
  Serial.println(F("[MEMORY] Forcing garbage collection"));
  ESP.resetHeap();
}
