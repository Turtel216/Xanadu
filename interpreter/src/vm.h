// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by a MIT
// license that can be found in the LICENSE file.

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
	ObjClosure *closure;
	uint8_t *ip;
	Value *slots;
} CallFrame;

// Virtual machine meta data
typedef struct {
	Chunk *chunk; // Byte code chunk
	uint8_t *ip; // Unique vm id
	Value stack[STACK_MAX]; // Stack dynamic array pointer
	Value *stackTop; // Stack's head pointer
	CallFrame frames[FRAMES_MAX];
	int frameCount;
	Table strings; // Hash table
	Table globals; // Hash table of global variables
	Obj *objects; // Head of object list
	ObjUpvalue *openUpvalues; // Array of open up values
	int gray_count;
	int gray_capacity;
	Obj **gray_stack;
	size_t bytesAllocated;
	size_t nextGC;
} VM;

// Global to program vm struct
extern VM vm;

// Start up virtual machine
void init_vm(void);
// Close virtual machine and free up memory
void free_vm(void);
// Interpret given string
InterpretResult interpret(const char *source);
// Push onto VM stack
void push(Value value);
// Pop from VM stack
Value pop(void);

#endif
