// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#include "vm.h"
#include <stdlib.h>
#include <stdio.h>

static void repl();
static void run_file(const char *path);
static char *read_file(const char *path);

int main(int argc, char *argv[])
{
	// Start up virtual machine
	init_vm();

	// Check for given xanadu file
	if (argc == 1) {
		repl(); // run command line interpreter
	} else if (argc == 2) {
		run_file(argv[1]); // run file interpreter
	} else {
		// Insufficient number of arguments, exit with error message
		fprintf(stderr, "Usage: clox [path]\n");
		exit(64);
	}

	// Close virtual machine
	free_vm();
	return EXIT_SUCCESS;
}

// Command line interpreter
static void repl()
{
	// input buffer
	char line[1024];
	// input loop
	for (;;) {
		printf("> ");

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		// intepreter input
		interpret(line);
	}
}

// File interpreter
static void run_file(const char *path)
{
	// Read from file
	char *source = read_file(path);
	// interpret read input
	InterpretResult result = interpret(source);
	free(source);

	// Exit on compile error
	if (result == INTERPRET_COMPILE_ERROR)
		exit(65);
	if (result == INTERPRET_RUNTIME_ERROR)
		exit(70);
}

// Read from given file
static char *read_file(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	// Allocate read buffer memory
	char *buffer = (char *)malloc(fileSize + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}

	// Read from file
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}

	// Append string termination character
	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}
