// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#ifndef xanadu_memory_h
#define xanadu_memory_h

#include "common.h"
#include "value.h"
#include "lookup_table.h"

// Macro to determine the new capacity for an array when expanding.
// The capacity is doubled unless it is less than 8, in which case it is set to 8.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// Macro to reallocate memory for an array, expanding or shrinking its size.
// It updates the array's memory allocation from an old size to a new size.
//
// Parameters:
//   type - The type of elements in the array
//   pointer - Pointer to the array to reallocate
//   oldCount - Previous number of elements in the array
//   newCount - New number of elements in the array
#define GROW_ARRAY(type, pointer, oldCount, newCount)          \
	(type *)reallocate(pointer, sizeof(type) * (oldCount), \
			   sizeof(type) * (newCount))

// Macro to free the memory allocated for an array.
// The memory for the array is deallocated based on its old size.
//
// Parameters:
//   type - The type of elements in the array
//   pointer - Pointer to the array to free
//   oldCount - Number of elements previously in the array
#define FREE_ARRAY(type, pointer, oldCount) \
	reallocate(pointer, sizeof(type) * (oldCount), 0)

// Macro to free memory allocated for a single object.
// Resizes the memory allocation to 0, effectively deallocating it.
//
// Parameters:
//   type - The type of the object
//   pointer - Pointer to the object to free
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// Macro to allocate memory for an array of a given type and count.
// This initializes the memory allocation for the specified number of elements.
//
// Parameters:
//   type - The type of elements in the array
//   count - Number of elements to allocate memory for
#define ALLOCATE(type, count) \
	(type *)reallocate(NULL, 0, sizeof(type) * (count))

// Function to reallocate memory for a given pointer.
// It resizes the allocated memory from oldSize to newSize.
//
// Parameters:
//   pointer - Pointer to the memory to reallocate
//   oldSize - Previous size of the allocated memory
//   newSize - New size of the allocated memory
//
// Returns:
//   A pointer to the reallocated memory
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

// Function to free the memory used by the list of objects.
// This is typically used to clean up memory used by objects in the VM.
void free_objects(void);

// Function to perform garbage collection for unused Xanadu variables.
// This function reclaims memory occupied by variables that are no longer in use.
void collect_garbage(void);

// Function to mark a Xanadu value for garbage collection.
// This function ensures that the value is not prematurely reclaimed by the garbage collector.
//
// Parameters:
//   value - The value to be marked
void mark_value(Value value);

// Function to mark a Xanadu object for garbage collection.
// This function ensures that the object is not prematurely reclaimed by the garbage collector.
//
// Parameters:
//   object - The object to be marked
void mark_object(Obj *object);

// Function to mark all global Xanadu variables for garbage collection.
// This function iterates through global variables and marks them to prevent premature collection.
//
// Parameters:
//   table - The table containing global variables to be marked
void mark_table(Table *table);

#endif
