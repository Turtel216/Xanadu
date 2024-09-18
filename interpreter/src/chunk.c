// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by an MIT
// license that can be found in the LICENSE file.

#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

// Initializes a Chunk structure with default values.
// Sets the code and line arrays to NULL, and initializes the constants array.
void init_chunk(Chunk *chunk)
{
	chunk->count = 0; // Number of bytes currently in the chunk
	chunk->capacity =
		0; // Capacity of the chunk (number of bytes it can hold)
	chunk->code = NULL; // Pointer to the array of bytecode
	chunk->lines =
		NULL; // Pointer to the array of line numbers corresponding to bytecode
	init_value_array(&chunk->constants); // Initialize the constants array
}

// Frees the resources used by a Chunk structure.
// Deallocates memory for the code and line arrays and frees the constants array.
// Reinitializes the chunk to its default state.
void free_chunk(Chunk *chunk)
{
	FREE_ARRAY(uint8_t, chunk->code,
		   chunk->capacity); // Free memory for bytecode array
	FREE_ARRAY(int, chunk->lines,
		   chunk->capacity); // Free memory for line number array
	free_value_array(&chunk->constants); // Free memory for constants array
	init_chunk(chunk); // Reinitialize the chunk
}

// Appends a byte to the chunk, expanding its capacity if necessary.
// Also records the line number associated with the byte in the chunk.
//
// Parameters:
//   chunk - The chunk to write to
//   byte - The byte to write
//   line - The line number associated with the byte
void write_chunk(Chunk *chunk, uint8_t byte, int line)
{
	// Check if the chunk needs more capacity and grow if necessary
	if (chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity =
			GROW_CAPACITY(oldCapacity); // Compute new capacity
		chunk->code =
			GROW_ARRAY(uint8_t, chunk->code, oldCapacity,
				   chunk->capacity); // Resize bytecode array
		chunk->lines =
			GROW_ARRAY(int, chunk->lines, oldCapacity,
				   chunk->capacity); // Resize line number array
	}

	chunk->code[chunk->count] = byte; // Store the byte in the code array
	chunk->lines[chunk->count] = line; // Store the associated line number
	chunk->count++; // Increment the count of bytes in the chunk
}

// Adds a constant value to the chunk's constants array and returns its index.
// Pushes the value onto the stack, writes it to the constants array, then pops it off the stack.
//
// Parameters:
//   chunk - The chunk to add the constant to
//   value - The constant value to add
//
// Returns:
//   The index of the added constant in the constants array
int add_constant(Chunk *chunk, Value value)
{
	push(value); // Push the value onto the stack
	write_value_array(&chunk->constants,
			  value); // Write the value to the constants array
	pop(); // Pop the value off the stack
	return chunk->constants.count -
	       1; // Return the index of the newly added constant
}
