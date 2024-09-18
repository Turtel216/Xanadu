// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#ifndef xanadu_value_h
#define xanadu_value_h

#include "common.h"

// Macros for creating Xanadu values from C values
// These macros help in constructing Value objects for different types.

// Create a boolean Value
#define BOOL_VAL(value) ((Value){ VAL_BOOL, { .boolean = value } })
// Create a nil Value
#define NIL_VAL ((Value){ VAL_NIL, { .number = 0 } })
// Create a number Value
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
//##################################################

// Macros for extracting C values from Xanadu values
// These macros help in extracting the underlying data from Value objects.

// Extract a boolean from a Value
#define AS_BOOL(value) ((value).as.boolean)
// Extract a number from a Value
#define AS_NUMBER(value) ((value).as.number)
// Extract an object from a Value
#define AS_OBJ(value) ((value).as.obj)
//##################################################

// Macros for checking the type of a Value
// These macros help in determining the type of a Value object.

// Check if the Value is a boolean
#define IS_BOOL(value) ((value).type == VAL_BOOL)
// Check if the Value is nil
#define IS_NIL(value) ((value).type == VAL_NIL)
// Check if the Value is a number
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
// Check if the Value is an object
#define IS_OBJ(value) ((value).type == VAL_OBJ)
//##################################

// Macro for creating a Value from an object
// This macro helps in creating a Value that wraps an object.
#define OBJ_VAL(object) ((Value){ VAL_OBJ, { .obj = (Obj *)object } })

// Enum for representing the type of a Value
typedef enum {
	VAL_BOOL, // Boolean value
	VAL_NIL, // Nil value
	VAL_NUMBER, // Number value
	VAL_OBJ, // Object value
} ValueType;

// Forward declarations of object types
typedef struct Obj Obj;
typedef struct ObjString ObjString;

// Structure representing a Value in Xanadu VM
typedef struct {
	ValueType type; // Type of the value
	union {
		bool boolean; // Boolean value
		double number; // Numeric value
		Obj *obj; // Object pointer
	} as; // Union for holding the actual value
} Value;

// Structure representing an array of Values
typedef struct {
	int capacity; // Capacity of the array
	int count; // Number of values currently in the array
	Value *values; // Pointer to the array of Values
} ValueArray;

// Enum for representing the result of interpreting code
typedef enum {
	INTERPRET_OK, // Interpretation succeeded
	INTERPRET_COMPILE_ERROR, // Compilation error
	INTERPRET_RUNTIME_ERROR // Runtime error
} InterpretResult;

// Function prototypes

// Initialize a ValueArray with default values
void init_value_array(ValueArray *array);

// Add a new Value to the end of a ValueArray
// Parameters:
//   array - The ValueArray to which the value will be added
//   value - The Value to add
void write_value_array(ValueArray *array, Value value);

// Free memory allocated for a ValueArray
// Parameters:
//   array - The ValueArray to free
void free_value_array(ValueArray *array);

// Print a Value to the standard output
// Parameters:
//   value - The Value to print
void print_value(Value value);

// Check if two Values are equal
// Parameters:
//   a - The first Value
//   b - The second Value
// Returns:
//   true if the Values are equal, false otherwise
bool values_equal(Value a, Value b);

#endif
