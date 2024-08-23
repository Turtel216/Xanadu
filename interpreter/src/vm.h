#ifndef xanadu_vm_h
#define xanadu_vm_h

#include "value.h"
#include "chunk.h"
#include "object.h"
#include "lookup_table.h"

// Initiale maximum stack size
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
	ObjFunction *function;
	uint8_t *ip;
	Value *slots;
} CallFrame;

// Virtual machine meta data
typedef struct {
	Chunk *chunk; // Byte code chunk
	uint8_t *ip; // Unique vm id
	Value *stack; // Stack dynamic array pointer
	Value *stackTop; // Stack's head pointer
	CallFrame frames[FRAMES_MAX];
	int frameCount;
	Table strings; // Hash table
	Table globals;
	Obj *objects; // Head of object list
} VM;

extern VM vm;

// Start up virtual machine
void init_vm();
// Close virtual machine and free up memory
void free_vm();
// Interpret given string
InterpretResult interpret(const char *source);
// Push onto VM stack
void push(Value value);
// Pop from VM stack
Value pop();

#endif
