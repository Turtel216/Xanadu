#include <stdlib.h>
#include "debug.h"
#include "chunk.h"

int main(int argc, char *argv[])
{
	Chunk chunk;
	init_chunk(&chunk);
	write_chunk(&chunk, OP_RETURN);
	disassemble_chunk(&chunk, "test chunk");

	free_chunk(&chunk);

	return EXIT_SUCCESS;
}
