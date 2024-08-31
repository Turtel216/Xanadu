// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#ifndef xanadu_object_h
#define xanadu_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "lookup_table.h"

// Macro for getting the type of an object
#define OBJ_TYPE(value) (AS_OBJ(value)->type)
// Macro for checking if an object is a string object
#define IS_STRING(value) isObjType(value, OBJ_STRING)
// Macro for checking if an object is a native object
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
// Macro for checking if an object is a closure object
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
// Macro for converting a Value type to a Object String
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
// Macro for converting a value to type to a cstring
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
// Macro for converting a value type to a function object
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
// Macro for converting a value type to a native object
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
// Macro for converting a value type to a closure object
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
// Macro for checking if an object is a function object
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
// Macro for checking if an object is a xanadu object
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
// Macro for converting a value type to a xanadu object
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))
// Macro for checking if an object is a xanadu object instance
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
// Macro for converting a value type to a xanadu object instance
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))

// Enum containing the types of objects
typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
	OBJ_NATIVE,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_CLASS,
	OBJ_INSTANCE,
} ObjType;

// Object for the Xanadu VM
struct Obj {
	ObjType type; // Type of object
	struct Obj *next; // Pointer to next object in object list
	bool is_marked; // Track if object is marked for gc
};

// Object wrapper for function object
typedef struct {
	Obj obj; // Object type
	int arity; // Number of function arguments
	Chunk chunk; // Bytecode info
	ObjString *name; // String info
	int upvalueCount; // Number of upvalues
} ObjFunction;

// Function pointer for Native Functions
typedef Value (*NativeFn)(int argCount, Value *args);

// Object wrapper for Native Objects
typedef struct {
	Obj obj; // Xanadu VM object
	NativeFn function; // Native function
} ObjNative;

// Object wrapper for String object
struct ObjString {
	Obj obj; // Object data
	int length; // String length
	char *chars; // String character array
	uint32_t hash; // Hash value
};

// Object wrapper for Upvalue objects
typedef struct ObjUpvalue {
	Obj obj; // Xanadu VM Object
	Value *location; // Location of the upvalue on the stack
	struct ObjUpvalue *next; // Pointer to next node in upvalue linked list
	Value closed; // Hold closed upvalue value
} ObjUpvalue;

// Object wrapper for Closure objects
typedef struct {
	Obj obj; // Xanadu VM Object
	ObjFunction *function; // Array of functions on the call stack
	ObjUpvalue **upvalues; // Array of upvalues
	int upvalueCount; //Number of upvalues associeted with the closure object
} ObjClosure;

// Object wrapper for Xanadu Objects
typedef struct {
	Obj obj;
	ObjString *name;
	Table methods; // Class methods
} ObjClass;

// Object wrapper for Xanadu Object Instances
typedef struct {
	Obj obj;
	ObjClass *class_;
	Table fields;
} ObjInstance;

// Returns true value is of the given object type
static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

// Copy string to new ObjString and return new object
ObjString *copy_string(const char *chars, int length);
// Print object types to iostream
void print_object(Value value);
// Create a string object
ObjString *take_string(char *chars, int length);
// Create a function object
ObjFunction *new_function(void);
// Create a native object
ObjNative *new_native(NativeFn function);
// Create a closure object
ObjClosure *new_closure(ObjFunction *function);
// Create a upvalue obejct
ObjUpvalue *new_upvalue(Value *slot);
// Create a Class Object
ObjClass *new_class(ObjString *name);
// Create a object instance Object
ObjInstance *new_instance(ObjClass *class_);

#endif
