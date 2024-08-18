#include "vm.h"
#include <stdlib.h>
#include "debug.h"
#include "chunk.h"

int main(int argc, char *argv[])
{
	init_vm();

	Chunk chunk;
	init_chunk(&chunk);

	int constant = add_constant(&chunk, 1.2);
	write_chunk(&chunk, OP_CONSTANT, 123);
	write_chunk(&chunk, constant, 123);

	write_chunk(&chunk, OP_RETURN, 123);
	disassemble_chunk(&chunk, "test chunk");
	interpret(&chunk);

	free_vm();

	free_chunk(&chunk);

	return EXIT_SUCCESS;
}
