#ifndef xanadu_object_h
#define xanadu_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"

// Macro for getting the type of an object
#define OBJ_TYPE(value) (AS_OBJ(value)->type)
// Macro for checking if an object is a string object
#define IS_STRING(value) isObjType(value, OBJ_STRING)
// Macro for checking if an object is a native object
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
// Macro for converting a Value type to a Object String
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
// Macro for converting a value to type to a cstring
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
// Macro for converting a value type to a function object
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
// Macro for converting a value type to a native object
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
// Macro for checking if an object is a function object
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// Enum containing the types of objects
typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
	OBJ_NATIVE,
} ObjType;

// Object meta data struct
struct Obj {
	ObjType type; // Type of object
	struct Obj *next; // Pointer to next object in object list
};

// Function meta data struct
typedef struct {
	Obj obj; // Object type
	int arity; //
	Chunk chunk; // Bytecode info
	ObjString *name; // String info
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
	Obj obj;
	NativeFn function;
} ObjNative;

// Meta data for String object
struct ObjString {
	Obj obj; // Object data
	int length; // String length
	char *chars; // String character array
	uint32_t hash; // Hash value
};

// Returns true value is of the given object type
static inline bool isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjString *copy_string(const char *chars, int length);
void print_object(Value value);
// Create a string object
ObjString *take_string(char *chars, int length);
// Create a function object
ObjFunction *new_function();
ObjNative *new_native(NativeFn function);

#endif
