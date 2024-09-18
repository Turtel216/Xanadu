// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#ifndef xanadu_object_h
#define xanadu_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "lookup_table.h"

// Macros for type-checking and type-casting objects in the VM

// Retrieve the type of the object from a Value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Check if a Value is a string object
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Check if a Value is a native object (C function)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

// Check if a Value is a closure object (function with upvalues)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)

// Convert a Value to an ObjString object
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))

// Convert a Value to a C-string (char array) from an ObjString
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

// Convert a Value to an ObjFunction object
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))

// Convert a Value to an ObjNative object
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)

// Convert a Value to an ObjClosure object
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))

// Check if a Value is a function object
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// Check if a Value is a class object
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)

// Convert a Value to an ObjClass object
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))

// Check if a Value is an instance of a class object
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)

// Convert a Value to an ObjInstance object
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))

// Check if a Value is a bound method object
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

// Convert a Value to an ObjBoundMethod object
#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))

// Enum for different object types in the VM
typedef enum {
	OBJ_STRING, // String object
	OBJ_FUNCTION, // Function object
	OBJ_NATIVE, // Native (C function) object
	OBJ_CLOSURE, // Closure object
	OBJ_UPVALUE, // Upvalue object
	OBJ_CLASS, // Class object
	OBJ_INSTANCE, // Instance of a class
	OBJ_BOUND_METHOD, // Bound method object
} ObjType;

// Base structure for all objects in the Xanadu VM
struct Obj {
	ObjType type; // Type of the object
	struct Obj *next; // Pointer to the next object in the list
	bool is_marked; // Flag indicating if the object is marked for garbage collection
};

// Object representing a function in the VM
typedef struct {
	Obj obj; // Base object structure
	int arity; // Number of arguments the function takes
	Chunk chunk; // Bytecode chunk representing the function's code
	ObjString *name; // Name of the function
	int upvalueCount; // Number of upvalues captured by the function
} ObjFunction;

// Type for native functions (C functions callable from the VM)
typedef Value (*NativeFn)(int argCount, Value *args);

// Object representing a native function (C function) in the VM
typedef struct {
	Obj obj; // Base object structure
	NativeFn function; // Pointer to the C function
} ObjNative;

// Object representing a string in the VM
struct ObjString {
	Obj obj; // Base object structure
	int length; // Length of the string
	char *chars; // Character array for the string
	uint32_t hash; // Hash value for the string
};

// Object representing an upvalue (closed-over variable) in the VM
typedef struct ObjUpvalue {
	Obj obj; // Base object structure
	Value *location; // Pointer to the stack location of the upvalue
	struct ObjUpvalue *next; // Pointer to the next upvalue in the list
	Value closed; // The value of the closed-over variable
} ObjUpvalue;

// Object representing a closure (function with captured upvalues) in the VM
typedef struct {
	Obj obj; // Base object structure
	ObjFunction *function; // The function that this closure wraps
	ObjUpvalue **upvalues; // Array of upvalues captured by the closure
	int upvalueCount; // Number of upvalues associated with the closure
} ObjClosure;

// Object representing a class in the VM
typedef struct {
	Obj obj; // Base object structure
	ObjString *name; // Name of the class
	Table methods; // Table of methods defined for the class
} ObjClass;

// Object representing an instance of a class in the VM
typedef struct {
	Obj obj; // Base object structure
	ObjClass *class_; // The class that this instance belongs to
	Table fields; // Table of fields for this instance
} ObjInstance;

// Object representing a bound method (method bound to an instance) in the VM
typedef struct {
	Obj obj; // Base object structure
	Value receiver; // The instance to which the method is bound
	ObjClosure *method; // The method (closure) being bound
} ObjBoundMethod;

// Check if a value is of a specific object type
// Parameters:
//   value - The value to check
//   type  - The type to compare against
// Returns:
//   true if the value is of the specified type; false otherwise
static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

// Create a new ObjString by copying the provided string
// Parameters:
//   chars  - The string to copy
//   length - Length of the string
// Returns:
//   A pointer to the newly created ObjString
ObjString *copy_string(const char *chars, int length);

// Print the type and value of an object to standard output
// Parameters:
//   value - The value to print
void print_object(Value value);

// Create a new ObjString by taking ownership of the provided string
// Parameters:
//   chars  - The string to take ownership of
//   length - Length of the string
// Returns:
//   A pointer to the newly created ObjString
ObjString *take_string(char *chars, int length);

// Create a new ObjFunction object
// Returns:
//   A pointer to the newly created ObjFunction
ObjFunction *new_function(void);

// Create a new ObjNative object
// Parameters:
//   function - The native function to wrap
// Returns:
//   A pointer to the newly created ObjNative
ObjNative *new_native(NativeFn function);

// Create a new ObjClosure object
// Parameters:
//   function - The function that the closure wraps
// Returns:
//   A pointer to the newly created ObjClosure
ObjClosure *new_closure(ObjFunction *function);

// Create a new ObjUpvalue object
// Parameters:
//   slot - Pointer to the stack location of the upvalue
// Returns:
//   A pointer to the newly created ObjUpvalue
ObjUpvalue *new_upvalue(Value *slot);

// Create a new ObjClass object
// Parameters:
//   name - The name of the class
// Returns:
//   A pointer to the newly created ObjClass
ObjClass *new_class(ObjString *name);

// Create a new ObjInstance object
// Parameters:
//   class_ - The class that the instance belongs to
// Returns:
//   A pointer to the newly created ObjInstance
ObjInstance *new_instance(ObjClass *class_);

// Create a new ObjBoundMethod object
// Parameters:
//   receiver - The instance to which the method is bound
//   method   - The method (closure) being bound
// Returns:
//   A pointer to the newly created ObjBoundMethod
ObjBoundMethod *new_bound_method(Value receiver, ObjClosure *method);

#endif
