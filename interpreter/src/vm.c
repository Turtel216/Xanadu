#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "error.h"
#include "lookup_table.h"
#include "object.h"
#include "compiler.h"
#include "memory.h"
#include "value.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

VM vm;

// Function declarations
static Value peek(int distance);
static void runtime_error(const char *format, ...);
static bool is_falsey(Value value);
// Concatenate first 2 strings on the stack
static void concatenate();
//######################

// Reset stack to initiale state
static void reset_stack()
{
	vm.stackTop = vm.stack;
	//vm.stack_size = 0;
}

// Start up virtual machine
void init_vm()
{
	// Allocate memory for stack
	vm.stack = (Value *)malloc(sizeof(Value) * STACK_MAX);
	if (vm.stack == NULL)
		error_msg_exit(
			"Failed to initialize stack, memory allocation failed");

	init_table(&vm.strings);
	init_table(&vm.globals);

	// Set stack head pointer and stack size
	reset_stack();
	vm.objects = NULL;
}

// Close virtual machine and free up memory
void inline free_vm()
{
	free_table(&vm.strings);
	free_table(&vm.globals);
	free_objects();
	free(vm.stack);
}

// Run compiled instructions on VM
static InterpretResult run()
{
// marcos for vm instruction execution
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define BINARY_OP(valueType, op)                                    \
	do {                                                        \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {   \
			runtime_error("Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR;             \
		}                                                   \
		double b = AS_NUMBER(pop());                        \
		double a = AS_NUMBER(pop());                        \
		push(valueType(a op b));                            \
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
		case OP_NIL:
			push(NIL_VAL);
			break;
		case OP_TRUE:
			push(BOOL_VAL(true));
			break;
		case OP_FALSE:
			push(BOOL_VAL(false));
			break;
		case OP_SET_GLOBAL: {
			ObjString *name = READ_STRING();
			if (insert_into_table(&vm.globals, name, peek(0))) {
				delete_from_table(&vm.globals, name);
				runtime_error("Undefined variable '%s'.",
					      name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_EQUAL: {
			Value b = pop();
			Value a = pop();
			push(BOOL_VAL(values_equal(a, b)));
			break;
		}
		case OP_ADD: {
			if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
				concatenate();
			} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
				double b = AS_NUMBER(pop());
				double a = AS_NUMBER(pop());
				push(NUMBER_VAL(a + b));
			} else {
				runtime_error(
					"Operands must be two numbers or two strings.");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_GREATER:
			BINARY_OP(BOOL_VAL, >);
			break;
		case OP_LESS:
			BINARY_OP(BOOL_VAL, <);
			break;
		case OP_SUBTRACT:
			BINARY_OP(NUMBER_VAL, -);
			break;
		case OP_MULTIPLY:
			BINARY_OP(NUMBER_VAL, *);
			break;
		case OP_DIVIDE:
			BINARY_OP(NUMBER_VAL, /);
			break;
		case OP_NOT:
			push(BOOL_VAL(is_falsey(pop())));
			break;
		case OP_NEGATE:
			if (!IS_NUMBER(peek(0))) {
				runtime_error("Operand must be a number.");
				return INTERPRET_RUNTIME_ERROR;
			}
			push(NUMBER_VAL(-AS_NUMBER(pop())));
			break;
		case OP_PRINT: {
			print_value(pop());
			printf("\n");
			break;
		}
		case OP_GET_GLOBAL: {
			ObjString *name = READ_STRING();
			Value value;
			if (!table_get_from_table(&vm.globals, name, &value)) {
				runtime_error("Undefined variable '%s'.",
					      name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			push(value);
			break;
		}
		case OP_POP:
			pop();
			break;
		case OP_GET_LOCAL: {
			uint8_t slot = READ_BYTE();
			push(vm.stack[slot]);
			break;
		}
		case OP_SET_LOCAL: {
			uint8_t slot = READ_BYTE();
			vm.stack[slot] = peek(0);
			break;
		}
		case OP_DEFINE_GLOBAL: {
			ObjString *name = READ_STRING();
			insert_into_table(&vm.globals, name, peek(0));
			pop();
			break;
		}
		case OP_JUMP_IF_FALSE: {
			uint16_t offset = READ_SHORT();
			if (is_falsey(peek(0)))
				vm.ip += offset;
			break;
		}
		case OP_JUMP: {
			uint16_t offset = READ_SHORT();
			vm.ip += offset;
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			vm.ip -= offset;
			break;
		}
		case OP_RETURN: {
			// Exit interpretor
			return INTERPRET_OK;
		}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef READ_SHORT
}

// Get stack value of specific distance
static Value peek(int distance)
{
	return vm.stackTop[-1 - distance];
}

// Throw runtime error and reset stack
static void runtime_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = vm.ip - vm.chunk->code - 1;
	int line = vm.chunk->lines[instruction];
	fprintf(stderr, "[line %d] in script\n", line);
	reset_stack();
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

// Check bool value of Value type
static bool is_falsey(Value value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// Concatenate first 2 strings on the stack
static void concatenate()
{
	ObjString *b = AS_STRING(pop());
	ObjString *a = AS_STRING(pop());

	int length = a->length + b->length;
	char *chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	ObjString *result = take_string(chars, length);
	push(OBJ_VAL(result));
}

/*
// Grow vm stack's size
static inline void grow_stack()
{
	// Reallocate new stack size
	vm.stack = realloc(vm.stack, vm.stack_size + STACK_MAX);
	if (vm.stack == NULL)
		error_msg_exit("Failed to grow stack");
}
TODO
*/

// Push onto VM stack
void push(Value value)
{
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
