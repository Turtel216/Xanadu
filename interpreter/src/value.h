#ifndef xanadu_value_h
#define xanadu_value_h

#include "common.h"

// Marcos for converting C values into Xanadu values
#define BOOL_VAL(value) ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL ((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
//##################################################

// Marcos for converting Xanadu values into C values
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)
//##################################################

// Marcos for checking a values type
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)
//##################################

// Macro for converting Object type to value type
#define OBJ_VAL(object) ((Value){ VAL_OBJ, { .obj = (Obj *)object } })

// The kind of type of a value
typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
} ValueType;

typedef struct Obj Obj;
typedef struct ObjString ObjString;

// The information of a type
typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj *obj;
	} as;
} Value;

typedef struct {
	int capacity;
	int count;
	Value *values;
} ValueArray;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
void print_value(Value value);
InterpretResult interpret(const char *source);
// Checks if two values are equal
bool values_equal(Value a, Value b);

#endif
