// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#ifndef xanadu_memory_h
#define xanadu_memory_h

#include "common.h"
#include "value.h"
#include "lookup_table.h"

// Macro for expending an arrays capacity
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// Macro for expending an arrays size
#define GROW_ARRAY(type, pointer, oldCount, newCount)          \
	(type *)reallocate(pointer, sizeof(type) * (oldCount), \
			   sizeof(type) * (newCount))

// Macro for freeing an array
#define FREE_ARRAY(type, pointer, oldCount) \
	reallocate(pointer, sizeof(type) * (oldCount), 0)

// Resize allocated memory to 0
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define ALLOCATE(type, count) \
	(type *)reallocate(NULL, 0, sizeof(type) * (count))

// Realocate given type
void *reallocate(void *pointer, size_t oldSize, size_t newSize);
// Free memory for object list
void free_objects(void);

// Free unused xanadu variables
void collect_garbage(void);

// Mark a xanadu value for garbage collection
void mark_value(Value value);

// Mark a xanadu object for garbage collection
void mark_object(Obj *object);

// Mark global xanadu variables
void mark_table(Table *table);

#endif
