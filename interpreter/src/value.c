// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "object.h"

// Initialize a ValueArray to an empty state.
// Parameters:
//   array - Pointer to the ValueArray to initialize.
void init_value_array(ValueArray *array)
{
	array->values = NULL; // Start with no allocated memory.
	array->capacity = 0; // No capacity initially.
	array->count = 0; // No elements initially.
}

// Append a Value to the end of a ValueArray.
// Expands the array if needed to accommodate the new value.
// Parameters:
//   array - Pointer to the ValueArray to which the value will be added.
//   value - The Value to append.
void write_value_array(ValueArray *array, Value value)
{
	// Check if the array needs to be resized.
	if (array->capacity < array->count + 1) {
		int oldCapacity = array->capacity; // Store old capacity.
		array->capacity =
			GROW_CAPACITY(oldCapacity); // Calculate new capacity.
		array->values =
			GROW_ARRAY(Value, array->values, oldCapacity,
				   array->capacity); // Allocate new memory.
	}

	// Add the new value and update the count.
	array->values[array->count] = value;
	array->count++;
}

// Free the memory allocated for a ValueArray and reinitialize it to an empty state.
// Parameters:
//   array - Pointer to the ValueArray to free.
void free_value_array(ValueArray *array)
{
	FREE_ARRAY(Value, array->values,
		   array->capacity); // Free the allocated memory.
	init_value_array(array); // Reinitialize to an empty state.
}

// Print a Value to the standard output (console).
// Parameters:
//   value - The Value to print.
void print_value(Value value)
{
	switch (value.type) {
	case VAL_BOOL:
		// Print "true" or "false" based on the boolean value.
		printf(AS_BOOL(value) ? "true" : "false");
		break;
	case VAL_NIL:
		// Print "nil" for nil values.
		printf("nil");
		break;
	case VAL_NUMBER:
		// Print the number value using default formatting.
		printf("%g", AS_NUMBER(value));
		break;
	case VAL_OBJ:
		// Print the object using its own print function.
		print_object(value);
		break;
	}
}

// Compare two Values for equality.
// Returns true if both Values are of the same type and have equal content.
// Parameters:
//   a - The first Value to compare.
//   b - The second Value to compare.
// Returns:
//   true if the Values are equal, false otherwise.
bool values_equal(Value a, Value b)
{
	// Check if the types of the two Values are the same.
	if (a.type != b.type)
		return false;

	// Compare values based on their type.
	switch (a.type) {
	case VAL_BOOL:
		// Compare boolean values.
		return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL:
		// Nil values are equal to each other.
		return true;
	case VAL_NUMBER:
		// Compare numeric values.
		return AS_NUMBER(a) == AS_NUMBER(b);
	case VAL_OBJ:
		// Compare object pointers (reference equality).
		return AS_OBJ(a) == AS_OBJ(b);
	default:
		return false; // Should never reach here, as all cases are covered.
	}
}
