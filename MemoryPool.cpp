// MemoryPool.cpp
#include "MemoryPool.h"

// Initialize global memory pools
MemoryPool<256, 8> smallPool;
MemoryPool<512, 4> mediumPool;
MemoryPool<1024, 2> largePool;