// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
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

// Global Variables
VM vm;

// Function declarations
// Get stack value of specific distance
static Value peek(int distance);
// Throw runtime error and reset stack
static void runtime_error(const char *format, ...);
// Check bool value of Value type
static bool is_falsey(Value value);
static bool call_value(Value callee, int argCount);
static void define_native(const char *name, NativeFn function);
static Value clock_native(int argCount, Value *args);
static ObjUpvalue *capture_upvalue(Value *local);
static void close_upvalues(Value *last);
// Concatenate first 2 strings on the stack
static void concatenate(void);
//######################

// Function definition

// Reset stack to initiale state
static void reset_stack(void)
{
	vm.stackTop = vm.stack;
	vm.frameCount = 0;
	vm.openUpvalues = NULL;
}

// Start up virtual machine
void init_vm(void)
{
	// Set stack head pointer and stack size
	reset_stack();

	vm.objects = NULL;
	vm.gray_count = 0;
	vm.gray_capacity = 0;
	vm.gray_stack = NULL;
	vm.bytesAllocated = 0;
	vm.nextGC = 1024 * 1024;

	init_table(&vm.strings);
	init_table(&vm.globals);

	define_native("clock", clock_native);
}

// Close virtual machine and free up memory
void free_vm(void)
{
	free_table(&vm.strings);
	free_table(&vm.globals);
	free_objects();
}

// Run compiled instructions on VM
static InterpretResult run(void)
{
	CallFrame *frame = &vm.frames[vm.frameCount - 1];

	// marcos for vm instruction execution
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
	(frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
	(frame->closure->function->chunk.constants.values[READ_BYTE()])
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

		disassemble_instruction(
			&frame->closure->function->chunk,
			(int)(frame->ip -
			      frame->closure->function->chunk.code));
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
		case OP_GET_UPVALUE: {
			uint8_t slot = READ_BYTE();
			push(*frame->closure->upvalues[slot]->location);
			break;
		}
		case OP_SET_UPVALUE: {
			uint8_t slot = READ_BYTE();
			*frame->closure->upvalues[slot]->location = peek(0);
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
			push(frame->slots[slot]);
			break;
		}
		case OP_SET_LOCAL: {
			uint8_t slot = READ_BYTE();
			frame->slots[slot] = peek(0);
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
				frame->ip += offset;
			break;
		}
		case OP_JUMP: {
			uint16_t offset = READ_SHORT();
			frame->ip += offset;
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			frame->ip -= offset;
			break;
		}
		case OP_CALL: {
			int argCount = READ_BYTE();
			if (!call_value(peek(argCount), argCount)) {
				return INTERPRET_RUNTIME_ERROR;
			}
			frame = &vm.frames[vm.frameCount - 1];
			break;
		}
		case OP_CLOSURE: {
			ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
			ObjClosure *closure = new_closure(function);
			push(OBJ_VAL(closure));
			for (int i = 0; i < closure->upvalueCount; i++) {
				uint8_t isLocal = READ_BYTE();
				uint8_t index = READ_BYTE();
				if (isLocal) {
					closure->upvalues[i] = capture_upvalue(
						frame->slots + index);
				} else {
					closure->upvalues[i] =
						frame->closure->upvalues[index];
				}
			}
			break;
		}
		case OP_CLOSE_UPVALUE:
			close_upvalues(vm.stackTop - 1);
			pop();
			break;
		case OP_CLASS:
			push(OBJ_VAL(new_class(READ_STRING())));
			break;
		case OP_RETURN: {
			Value result = pop();
			close_upvalues(frame->slots);
			vm.frameCount--;
			if (vm.frameCount == 0) {
				pop();
				return INTERPRET_OK;
			}

			vm.stackTop = frame->slots;
			push(result);
			frame = &vm.frames[vm.frameCount - 1];
			break;
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

static bool call(ObjClosure *closure, int argCount)
{
	if (argCount != closure->function->arity) {
		runtime_error("Expected %d arguments but got %d.",
			      closure->function->arity, argCount);
		return false;
	}

	if (vm.frameCount == FRAMES_MAX) {
		runtime_error("Stack overflow.");
		return false;
	}

	CallFrame *frame = &vm.frames[vm.frameCount++];
	frame->closure = closure;
	frame->ip = closure->function->chunk.code;
	frame->slots = vm.stackTop - argCount - 1;
	return true;
}

static bool call_value(Value callee, int argCount)
{
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
		case OBJ_NATIVE: {
			NativeFn native = AS_NATIVE(callee);
			Value result = native(argCount, vm.stackTop - argCount);
			vm.stackTop -= argCount + 1;
			push(result);
			return true;
		}
		case OBJ_CLOSURE:
			return call(AS_CLOSURE(callee), argCount);
		default:
			break; // Non-callable object type.
		}
	}
	runtime_error("Can only call functions and classes.");
	return false;
}

static ObjUpvalue *capture_upvalue(Value *local)
{
	// Check for existing upvalue on linked list
	ObjUpvalue *prevUpvalue = NULL;
	ObjUpvalue *upvalue = vm.openUpvalues;
	while (upvalue != NULL && upvalue->location > local) {
		prevUpvalue = upvalue;
		upvalue = upvalue->next;
	}

	if (upvalue != NULL && upvalue->location == local) {
		return upvalue;
	}

	// Create new upvalue since it doesn't exist
	ObjUpvalue *createdUpvalue = new_upvalue(local);
	// Insert it into the linked list
	createdUpvalue->next = upvalue;

	if (prevUpvalue == NULL) {
		vm.openUpvalues = createdUpvalue;
	} else {
		prevUpvalue->next = createdUpvalue;
	}
	return createdUpvalue;
}

static void close_upvalues(Value *last)
{
	while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
		ObjUpvalue *upvalue = vm.openUpvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm.openUpvalues = upvalue->next;
	}
}

// Throw runtime error and reset stack
static void runtime_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = vm.frameCount - 1; i >= 0; i--) {
		CallFrame *frame = &vm.frames[i];
		ObjFunction *function = frame->closure->function;
		size_t instruction = frame->ip - function->chunk.code - 1;

		fprintf(stderr, "[line %d] in ",
			function->chunk.lines[instruction]);

		if (function->name == NULL) {
			fprintf(stderr, "script\n");
		} else {
			fprintf(stderr, "%s()\n", function->name->chars);
		}
	}

	reset_stack();
}

static void define_native(const char *name, NativeFn function)
{
	push(OBJ_VAL(copy_string(name, (int)strlen(name))));
	push(OBJ_VAL(new_native(function)));
	insert_into_table(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}

// Interpret given string
InterpretResult interpret(const char *source)
{
	ObjFunction *function = compile(source);
	if (function == NULL)
		return INTERPRET_COMPILE_ERROR;

	push(OBJ_VAL(function));
	ObjClosure *closure = new_closure(function);
	pop();
	push(OBJ_VAL(closure));
	call(closure, 0);

	return run();
}

// Check bool value of Value type
static bool is_falsey(Value value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// Concatenate first 2 strings on the stack
static void concatenate(void)
{
	ObjString *b = AS_STRING(pop());
	ObjString *a = AS_STRING(pop());

	int length = a->length + b->length;
	char *chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	ObjString *result = take_string(chars, length);

	pop();
	pop();

	push(OBJ_VAL(result));
}

static Value clock_native(int argCount, Value *args)
{
	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
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
Value pop(void)
{
	--vm.stackTop;
	return *vm.stackTop;
}
//####################
