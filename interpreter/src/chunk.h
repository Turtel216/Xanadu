#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

typedef enum {
	OP_RETURN,
} OpCode;

// Chunk holding byte code
typedef struct {
	int count;
	int capacity;
	uint8_t *code;
} Chunk;

// Initialize chunk
void init_chunk(Chunk *chunk);
// Free chunk
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);

#endif
