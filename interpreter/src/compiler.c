// Copyright 2024 Dimitrios Papakonstantinou. All rights reserved.
// Use of this source code is governed by a MIT
// license that can be found in the LICENSE file.

#include "value.h"
#include "compiler.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"
#include "memory.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

// Parser state
typedef struct {
	Token current; // Current token
	Token previous; // Previous token
	bool had_error; // Keep track of error accurance
	bool panic_mode; // Mark parser panic
} Parser;

// Operator precedence of Xanadu
typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR, // or
	PREC_AND, // and
	PREC_EQUALITY, // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM, // + -
	PREC_FACTOR, // * /
	PREC_UNARY, // ! -
	PREC_CALL, // . ()
	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

// Define a parser rule
typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

// Holds information of local variables
typedef struct {
	Token name; // Variable name
	int depth; // Variable scope
	bool isCaptured; // Track if varibale is captured
} Local;

typedef struct {
	uint8_t index; // Index in table
	bool isLocal; // Mark as global
} Upvalue;

// Enum tracking types of functions
typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
	TYPE_METHOD,
	TYPE_INITIALIZER
} FunctionType;

// Keep track of compiler state
typedef struct Compiler {
	struct Compiler *enclosing; // Compiler Stack
	ObjFunction *function; // Function call stack
	FunctionType type; // Function type
	Local locals[UINT8_COUNT]; // Array of local variables
	Upvalue upvalues[UINT8_COUNT];
	int localCount; // Count of local variables
	int scopeDepth; // Keeps track of scope level
} Compiler;

typedef struct ClassCompiler {
	struct ClassCompiler *enclosing;
	bool has_super_class;
} ClassCompiler;

// Global Variables
Parser parser;
Compiler *current = NULL;
ClassCompiler *currentClass = NULL;
Chunk *compiling_chunk;
//#################

// Function definitions
static void advance(void);
static void error_at_current(const char *message);
static void error(const char *message);
static void error_at(Token *token, const char *message);
static void consume(TokenType type, const char *message);
// Compile expression
static void expression(void);
static void emit_byte(uint8_t byte);
// Get current chunk
static Chunk *current_chunk(void);
static ObjFunction *end_compiler(void);
// Set return operation on VM
static void emit_return(void);
static void emit_bytes(uint8_t byte1, uint8_t byte2);
// Set loop operation on VM
static void emit_loop(int loopStart);
// Set jump operation on VM
static int emit_jump(uint8_t instruction);
// Add new constant to VM
static void emit_constant(Value value);
static void patch_jump(int offset);
// Xanadu function call
static void call(bool canAssign);
static void dot(bool canAssign);
static uint8_t make_constant(Value value);
static void unary(bool can_assign);
static void grouping(bool can_assign);
// Compile binary expression
static void binary(bool can_assign);
// Compile String
static void string(bool can_assign);
// Compile number
static void number(bool can_assign);
static void parse_precedence(Precedence precedence);
static ParseRule *get_rule(TokenType type);
// Compile literal
static void literal(bool can_assign);
static void declaration(void);
// Compile statement
static void statement(void);
static void block(void);
static void begin_scope(void);
static void end_scope(void);
static uint8_t argument_list(void);
static void add_local(Token name);
static int add_upvalue(Compiler *compiler, uint8_t index, bool isLocal);
static void mark_initialized(void);
// Check if current type is the type given as an argument
static bool check(TokenType type);
// Check if the current token is the expexted one and advance to next token
static bool match(TokenType type);
// Compile print statement
static void print_statement(void);
// Compile expression statement
static void expression_statement(void);
// Compile if statement
static void if_statement(void);
// Compile while statement
static void while_statement(void);
// Compile for statement
static void for_statement(void);
// Compile return statement
static void return_statement(void);
// Synchronize compiler on panic
static void synchronize(void);
static void var_declaration(void);
static uint8_t parse_variable(const char *errorMessage);
static uint8_t identifier_constant(Token *name);
static int resolve_local(Compiler *compiler, Token *name);
static bool identifiers_equal(Token *a, Token *b);
static void define_variable(uint8_t global);
static void declare_variable(void);
static void fun_declaration(void);
static void function(FunctionType type);
// Compile String
static void variable(bool can_assign);
static Token synthetic_token(const char *text);
// Compile this statement
static void _this(bool canAssign);
static void named_variable(Token name, bool can_assign);
static int resolve_upvalue(Compiler *compiler, Token *name);
static void init_compiler(Compiler *compiler, FunctionType type);
static void and_(bool can_assign);
static void or_(bool can_assign);
//#####################

// Function definitions

// Initialise compiler
static void init_compiler(Compiler *compiler, FunctionType type)
{
	compiler->function = NULL;
	compiler->type = type;
	compiler->enclosing = current;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	compiler->function = new_function();
	current = compiler;

	// Check if the source is a function or the source script
	if (type != TYPE_SCRIPT) {
		current->function->name = copy_string(parser.previous.start,
						      parser.previous.length);
	}

	// Create slot zero local
	Local *local = &current->locals[current->localCount++];
	local->depth = 0;
	// Mark as uncaptured
	local->isCaptured = false;
	if (type != TYPE_FUNCTION) {
		local->name.start = "todays";
		local->name.length = 6;
	} else {
		local->name.start = "";
		local->name.length = 0;
	}
}

// Compile source string
ObjFunction *compile(const char *source)
{
	// Tokenise source string
	init_scanner(source);

	// Initialise compiler
	Compiler compiler;
	init_compiler(&compiler, TYPE_SCRIPT);

	// Reset error state
	parser.had_error = false;
	parser.panic_mode = false;

	// Advance to next token
	advance();

	// Compile until end of file
	while (!match(TOKEN_EOF)) {
		declaration();
	}

	ObjFunction *function = end_compiler();
	return parser.had_error ? NULL : function;
}

// Mark values heap allocated by the compiler
void mark_compiler_roots()
{
	Compiler *compiler = current;
	while (compiler != NULL) {
		mark_object((Obj *)compiler->function);
		compiler = compiler->enclosing;
	}
}

// Compile expression
static void expression(void)
{
	parse_precedence(PREC_ASSIGNMENT);
}

// Finish compilation
static ObjFunction *end_compiler(void)
{
	emit_return();
	ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
	if (!parser.had_error) {
		disassemble_chunk(current_chunk(),
				  function->name != NULL ?
					  function->name->chars :
					  "<script>");
	}
#endif

	current = current->enclosing;
	return function;
}

// Compile binary expression
static void binary(bool can_assign)
{
	// Create new operator type and retrieve the parser rule for that operator
	TokenType operatorType = parser.previous.type;
	ParseRule *rule = get_rule(operatorType);
	parse_precedence((Precedence)(rule->precedence + 1));

	// Check all operators and act accordingly
	switch (operatorType) {
	case TOKEN_BANG_EQUAL:
		emit_bytes(OP_EQUAL, OP_NOT);
		break;
	case TOKEN_EQUAL_EQUAL:
		emit_byte(OP_EQUAL);
		break;
	case TOKEN_GREATER:
		emit_byte(OP_GREATER);
		break;
	case TOKEN_GREATER_EQUAL:
		emit_bytes(OP_LESS, OP_NOT);
		break;
	case TOKEN_LESS:
		emit_byte(OP_LESS);
		break;
	case TOKEN_LESS_EQUAL:
		emit_bytes(OP_GREATER, OP_NOT);
		break;
	case TOKEN_PLUS:
		emit_byte(OP_ADD);
		break;
	case TOKEN_BANG:
		emit_byte(OP_NOT);
		break;
	case TOKEN_MINUS:
		emit_byte(OP_SUBTRACT);
		break;
	case TOKEN_STAR:
		emit_byte(OP_MULTIPLY);
		break;
	case TOKEN_SLASH:
		emit_byte(OP_DIVIDE);
		break;
	default:
		return; // Unreachable.
	}
}

// Xanadu function call
static void call(bool canAssign)
{
	uint8_t argCount = argument_list();
	emit_bytes(OP_CALL, argCount);
}

static void dot(bool canAssign)
{
	consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
	uint8_t name = identifier_constant(&parser.previous);

	if (canAssign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(OP_SET_PROPERTY, name);
	} else if (match(TOKEN_LEFT_PAREN)) {
		uint8_t argCount = argument_list();
		emit_bytes(OP_INVOKE, name);
		emit_byte(argCount);
	} else {
		emit_bytes(OP_GET_PROPERTY, name);
	}
}

// Compile literal
static void literal(bool can_assign)
{
	// Get token, check all possible tokens and act accordingly
	switch (parser.previous.type) {
	case TOKEN_FALSE:
		emit_byte(OP_FALSE);
		break;
	case TOKEN_NIL:
		emit_byte(OP_NIL);
		break;
	case TOKEN_TRUE:
		emit_byte(OP_TRUE);
		break;
	default:
		return; // Unreachable.
	}
}

// Set return operation on VM
static void emit_return(void)
{
	if (current->type == TYPE_INITIALIZER) {
		emit_bytes(OP_GET_LOCAL, 0);
	} else {
		emit_byte(OP_NIL);
	}

	emit_byte(OP_RETURN);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
	emit_byte(byte1);
	emit_byte(byte2);
}

// Set loop operation on VM
static void emit_loop(int loopStart)
{
	emit_byte(OP_LOOP);

	int offset = current_chunk()->count - loopStart + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large.");

	emit_byte((offset >> 8) & 0xff);
	emit_byte(offset & 0xff);
}

// Set jump operation on VM
static int emit_jump(uint8_t instruction)
{
	emit_byte(instruction);
	emit_byte(0xff);
	emit_byte(0xff);
	return current_chunk()->count - 2;
}

// Add new constant to VM
static void emit_constant(Value value)
{
	emit_bytes(OP_CONSTANT, make_constant(value));
}

static void patch_jump(int offset)
{
	// -2 to adjust for the bytecode for the jump offset itself.
	int jump = current_chunk()->count - offset - 2;

	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}

	current_chunk()->code[offset] = (jump >> 8) & 0xff;
	current_chunk()->code[offset + 1] = jump & 0xff;
}

// Compile number
static void number(bool can_assign)
{
	double value = strtod(parser.previous.start, NULL);
	emit_constant(NUMBER_VAL(value));
}

// Compile String
static void string(bool can_assign)
{
	emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
					  parser.previous.length - 2)));
}

// Compile Variable
static void variable(bool can_assign)
{
	named_variable(parser.previous, can_assign);
}

static Token synthetic_token(const char *text)
{
	Token token;
	token.start = text;
	token.length = (int)strlen(text);
	return token;
}

static void super_(bool canAssign)
{
	if (currentClass == NULL) {
		error("Can't use 'syrinx' outside of a class.");
	} else if (!currentClass->has_super_class) {
		error("Can't use 'syrinx' in a class with no superclass.");
	}

	consume(TOKEN_DOT, "Expect '.' after 'syrinx'.");
	consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
	uint8_t name = identifier_constant(&parser.previous);

	named_variable(synthetic_token("todays"), false);

	if (match(TOKEN_LEFT_PAREN)) {
		uint8_t argCount = argument_list();
		named_variable(synthetic_token("syrinx"), false);
		emit_bytes(OP_SUPER_INVOKE, name);
		emit_byte(argCount);
	} else {
		named_variable(synthetic_token("syrinx"), false);
		emit_bytes(OP_GET_SUPER, name);
	}
}

// Compile this statement
static void _this(bool canAssign)
{
	// Check if inside a class declaration
	if (currentClass == NULL) {
		error("Can't use 'this' outside of a class");
		return;
	}

	variable(false);
}

static void function(FunctionType type)
{
	Compiler compiler;
	init_compiler(&compiler, type);
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			current->function->arity++;
			if (current->function->arity > 255) {
				error_at_current(
					"Can't have more than 255 parameters.");
			}
			uint8_t constant =
				parse_variable("Expect parameter name.");
			define_variable(constant);
		} while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();

	ObjFunction *function = end_compiler();
	emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(function)));

	for (int i = 0; i < function->upvalueCount; i++) {
		emit_byte(compiler.upvalues[i].isLocal ? 1 : 0);
		emit_byte(compiler.upvalues[i].index);
	}
}

static void method()
{
	consume(TOKEN_IDENTIFIER, "Expect method name.");
	uint8_t constant = identifier_constant(&parser.previous);

	FunctionType type = TYPE_METHOD;

	if (parser.previous.length == 4 &&
	    memcmp(parser.previous.start, "init", 4) == 0) {
		type = TYPE_INITIALIZER;
	}

	function(type);
	emit_bytes(OP_METHOD, constant);
}

static void class_declaration()
{
	consume(TOKEN_IDENTIFIER, "Expect class name.");
	Token className = parser.previous;
	uint8_t nameConstant = identifier_constant(&parser.previous);
	declare_variable();

	emit_bytes(OP_CLASS, nameConstant);
	define_variable(nameConstant);

	ClassCompiler classCompiler;
	classCompiler.has_super_class = false;
	classCompiler.enclosing = currentClass;
	currentClass = &classCompiler;

	if (match(TOKEN_EXTENDS)) {
		consume(TOKEN_IDENTIFIER, "Epxected superclass name.");
		variable(false);

		if (identifiers_equal(&className, &parser.previous)) {
			error("A class can't inherit from itself.");
		}

		begin_scope();
		add_local(synthetic_token("syrinx"));
		define_variable(0);

		named_variable(className, false);
		emit_byte(OP_INHERIT);
		classCompiler.has_super_class = true;
	}

	named_variable(className, false);
	consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		method();
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
	emit_byte(OP_POP);

	if (classCompiler.has_super_class) {
		end_scope();
	}

	currentClass = currentClass->enclosing;
}

static uint8_t make_constant(Value value)
{
	int constant = add_constant(current_chunk(), value);
	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}

	return (uint8_t)constant;
}

static void grouping(bool can_assign)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool can_assign)
{
	TokenType operatorType = parser.previous.type;

	// Compile the operand.
	parse_precedence(PREC_UNARY);

	// Emit the operator instruction.
	switch (operatorType) {
	case TOKEN_MINUS:
		emit_byte(OP_NEGATE);
		break;
	default:
		return; // Unreachable.
	}
}

ParseRule rules[] = {
	[TOKEN_LEFT_PAREN] = { grouping, call, PREC_CALL },
	[TOKEN_RIGHT_PAREN] = { NULL, NULL, PREC_NONE },
	[TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE },
	[TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE },
	[TOKEN_COMMA] = { NULL, NULL, PREC_NONE },
	[TOKEN_DOT] = { NULL, dot, PREC_CALL },
	[TOKEN_MINUS] = { unary, binary, PREC_TERM },
	[TOKEN_PLUS] = { NULL, binary, PREC_TERM },
	[TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE },
	[TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
	[TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
	[TOKEN_BANG] = { unary, NULL, PREC_NONE },
	[TOKEN_BANG_EQUAL] = { NULL, binary, PREC_EQUALITY },
	[TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
	[TOKEN_EQUAL_EQUAL] = { NULL, binary, PREC_EQUALITY },
	[TOKEN_GREATER] = { NULL, binary, PREC_COMPARISON },
	[TOKEN_GREATER_EQUAL] = { NULL, binary, PREC_COMPARISON },
	[TOKEN_LESS] = { NULL, binary, PREC_COMPARISON },
	[TOKEN_LESS_EQUAL] = { NULL, binary, PREC_COMPARISON },
	[TOKEN_IDENTIFIER] = { variable, NULL, PREC_NONE },
	[TOKEN_STRING] = { string, NULL, PREC_NONE },
	[TOKEN_NUMBER] = { number, NULL, PREC_NONE },
	[TOKEN_AND] = { NULL, and_, PREC_AND },
	[TOKEN_CLASS] = { NULL, NULL, PREC_NONE },
	[TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
	[TOKEN_FALSE] = { literal, NULL, PREC_NONE },
	[TOKEN_FOR] = { NULL, NULL, PREC_NONE },
	[TOKEN_FUN] = { NULL, NULL, PREC_NONE },
	[TOKEN_IF] = { NULL, NULL, PREC_NONE },
	[TOKEN_NIL] = { literal, NULL, PREC_NONE },
	[TOKEN_OR] = { NULL, or_, PREC_OR },
	[TOKEN_PRINT] = { NULL, NULL, PREC_NONE },
	[TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
	[TOKEN_SUPER] = { super_, NULL, PREC_NONE },
	[TOKEN_THIS] = { _this, NULL, PREC_NONE },
	[TOKEN_TRUE] = { literal, NULL, PREC_NONE },
	[TOKEN_VAR] = { NULL, NULL, PREC_NONE },
	[TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
	[TOKEN_ERROR] = { NULL, NULL, PREC_NONE },
	[TOKEN_EOF] = { NULL, NULL, PREC_NONE },
};

static void parse_precedence(Precedence precedence)
{
	advance();

	ParseFn prefixRule = get_rule(parser.previous.type)->prefix;
	if (prefixRule == NULL) {
		error("Expect expression.");
		return;
	}

	bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefixRule(can_assign);

	while (precedence <= get_rule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = get_rule(parser.previous.type)->infix;
		infixRule(can_assign);
	}

	if (can_assign && match(TOKEN_EQUAL)) {
		error("Invalid assignment target.");
	}
}

static ParseRule *get_rule(TokenType type)
{
	return &rules[type];
}

static void advance(void)
{
	parser.previous = parser.current;

	for (;;) {
		parser.current = scan_token();
		if (parser.current.type != TOKEN_ERROR)
			break;

		error_at_current(parser.current.start);
	}
}

static void error_at_current(const char *message)
{
	error_at(&parser.current, message);
}

static void error(const char *message)
{
	error_at(&parser.previous, message);
}

static void error_at(Token *token, const char *message)
{
	if (parser.panic_mode)
		return;

	parser.panic_mode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type == TOKEN_ERROR) {
		// Nothing.
	} else {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.had_error = true;
}

static void consume(TokenType type, const char *message)
{
	if (parser.current.type == type) {
		advance();
		return;
	}

	error_at_current(message);
}

static void declaration(void)
{
	if (match(TOKEN_CLASS)) {
		class_declaration();
	} else if (match(TOKEN_FUN)) {
		fun_declaration();
	} else if (match(TOKEN_VAR)) {
		var_declaration();
	} else {
		statement();
	}

	if (parser.panic_mode)
		synchronize();
}

static bool identifiers_equal(Token *a, Token *b)
{
	if (a->length != b->length)
		return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

// Add local varibale to local array
static void add_local(Token name)
{
	if (current->localCount == UINT8_COUNT) {
		error("Too many local variables in function.");
		return;
	}

	Local *local = &current->locals[current->localCount++];
	local->name = name;
	// Mark variable as uncaptured
	local->isCaptured = false;
	// Mark variable as uninitialized
	local->depth = -1;
}

static void declare_variable(void)
{
	if (current->scopeDepth == 0)
		return;

	Token *name = &parser.previous;

	for (int i = current->localCount - 1; i >= 0; i--) {
		Local *local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scopeDepth) {
			break;
		}

		if (identifiers_equal(name, &local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}

	add_local(*name);
}

static void define_variable(uint8_t global)
{
	if (current->scopeDepth > 0) {
		mark_initialized();
		return;
	}
	emit_bytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argument_list()
{
	uint8_t argCount = 0;
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if (argCount == 255) {
				error("Can't have more than 255 arguments.");
			}
			argCount++;
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return argCount;
}

static void and_(bool canAssign)
{
	int endJump = emit_jump(OP_JUMP_IF_FALSE);

	emit_byte(OP_POP);
	parse_precedence(PREC_AND);

	patch_jump(endJump);
}

static void or_(bool can_assign)
{
	int elseJump = emit_jump(OP_JUMP_IF_FALSE);
	int endJump = emit_jump(OP_JUMP);

	patch_jump(elseJump);
	emit_byte(OP_POP);

	parse_precedence(PREC_OR);
	patch_jump(endJump);
}

static uint8_t parse_variable(const char *errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);

	declare_variable();
	if (current->scopeDepth > 0)
		return 0;

	return identifier_constant(&parser.previous);
}

static void mark_initialized(void)
{
	if (current->scopeDepth == 0)
		return;

	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void var_declaration(void)
{
	uint8_t global = parse_variable("Expect variable name.");

	if (match(TOKEN_EQUAL)) {
		expression();
	} else {
		emit_byte(OP_NIL);
	}
	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

	define_variable(global);
}

static void fun_declaration(void)
{
	uint8_t global = parse_variable("Expect function name.");
	mark_initialized();
	function(TYPE_FUNCTION);
	define_variable(global);
}

static uint8_t identifier_constant(Token *name)
{
	return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static int resolve_local(Compiler *compiler, Token *name)
{
	for (int i = compiler->localCount - 1; i >= 0; --i) {
		Local *local = &compiler->locals[i];
		if (identifiers_equal(name, &local->name)) {
			if (local->depth == -1) {
				error("Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}

static void named_variable(Token name, bool can_assign)
{
	uint8_t getOp, setOp;
	int arg = resolve_local(current, &name);

	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else if ((arg = resolve_upvalue(current, &name)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;

	} else {
		arg = identifier_constant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(setOp, (uint8_t)arg);
	} else {
		emit_bytes(getOp, (uint8_t)arg);
	}
}

static int add_upvalue(Compiler *compiler, uint8_t index, bool isLocal)
{
	int upvalueCount = compiler->function->upvalueCount;

	for (int i = 0; i < upvalueCount; i++) {
		Upvalue *upvalue = &compiler->upvalues[i];
		if (upvalue->index == index && upvalue->isLocal == isLocal) {
			return i;
		}
	}

	if (upvalueCount == UINT8_COUNT) {
		error("Too many closure variables in function.");
		return 0;
	}

	compiler->upvalues[upvalueCount].isLocal = isLocal;
	compiler->upvalues[upvalueCount].index = index;
	return compiler->function->upvalueCount++;
}

static int resolve_upvalue(Compiler *compiler, Token *name)
{
	if (compiler->enclosing == NULL)
		return -1;

	int local = resolve_local(compiler->enclosing, name);
	if (local != -1) {
		// Mark local variable as captured
		compiler->enclosing->locals[local].isCaptured = true;
		return add_upvalue(compiler, (uint8_t)local, true);
	}

	int upvalue = resolve_upvalue(compiler->enclosing, name);
	if (upvalue != -1) {
		return add_upvalue(compiler, (uint8_t)upvalue, false);
	}

	return -1;
}

// Synchronize compiler on panic
static void synchronize(void)
{
	parser.panic_mode = false;

	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_SEMICOLON)
			return;
		switch (parser.current.type) {
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;

		default:; // Do nothing.
		}

		advance();
	}
}

// Check if current type is the type given as an argument
static bool check(TokenType type)
{
	return parser.current.type == type;
}

// Check if the current token is the expexted one and advance to next token
static bool match(TokenType type)
{
	if (!check(type))
		return false;

	advance();
	return true;
}

// Get current chunk
static Chunk *current_chunk(void)
{
	return &current->function->chunk;
}

// Define new block
static void block(void)
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// Enter new scope level
static void begin_scope(void)
{
	current->scopeDepth++;
}

// Exit new scope level
static void end_scope(void)
{
	// Leave current scope
	current->scopeDepth--;

	// Pop out of scope variables from stack
	while (current->localCount > 0 &&
	       current->locals[current->localCount - 1].depth >
		       current->scopeDepth) {
		// Place captured variables onto the stack
		if (current->locals[current->localCount - 1].isCaptured) {
			emit_byte(OP_CLOSE_UPVALUE);
		} else {
			emit_byte(OP_POP);
		}
		current->localCount--;
	}
}

// Compile statement
static void statement(void)
{
	if (match(TOKEN_PRINT)) {
		print_statement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		block();
		end_scope();
	} else if (match(TOKEN_IF)) {
		if_statement();
	} else if (match(TOKEN_FOR)) {
		for_statement();
	} else if (match(TOKEN_RETURN)) {
		return_statement();
	} else if (match(TOKEN_WHILE)) {
		while_statement();
	} else {
		expression_statement();
	}
}

// Compile expression statement
static void expression_statement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emit_byte(OP_POP);
}

// Compile for statement
static void for_statement(void)
{
	// Enter new scope
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	// Initializer clause
	if (match(TOKEN_SEMICOLON)) { // Endless loop
		// No initializer.
	} else if (match(TOKEN_VAR)) { // Loop step tracker variable
		var_declaration();
	} else { // Step length
		expression_statement();
	}

	int loopStart = current_chunk()->count;

	// Condition clause
	int exitJump = -1;
	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

		// Jump out of the loop if the condition is false.
		exitJump = emit_jump(OP_JUMP_IF_FALSE);
		emit_byte(OP_POP); // Condition.
	}

	// Increment clause
	if (!match(TOKEN_RIGHT_PAREN)) {
		int bodyJump = emit_jump(OP_JUMP);
		int incrementStart = current_chunk()->count;
		expression();
		emit_byte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

		emit_loop(loopStart);
		loopStart = incrementStart;
		patch_jump(bodyJump);
	}

	statement();
	emit_loop(loopStart);

	if (exitJump != -1) {
		patch_jump(exitJump);
		emit_byte(OP_POP); // Condition.
	}

	end_scope();
}

// Compile if statement
static void if_statement(void)
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int thenJump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();

	int elseJump = emit_jump(OP_JUMP);

	patch_jump(thenJump);

	emit_byte(OP_POP);

	if (match(TOKEN_ELSE))
		statement();

	patch_jump(elseJump);
}

// Compile print statement
static void print_statement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	// Invoke print operation
	emit_byte(OP_PRINT);
}

// Compile return statement
static void return_statement(void)
{
	// Dont let the program return from source file
	if (current->type == TYPE_SCRIPT) {
		error("Can't return from top-level code.");
	}
	if (match(TOKEN_SEMICOLON)) {
		emit_return();
	} else {
		if (current->type == TYPE_INITIALIZER) {
			error("Can't return a value from an initializer");
		}
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emit_byte(OP_RETURN);
	}
}

// Compile while statement
static void while_statement(void)
{
	int loop_start = current_chunk()->count;

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int exitJump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();

	emit_loop(loop_start);

	patch_jump(exitJump);
	emit_byte(OP_POP);
}

static void emit_byte(uint8_t byte)
{
	write_chunk(current_chunk(), byte, parser.previous.line);
}

//#####################
