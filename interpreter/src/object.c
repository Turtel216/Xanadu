// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#define ALLOCATE_OBJ(type, objectType) \
	(type *)allocate_object(sizeof(type), objectType)

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "lookup_table.h"

// Print a function object to the output stream
// Parameters:
//   function - The function object to print
// If the function does not have a name, "<script>" is printed.
static void print_function(ObjFunction *function)
{
	if (function->name == NULL) {
		printf("<script>");
		return;
	}
	printf("<fn %s>", function->name->chars);
}

// Print the representation of an object to the output stream
// Parameters:
//   value - The value representing the object to print
void print_object(Value value)
{
	switch (OBJ_TYPE(value)) {
	case OBJ_BOUND_METHOD:
		// Print bound method by delegating to function printer
		print_function(AS_BOUND_METHOD(value)->method->function);
		break;
	case OBJ_INSTANCE:
		// Print class name of the instance
		printf("%s instance", AS_INSTANCE(value)->class_->name->chars);
		break;
	case OBJ_CLASS:
		// Print class name
		printf("%s", AS_CLASS(value)->name->chars);
		break;
	case OBJ_STRING:
		// Print string contents
		printf("%s", AS_CSTRING(value));
		break;
	case OBJ_FUNCTION:
		// Print function object
		print_function(AS_FUNCTION(value));
		break;
	case OBJ_NATIVE:
		// Print native function placeholder
		printf("<native fn>");
		break;
	case OBJ_CLOSURE:
		// Print closure function
		print_function(AS_CLOSURE(value)->function);
		break;
	case OBJ_UPVALUE:
		// Print upvalue placeholder
		printf("upvalue");
		break;
	}
}

// Allocate a new object of a specified type and size
// Parameters:
//   size - The size of the object to allocate
//   type - The type of the object being allocated
// Returns:
//   A pointer to the newly allocated object
static Obj *allocate_object(size_t size, ObjType type)
{
	// Allocate memory for the new object
	Obj *object = (Obj *)reallocate(NULL, 0, size);
	object->type = type;
	object->next = vm.objects; // Link new object into the list
	object->is_marked = false; // Initial state: not marked for GC
	vm.objects = object; // Update the head of the list

#ifdef DEBUG_LOG_GC
	// Log the allocation if debugging GC
	printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif
	return object;
}

// Compute a hash value for a given string
// Parameters:
//   key    - The string to hash
//   length - The length of the string
// Returns:
//   The computed hash value
static uint32_t hash_string(const char *key, int length)
{
	uint32_t hash = 2166136261u; // FNV-1a initial hash value
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)key[i]; // XOR the byte with the hash
		hash *= 16777619; // Multiply by the FNV-1a prime
	}
	return hash;
}

// Allocate a new ObjString and initialize it
// Parameters:
//   chars - The character array for the string
//   length - The length of the string
//   hash - The precomputed hash value of the string
// Returns:
//   A pointer to the newly allocated ObjString
static ObjString *allocate_string(char *chars, int length, uint32_t hash)
{
	// Allocate and initialize a new string object
	ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	// Manage the object in the GC and insert into the hash table
	push(OBJ_VAL(string));
	insert_into_table(&vm.strings, string, NIL_VAL);
	pop();

	return string;
}

// Copy a string into a new ObjString and return it
// Parameters:
//   chars - The character array of the string to copy
//   length - The length of the string
// Returns:
//   A pointer to the new ObjString
ObjString *copy_string(const char *chars, int length)
{
	// Compute hash value of the string
	uint32_t hash = hash_string(chars, length);
	// Check if the string is already interned
	ObjString *interned =
		table_find_string(&vm.strings, chars, length, hash);

	if (interned != NULL)
		return interned; // Return existing string if found

	// Allocate memory for a new string and copy the contents
	char *heap_chars = ALLOCATE(char, length + 1);
	memcpy(heap_chars, chars, length);
	heap_chars[length] = '\0'; // Null-terminate the string

	// Allocate and return the new string object
	return allocate_string(heap_chars, length, hash);
}

// Create a new ObjString by taking ownership of an existing character array
// Parameters:
//   chars - The character array for the string (will be freed if already interned)
//   length - The length of the string
// Returns:
//   A pointer to the new ObjString
ObjString *take_string(char *chars, int length)
{
	// Compute hash value of the string
	uint32_t hash = hash_string(chars, length);

	// Check if the string is already interned
	ObjString *interned =
		table_find_string(&vm.strings, chars, length, hash);
	if (interned != NULL) {
		// Free the old memory and return the interned string
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	}

	// Allocate and return the new string object
	return allocate_string(chars, length, hash);
}

// Create a new ObjFunction object
// Returns:
//   A pointer to the newly created ObjFunction
ObjFunction *new_function(void)
{
	// Allocate and initialize a new function object
	ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0; // Default arity is 0
	function->upvalueCount = 0; // No upvalues initially
	function->name = NULL; // Function name is not set

	// Initialize the bytecode chunk for the function
	init_chunk(&function->chunk);
	return function;
}

// Create a new ObjNative object for a native function
// Parameters:
//   function - The native function pointer
// Returns:
//   A pointer to the newly created ObjNative
ObjNative *new_native(NativeFn function)
{
	ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->function = function; // Set the native function pointer
	return native;
}

// Create a new ObjClosure object
// Parameters:
//   function - The function that this closure wraps
// Returns:
//   A pointer to the newly created ObjClosure
ObjClosure *new_closure(ObjFunction *function)
{
	// Allocate and initialize upvalues
	ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalueCount);
	for (int i = 0; i < function->upvalueCount; i++) {
		upvalues[i] = NULL; // Upvalues are initially uninitialized
	}

	// Create and initialize the closure object
	ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalueCount = function->upvalueCount;
	return closure;
}

// Create a new ObjUpvalue object
// Parameters:
//   slot - The location of the upvalue on the stack
// Returns:
//   A pointer to the newly created ObjUpvalue
ObjUpvalue *new_upvalue(Value *slot)
{
	ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
	upvalue->location = slot; // Set the location of the upvalue
	upvalue->next = NULL; // No next upvalue initially
	upvalue->closed = NIL_VAL; // Initial closed value is NIL
	return upvalue;
}

// Create a new ObjClass object
// Parameters:
//   name - The name of the class
// Returns:
//   A pointer to the newly created ObjClass
ObjClass *new_class(ObjString *name)
{
	ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
	klass->name = name; // Set the class name
	init_table(&klass->methods); // Initialize the class methods table

	return klass;
}

// Create a new ObjInstance object
// Parameters:
//   class_ - The class to which this instance belongs
// Returns:
//   A pointer to the newly created ObjInstance
ObjInstance *new_instance(ObjClass *class_)
{
	ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
	instance->class_ = class_; // Set the class of the instance
	init_table(&instance->fields); // Initialize the instance fields table
	return instance;
}

// Create a new ObjBoundMethod object
// Parameters:
//   receiver - The instance that the method is bound to
//   method - The method (closure) being bound
// Returns:
//   A pointer to the newly created ObjBoundMethod
ObjBoundMethod *new_bound_method(Value receiver, ObjClosure *method)
{
	ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
	bound->receiver = receiver; // Set the bound instance
	bound->method = method; // Set the bound method (closure)
	return bound;
}
