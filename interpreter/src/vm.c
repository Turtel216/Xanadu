#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "error.h"
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vm.h"

VM vm;

// Reset stack to initiale state
static void reset_stack()
{
	vm.stackTop = vm.stack;
	vm.stack_size = 0;
}

// Start up virtual machine
void init_vm()
{
	// Allocate memory for stack
	vm.stack = (Value *)malloc(sizeof(Value) * STACK_MAX);
	if (vm.stack == NULL)
		error_msg_exit(
			"Failed to initialize stack, memory allocation failed");

	// Set stack head pointer and stack size
	reset_stack();
}

// Close virtual machine and free up memory
void inline free_vm()
{
	free(vm.stack);
}

// Run compiled instructions on VM
static InterpretResult run()
{
// marcos for vm instruction execution
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)             \
	do {                      \
		double b = pop(); \
		double a = pop(); \
		push(a op b);     \
	} while (false)
	//#########################################
	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		printf("          ");
		for (Value *slot = vm.stack; slot < vm.stackTop; ++slot) {
			printf("[ ");
			print_value(*slot);
			printf(" ]");
		}
		printf("\n");

		disassemble_instruction(vm.chunk,
					(int)(vm.ip - vm.chunk->code));
#endif

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
		case OP_CONSTANT: {
			Value constant = READ_CONSTANT();
			push(constant);
			break;
		}
		case OP_ADD:
			BINARY_OP(+);
			break;
		case OP_SUBTRACT:
			BINARY_OP(-);
			break;
		case OP_MULTIPLY:
			BINARY_OP(*);
			break;
		case OP_DIVIDE:
			BINARY_OP(/);
			break;
		case OP_NEGATE:
			push(-pop());
			break;
		case OP_RETURN: {
			print_value(pop());
			printf("\n");
			return INTERPRET_OK;
		}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

// Interpret given string
InterpretResult interpret(const char *source)
{
	// Create Byte code chunk
	Chunk chunk;
	init_chunk(&chunk);

	// Compile string
	if (!compile(source, &chunk)) {
		free_chunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	// Run compiled instructions on VM
	InterpretResult result = run();

	// Clean up chunk
	free_chunk(&chunk);
	return result;
}

// Grow vm stack's size
static inline void grow_stack()
{
	// Reallocate new stack size
	vm.stack = realloc(vm.stack, vm.stack_size + STACK_MAX);
	if (vm.stack == NULL)
		error_msg_exit("Failed to grow stack");
}

// Push onto VM stack
void push(Value value)
{
	if (++vm.stack_size >= STACK_MAX)
		grow_stack();

	*vm.stackTop = value;
	vm.stackTop++;
}

// Pop from VM stack
Value pop()
{
	--vm.stackTop;
	--vm.stack_size;
	return *vm.stackTop;
}
