// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#ifndef xanadu_chunk_h
#define xanadu_chunk_h

#include "common.h"
#include "value.h"

// Enumeration of virtual machine (VM) instruction opcodes.
// Each opcode represents a different type of operation that the VM can perform.
typedef enum {
	OP_CONSTANT, // Push a constant value onto the stack
	OP_NIL, // Push the nil value onto the stack
	OP_TRUE, // Push the true value onto the stack
	OP_POP, // Remove the top value from the stack
	OP_DEFINE_GLOBAL, // Define a global variable
	OP_FALSE, // Push the false value onto the stack
	OP_CALL, // Call a function
	OP_CLOSURE, // Create a closure (function with captured variables)
	OP_CLOSE_UPVALUE, // Close over an upvalue (captured variable)
	OP_GET_GLOBAL, // Retrieve a global variable
	OP_SET_GLOBAL, // Set a global variable
	OP_GET_LOCAL, // Retrieve a local variable
	OP_SET_LOCAL, // Set a local variable
	OP_GET_UPVALUE, // Retrieve an upvalue (captured variable)
	OP_SET_UPVALUE, // Set an upvalue (captured variable)
	OP_EQUAL, // Compare two values for equality
	OP_GREATER, // Compare two values to check if the first is greater
	OP_LESS, // Compare two values to check if the first is less
	OP_JUMP_IF_FALSE, // Jump to a specified instruction if the top value is false
	OP_JUMP, // Unconditionally jump to a specified instruction
	OP_LOOP, // Jump back to a previous instruction to create a loop
	OP_ADD, // Add the top two values on the stack
	OP_SUBTRACT, // Subtract the top value from the second-to-top value on the stack
	OP_MULTIPLY, // Multiply the top two values on the stack
	OP_DIVIDE, // Divide the second-to-top value by the top value on the stack
	OP_NOT, // Logical negation of the top value on the stack
	OP_NEGATE, // Negate the top value on the stack
	OP_PRINT, // Print the top value on the stack
	OP_RETURN, // Return from a function
	OP_CLASS, // Define a class
	OP_INHERIT, // Inherit from a superclass
	OP_GET_SUPER, // Retrieve a method from a superclass
	OP_SUPER_INVOKE, // Invoke a method from a superclass
	OP_METHOD, // Define a method for a class
	OP_GET_PROPERTY, // Retrieve a property from an object
	OP_SET_PROPERTY, // Set a property on an object
	OP_INVOKE, // Invoke a method on an object
} OpCode;

// Represents a chunk of bytecode, which is a sequence of VM instructions.
// The chunk contains the bytecode itself, line numbers for debugging, and a list of constant values.
typedef struct {
	int count; // Number of bytes currently in the chunk
	int capacity; // Total capacity of the bytecode array
	uint8_t *code; // Array of bytecode instructions
	int *lines; // Array of line numbers corresponding to each bytecode instruction
	ValueArray constants; // Array of constant values used in the bytecode
} Chunk;

// Initializes a Chunk structure with default values.
// Sets the bytecode array and line number array to NULL, and initializes the constants array.
void init_chunk(Chunk *chunk);

// Frees the resources used by a Chunk structure.
// Deallocates memory for the bytecode array, line number array, and constants array.
// Reinitializes the chunk to its default state.
void free_chunk(Chunk *chunk);

// Appends a byte to the chunk and records the line number associated with the byte.
// Expands the capacity of the chunk if necessary.
//
// Parameters:
//   chunk - The chunk to write to
//   byte - The byte to append to the chunk
//   line - The line number associated with the byte
void write_chunk(Chunk *chunk, uint8_t byte, int line);

// Adds a constant value to the chunk's constants array and returns its index.
// Pushes the value onto the stack, writes it to the constants array, then pops it off the stack.
//
// Parameters:
//   chunk - The chunk to add the constant to
//   value - The constant value to add
//
// Returns:
//   The index of the newly added constant in the constants array
int add_constant(Chunk *chunk, Value value);

#endif
