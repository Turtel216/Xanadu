#ifndef xanadu_object_h
#define xanadu_object_h

#include "common.h"
#include "value.h"

// Macro for getting the type of an object
#define OBJ_TYPE(value) (AS_OBJ(value)->type)
// Macro for checking if an object is a string object
#define IS_STRING(value) isObjType(value, OBJ_STRING)
// Macro for converting a Value type to a Object String
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
// Macro for converting a value to type to a cstring
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

// Enum containing the types of objects
typedef enum {
	OBJ_STRING,
} ObjType;

// Object meta data struct
struct Obj {
	ObjType type; // Type of object
};

// Meta data for String object
struct ObjString {
	Obj obj; // Object data
	int length; // String length
	char *chars; // String character array
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

#endif
