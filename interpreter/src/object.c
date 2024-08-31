// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

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

// Print function types to iostream
static void print_function(ObjFunction *function)
{
	if (function->name == NULL) {
		printf("<script>");
		return;
	}
	printf("<fn %s>", function->name->chars);
}

// Print object types to iostream
void print_object(Value value)
{
	switch (OBJ_TYPE(value)) {
	case OBJ_INSTANCE:
		printf("%s instance", AS_INSTANCE(value)->class_->name->chars);
		break;
	case OBJ_CLASS:
		printf("%s", AS_CLASS(value)->name->chars);
		break;
	case OBJ_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	case OBJ_FUNCTION:
		print_function(AS_FUNCTION(value));
		break;
	case OBJ_NATIVE:
		printf("<native fn>");
		break;
	case OBJ_CLOSURE:
		print_function(AS_CLOSURE(value)->function);
		break;
	case OBJ_UPVALUE:
		printf("upvalue");
		break;
	}
}

// Allocate new Object with specified type and size
static Obj *allocate_object(size_t size, ObjType type)
{
	Obj *object = (Obj *)reallocate(NULL, 0, size);
	object->type = type;
	object->next = vm.objects;
	object->is_marked = false;
	vm.objects = object;

#ifdef DEBUG_LOG_GC
	printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif
	return object;
}

// Calcute hash valye of string
static uint32_t hash_string(const char *key, int length)
{
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)key[i];
		hash *= 16777619;
	}
	return hash;
}

// Alocate new ObjString and return said object
static ObjString *allocate_string(char *chars, int length, uint32_t hash)
{
	// Create new String object
	ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	push(OBJ_VAL(string));

	// Insert object into hash table
	insert_into_table(&vm.strings, string, NIL_VAL);

	pop();

	return string;
}

// Copy string to new ObjString and return new object
ObjString *copy_string(const char *chars, int length)
{
	// Calculate hash value
	uint32_t hash = hash_string(chars, length);
	ObjString *interned =
		table_find_string(&vm.strings, chars, length, hash);

	// Notify user since string not found on the hash table
	if (interned != NULL)
		return interned;

	// Alocate and copy memory
	char *heap_chars = ALLOCATE(char, length + 1);
	memcpy(heap_chars, chars, length);
	heap_chars[length] = '\0';
	// Allocate new string and return String object
	return allocate_string(heap_chars, length, hash);
}

// Create a string object
ObjString *take_string(char *chars, int length)
{
	// Calculate hash value
	uint32_t hash = hash_string(chars, length);

	// Insert new string into hash table
	ObjString *interned =
		table_find_string(&vm.strings, chars, length, hash);
	if (interned != NULL) {
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	}

	// Allocate and return new String object
	return allocate_string(chars, length, hash);
}

// Create a function object
ObjFunction *new_function(void)
{
	// Create new function object
	ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->upvalueCount = 0;
	function->name = NULL;

	// Initialize byte code data and return object
	init_chunk(&function->chunk);
	return function;
}

// Create a native object
ObjNative *new_native(NativeFn function)
{
	ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->function = function;
	return native;
}

// Create a closure object
ObjClosure *new_closure(ObjFunction *function)
{
	// Create Upvalue object and initialize upvalues
	ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalueCount);
	for (int i = 0; i < function->upvalueCount; i++) {
		upvalues[i] = NULL;
	}

	// Create closure object
	ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalueCount = function->upvalueCount;
	return closure;
}

// Create a upvalue obejct
ObjUpvalue *new_upvalue(Value *slot)
{
	ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
	upvalue->location = slot;
	upvalue->next = NULL;
	upvalue->closed = NIL_VAL;
	return upvalue;
}

// Create a Class Object
ObjClass *new_class(ObjString *name)
{
	ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
	klass->name = name;
	init_table(&klass->methods);

	return klass;
}

ObjInstance *new_instance(ObjClass *class_)
{
	ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
	instance->class_ = class_;
	init_table(&instance->fields);
	return instance;
}
