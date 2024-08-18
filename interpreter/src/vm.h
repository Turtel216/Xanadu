#ifndef xanadu_vm_h
#define xanadu_vm_h

#include "value.h"
#include "chunk.h"

#define STACK_MAX 256

typedef struct {
	Chunk *chunk;
	uint8_t *ip;
	Value *stack;
	Value *stackTop;
	int stack_size;
} VM;

void init_vm();
void free_vm();
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();

#endif
