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
}

// Function to defragment the heap
void defragmentHeap() {
  Serial.println(F("[MEMORY] Starting heap defragmentation"));
  printMemoryStats();
  
  // Get current free heap
  uint32_t freeHeap = ESP.getFreeHeap();
  
  // Try to allocate a large block (80% of free heap)
  uint32_t blockSize = (freeHeap * 80) / 100;
  char* block = nullptr;
  
  // Try to allocate with decreasing sizes
  while (blockSize > 1024 && block == nullptr) {
    block = (char*)malloc(blockSize);
    if (block == nullptr) {
      blockSize = (blockSize * 90) / 100; // Reduce by 10%
    }
  }
  
  // If allocation succeeded, fill with pattern and free
  if (block != nullptr) {
    // Fill with pattern to ensure physical allocation
    memset(block, 0xAA, blockSize);
    free(block);
    Serial.print(F("[MEMORY] Defragmented "));
    Serial.print(blockSize);
    Serial.println(F(" bytes"));
  } else {
    Serial.println(F("[MEMORY] Failed to allocate block for defragmentation"));
  }
  
  // Force garbage collection
  forceGarbageCollection();
  
  Serial.println(F("[MEMORY] Heap defragmentation complete"));
  printMemoryStats();
}

// Function to force garbage collection
void forceGarbageCollection() {
  Serial.println(F("[MEMORY] Forcing garbage collection"));
  
  for (int i = 0; i < 10; i++) {
    void* p = malloc(128);
    if (p) free(p);
    yield(); // Allow system tasks to run
  }
}

// Function to check if defragmentation is needed
bool shouldDefragment() {
  int fragmentation = ESP.getHeapFragmentation();
  return (fragmentation > 50); // Defragment if fragmentation exceeds 50%
}
