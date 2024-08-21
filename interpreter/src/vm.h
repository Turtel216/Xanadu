#ifndef xanadu_vm_h
#define xanadu_vm_h

#include "value.h"
#include "chunk.h"
#include "lookup_table.h"

// Initiale maximum stack size
#define STACK_MAX 256

// Virtual machine meta data
typedef struct {
	Chunk *chunk; // Byte code chunk
	uint8_t *ip; // Unique vm id
	Value *stack; // Stack dynamic array pointer
	Value *stackTop; // Stack's head pointer
	int stack_size; // Stack size
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
