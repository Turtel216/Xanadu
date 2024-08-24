#include "value.h"
#include "compiler.h"
#include "scanner.h"
#include "chunk.h"
#include "object.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
	Token current;
	Token previous;
	bool had_error;
	bool panic_mode;
} Parser;

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

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

// Holds information of local variables
typedef struct {
	Token name; // Variable name
	int depth; // Variable scope
} Local;

// Enum tracking types of functions
typedef enum { TYPE_FUNCTION, TYPE_SCRIPT } FunctionType;

// Keep track of compiler state
typedef struct Compiler {
	struct Compiler *enclosing; // Compiler Stack
	ObjFunction *function; // Function call stack
	FunctionType type; // Function type
	Local locals[UINT8_COUNT]; // Array of local variables
	int localCount; // Count of local variables
	int scopeDepth; // Keeps track of scope level
} Compiler;

Parser parser;
Compiler *current;
Chunk *compiling_chunk;

// Function definitions
static void advance();
static void error_at_current(const char *message);
static void error(const char *message);
static void error_at(Token *token, const char *message);
static void consume(TokenType type, const char *message);
static void expression();
static void emit_byte(uint8_t byte);
static Chunk *current_chunk();
static ObjFunction *end_compiler();
static void emit_return();
static void emit_bytes(uint8_t byte1, uint8_t byte2);
static void emit_loop(int loopStart);
static int emit_jump(uint8_t instruction);
static void emit_constant(Value value);
static void patch_jump(int offset);
static void call(bool canAssign);
static uint8_t make_constant(Value value);
static void unary(bool can_assign);
static void grouping(bool can_assign);
static void binary(bool can_assign);
static void string(bool can_assign);
static void parse_precedence(Precedence precedence);
static ParseRule *get_rule(TokenType type);
static void literal(bool can_assign);
static void declaration();
static void statement();
static void block();
static void begin_scope();
static void end_scope();
static uint8_t argument_list();
static void add_local(Token name);
static void mark_initialized();
static bool check(TokenType type);
static bool match(TokenType type);
static void print_statement();
static void expression_statement();
static void if_statement();
static void while_statement();
static void for_statement();
static void return_statement();
static void synchronize();
static void var_declaration();
static uint8_t parse_variable(const char *errorMessage);
static uint8_t identifier_constant(Token *name);
static int resolve_local(Compiler *compiler, Token *name);
static bool identifiers_equal(Token *a, Token *b);
static void define_variable(uint8_t global);
static void declare_variable();
static void fun_declaration();
static void function(FunctionType type);
static void variable(bool can_assign);
static void named_variable(Token name, bool can_assign);
static void init_compiler(Compiler *compiler, FunctionType type);
static void and_(bool can_assign);
static void or_(bool can_assign);
//#####################

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

	if (type != TYPE_SCRIPT) {
		current->function->name = copy_string(parser.previous.start,
						      parser.previous.length);
	}

	Local *local = &current->locals[current->localCount++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
}

ObjFunction *compile(const char *source)
{
	init_scanner(source);

	// Initialise compiler
	Compiler compiler;
	init_compiler(&compiler, TYPE_SCRIPT);

	parser.had_error = false;
	parser.panic_mode = false;

	advance();

	while (!match(TOKEN_EOF)) {
		declaration();
	}

	ObjFunction *function = end_compiler();
	return parser.had_error ? NULL : function;
}

static void expression()
{
	parse_precedence(PREC_ASSIGNMENT);
}

static ObjFunction *end_compiler()
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

static void binary(bool can_assign)
{
	TokenType operatorType = parser.previous.type;
	ParseRule *rule = get_rule(operatorType);
	parse_precedence((Precedence)(rule->precedence + 1));

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

static void call(bool canAssign)
{
	uint8_t argCount = argument_list();
	emit_bytes(OP_CALL, argCount);
}

static void literal(bool can_assign)
{
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

static void emit_return()
{
	emit_byte(OP_NIL);
	emit_byte(OP_RETURN);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
	emit_byte(byte1);
	emit_byte(byte2);
}

static void emit_loop(int loopStart)
{
	emit_byte(OP_LOOP);

	int offset = current_chunk()->count - loopStart + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large.");

	emit_byte((offset >> 8) & 0xff);
	emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction)
{
	emit_byte(instruction);
	emit_byte(0xff);
	emit_byte(0xff);
	return current_chunk()->count - 2;
}

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

static void number(bool can_assign)
{
	double value = strtod(parser.previous.start, NULL);
	emit_constant(NUMBER_VAL(value));
}

static void string(bool can_assign)
{
	emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
					  parser.previous.length - 2)));
}

static void variable(bool can_assign)
{
	named_variable(parser.previous, can_assign);
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
	emit_bytes(OP_CONSTANT, make_constant(OBJ_VAL(function)));
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
	[TOKEN_DOT] = { NULL, NULL, PREC_NONE },
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
	[TOKEN_SUPER] = { NULL, NULL, PREC_NONE },
	[TOKEN_THIS] = { NULL, NULL, PREC_NONE },
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

static void advance()
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

static void declaration()
{
	if (match(TOKEN_FUN)) {
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
	// Mark variable as uninitialized
	local->depth = -1;
}

static void declare_variable()
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

static void mark_initialized()
{
	if (current->scopeDepth == 0)
		return;

	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void var_declaration()
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

static void fun_declaration()
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
	} else {
		arg = identifier_constant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(setOp, (uint8_t)arg);
	} else {
		emit_bytes(OP_GET_GLOBAL, arg);
	}
}

static void synchronize()
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

static bool check(TokenType type)
{
	return parser.current.type == type;
}

static bool match(TokenType type)
{
	if (!check(type))
		return false;
	advance();
	return true;
}

static Chunk *current_chunk()
{
	return &current->function->chunk;
}

// Define new block
static void block()
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// Enter new scope level
static void begin_scope()
{
	current->scopeDepth++;
}

// Exit new scope level
static void end_scope()
{
	current->scopeDepth--;

	// Pop out of scope variables from stack
	while (current->localCount > 0 &&
	       current->locals[current->localCount - 1].depth >
		       current->scopeDepth) {
		emit_byte(OP_POP);
		current->localCount--;
	}
}

static void statement()
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

static void expression_statement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emit_byte(OP_POP);
}

static void for_statement()
{
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	// Initializer clause
	if (match(TOKEN_SEMICOLON)) {
		// No initializer.
	} else if (match(TOKEN_VAR)) {
		var_declaration();
	} else {
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

static void if_statement()
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

static void print_statement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emit_byte(OP_PRINT);
}

static void return_statement()
{
	// Check for returning from main program
	if (current->type == TYPE_SCRIPT) {
		error("Can't return from top-level code.");
	}
	if (match(TOKEN_SEMICOLON)) {
		emit_return();
	} else {
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emit_byte(OP_RETURN);
	}
}

static void while_statement()
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
