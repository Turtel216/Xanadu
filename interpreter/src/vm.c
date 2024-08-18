#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vm.h"

VM vm;

static void reset_stack()
{
	vm.stackTop = vm.stack;
	vm.stack_size = 0;
}

void init_vm()
{
	vm.stack = (Value *)malloc(sizeof(Value) * STACK_MAX);
	if (vm.stack == NULL)
		error_msg_exit(
			"Failed to initialize stack, memory allocation failed");

	reset_stack();
}

void inline free_vm()
{
	free(vm.stack);
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)             \
	do {                      \
		double b = pop(); \
		double a = pop(); \
		push(a op b);     \
	} while (false)

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

InterpretResult interpret(Chunk *chunk)
{
	vm.chunk = chunk;
	vm.ip = vm.chunk->code;
	return run();
}

static inline void grow_stack()
{
	vm.stack = realloc(vm.stack, vm.stack_size + STACK_MAX);
	if (vm.stack == NULL)
		error_msg_exit("Failed to reallocate stack");
}

void push(Value value)
{
	if (++vm.stack_size >= STACK_MAX)
		grow_stack();

	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop()
{
	--vm.stackTop;
	--vm.stack_size;
	return *vm.stackTop;
}
