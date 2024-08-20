#ifndef xanadu_memory_h
#define xanadu_memory_h

#include "common.h"

// Macro for expending an arrays capacity
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// Macro for expending an arrays size
#define GROW_ARRAY(type, pointer, oldCount, newCount)          \
	(type *)reallocate(pointer, sizeof(type) * (oldCount), \
			   sizeof(type) * (newCount))

// Macro for freeing an array
#define FREE_ARRAY(type, pointer, oldCount) \
	reallocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, count) \
	(type *)reallocate(NULL, 0, sizeof(type) * (count))

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif
