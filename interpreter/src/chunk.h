#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Types of vm instructions
typedef enum {
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_POP,
	OP_DEFINE_GLOBAL,
	OP_FALSE,
	OP_CALL,
	OP_CLOSURE,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_JUMP_IF_FALSE,
	OP_JUMP,
	OP_LOOP,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_NEGATE,
	OP_PRINT,
	OP_RETURN,
} OpCode;

// Chunk holding byte code
typedef struct {
	int count;
	int capacity;
	uint8_t *code;
	int *lines;
	ValueArray constants;
} Chunk;

// Initialize chunk
void init_chunk(Chunk *chunk);
// Free chunk
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
int add_constant(Chunk *chunk, Value value);

#endif
