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

// The `Parser` struct holds the current and previous tokens, along with error and panic state.
typedef struct {
	Token current; // Current token being processed
	Token previous; // Previously processed token
	bool had_error; // Tracks if any errors occurred during parsing
	bool panic_mode; // Signals that the parser is in panic mode
} Parser;

// Enum defining operator precedence for Xanadu language.
typedef enum {
	PREC_NONE, // No precedence
	PREC_ASSIGNMENT, // =
	PREC_OR, // or
	PREC_AND, // and
	PREC_EQUALITY, // == !=
	PREC_COMPARISON, // < > <= >=
	PREC_TERM, // + -
	PREC_FACTOR, // * /
	PREC_UNARY, // ! -
	PREC_CALL, // . ()
	PREC_PRIMARY // Highest precedence (literals, etc.)
} Precedence;

// Function pointer type definition for parsing expressions.
typedef void (*ParseFn)(bool canAssign);

// Struct to define parsing rules, associating prefix/infix functions with operator precedence.
typedef struct {
	ParseFn prefix; // Function for parsing prefix expressions
	ParseFn infix; // Function for parsing infix expressions
	Precedence precedence; // Operator precedence level
} ParseRule;

// Struct representing a local variable's information.
typedef struct {
	Token name; // Name of the local variable
	int depth; // Depth of the variable's scope (scope level)
	bool isCaptured; // Whether the variable has been captured (closure)
} Local;

// Struct representing an upvalue in closures.
typedef struct {
	uint8_t index; // Index of the upvalue in the function
	bool isLocal; // Whether the upvalue is local
} Upvalue;

// Enum to track different function types.
typedef enum {
	TYPE_FUNCTION, // Normal function
	TYPE_SCRIPT, // Top-level script
	TYPE_METHOD, // Class method
	TYPE_INITIALIZER // Class initializer (constructor)
} FunctionType;

// Compiler struct that tracks the current compilation state.
typedef struct Compiler {
	struct Compiler *
		enclosing; // Pointer to the enclosing compiler (for nested scopes)
	ObjFunction *function; // The function being compiled
	FunctionType type; // The type of function being compiled
	Local locals[UINT8_COUNT]; // Array of local variables
	Upvalue upvalues[UINT8_COUNT]; // Array of upvalues for closures
	int localCount; // Number of local variables in the function
	int scopeDepth; // Current scope depth (for managing local variables)
} Compiler;

// ClassCompiler struct tracks the state of class compilation.
typedef struct ClassCompiler {
	struct ClassCompiler
		*enclosing; // Enclosing class compiler (for nested classes)
	bool has_super_class; // Whether the class has a superclass
} ClassCompiler;

// Global variables for the parser, compiler, and current chunk being compiled.
Parser parser;
Compiler *current = NULL;
ClassCompiler *currentClass = NULL;
Chunk *compiling_chunk; // Current chunk being compiled
//#################

// Function declarations (used throughout the file).
static void advance(void);
static void error_at_current(const char *message);
static void error(const char *message);
static void error_at(Token *token, const char *message);
static void consume(TokenType type, const char *message);

// Compilation-related function declarations.
static void expression(void);
static void emit_byte(uint8_t byte);
static Chunk *current_chunk(void);
static ObjFunction *end_compiler(void);
static void emit_return(void);
static void emit_bytes(uint8_t byte1, uint8_t byte2);
static void emit_loop(int loopStart);
static int emit_jump(uint8_t instruction);
static void emit_constant(Value value);
static void patch_jump(int offset);

// Xanadu function calls and expressions.
static void call(bool canAssign);
static void dot(bool canAssign);
static uint8_t make_constant(Value value);
static void unary(bool canAssign);
static void grouping(bool canAssign);
static void binary(bool canAssign);
static void string(bool canAssign);
static void number(bool canAssign);
static void parse_precedence(Precedence precedence);
static ParseRule *get_rule(TokenType type);
static void literal(bool canAssign);
static void declaration(void);
static void statement(void);
static void block(void);
static void begin_scope(void);
static void end_scope(void);
static uint8_t argument_list(void);
static void add_local(Token name);
static int add_upvalue(Compiler *compiler, uint8_t index, bool isLocal);
static void mark_initialized(void);
static bool check(TokenType type);
static bool match(TokenType type);
static void print_statement(void);
static void expression_statement(void);
static void if_statement(void);
static void while_statement(void);
static void for_statement(void);
static void return_statement(void);
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
static void variable(bool canAssign);
static Token synthetic_token(const char *text);
static void _this(bool canAssign);
static void named_variable(Token name, bool canAssign);
static int resolve_upvalue(Compiler *compiler, Token *name);
static void init_compiler(Compiler *compiler, FunctionType type);
static void and_(bool canAssign);
static void or_(bool canAssign);
//#####################

// Function definitions

// Initialize the compiler.
static void init_compiler(Compiler *compiler, FunctionType type)
{
	compiler->function = NULL;
	compiler->type = type;
	compiler->enclosing =
		current; // Link to the enclosing compiler for nested functions
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	compiler->function = new_function(); // Create a new function object
	current = compiler; // Update the current compiler reference

	// If we're compiling a function (not the main script), assign the function's name.
	if (type != TYPE_SCRIPT) {
		current->function->name = copy_string(parser.previous.start,
						      parser.previous.length);
	}

	// Reserve the first slot for the "this" reference in methods and initializers.
	Local *local = &current->locals[current->localCount++];
	local->depth = 0; // This variable is in the outermost scope.
	local->isCaptured = false; // Mark as not captured yet.

	// If compiling a method, assign the name "this"; otherwise, leave it empty.
	if (type != TYPE_FUNCTION) {
		local->name.start = "todays";
		local->name.length = 6;
	} else {
		local->name.start = "";
		local->name.length = 0;
	}
}

// Compile the source string into a function.
ObjFunction *compile(const char *source)
{
	init_scanner(source); // Tokenize the source string.

	Compiler compiler;
	init_compiler(
		&compiler,
		TYPE_SCRIPT); // Initialize the compiler for the top-level script.

	parser.had_error = false; // Reset the error state.
	parser.panic_mode = false;

	advance(); // Start the compilation by advancing to the first token.

	// Compile declarations until we reach the end of the file.
	while (!match(TOKEN_EOF)) {
		declaration();
	}

	// Finish the compilation and return the function.
	ObjFunction *function = end_compiler();
	return parser.had_error ? NULL : function;
}

// Mark values in the compiler's scope as roots (for garbage collection).
void mark_compiler_roots()
{
	Compiler *compiler = current;
	while (compiler != NULL) {
		mark_object((Obj *)compiler->function);
		compiler =
			compiler->enclosing; // Move to the enclosing compiler.
	}
}

// Compile an expression by following precedence rules.
static void expression(void)
{
	parse_precedence(
		PREC_ASSIGNMENT); // Start parsing expressions with assignment precedence.
}

// Complete the current compilation and return the compiled function.
static ObjFunction *end_compiler(void)
{
	emit_return(); // Emit return statement for the function.
	ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
	if (!parser.had_error) {
		disassemble_chunk(current_chunk(),
				  function->name != NULL ?
					  function->name->chars :
					  "<script>");
	}
#endif

	current = current->enclosing; // Pop the compiler off the stack.
	return function;
}

// Compile a binary expression based on the operator type.
static void binary(bool canAssign)
{
	TokenType operatorType = parser.previous.type; // Get the operator type.
	ParseRule *rule = get_rule(
		operatorType); // Get the parsing rule for the operator.
	parse_precedence((Precedence)(rule->precedence +
				      1)); // Parse the right-hand side.

	// Emit the corresponding bytecode based on the operator type.
	switch (operatorType) {
	case TOKEN_BANG_EQUAL:
		emit_bytes(OP_EQUAL, OP_NOT);
		break; // !=
	case TOKEN_EQUAL_EQUAL:
		emit_byte(OP_EQUAL);
		break; // ==
	case TOKEN_GREATER:
		emit_byte(OP_GREATER);
		break; // >
	case TOKEN_GREATER_EQUAL:
		emit_bytes(OP_LESS, OP_NOT);
		break; // >=
	case TOKEN_LESS:
		emit_byte(OP_LESS);
		break; // <
	case TOKEN_LESS_EQUAL:
		emit_bytes(OP_GREATER, OP_NOT);
		break; // <=
	case TOKEN_PLUS:
		emit_byte(OP_ADD);
		break; // +
	case TOKEN_MINUS:
		emit_byte(OP_SUBTRACT);
		break; // -
	case TOKEN_STAR:
		emit_byte(OP_MULTIPLY);
		break; // *
	case TOKEN_SLASH:
		emit_byte(OP_DIVIDE);
		break; // /
	default:
		return; // Unreachable.
	}
}

// Emit a function call, passing arguments to the function.
static void call(bool canAssign)
{
	uint8_t argCount =
		argument_list(); // Parse arguments and get their count.
	emit_bytes(OP_CALL, argCount); // Emit bytecode for the function call.
}

// Compile a field or method access using the dot operator.
static void dot(bool canAssign)
{
	consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
	uint8_t name = identifier_constant(&parser.previous);

	// Handle property assignment.
	if (canAssign && match(TOKEN_EQUAL)) {
		expression(); // Compile the right-hand side of the assignment.
		emit_bytes(OP_SET_PROPERTY,
			   name); // Emit bytecode to set the property.
	}
	// Handle method invocation.
	else if (match(TOKEN_LEFT_PAREN)) {
		uint8_t argCount =
			argument_list(); // Parse the method's arguments.
		emit_bytes(OP_INVOKE, name); // Invoke the method.
		emit_byte(argCount); // Emit the argument count.
	}
	// Handle property access.
	else {
		emit_bytes(OP_GET_PROPERTY,
			   name); // Emit bytecode to get the property.
	}
}

// Compile a literal value (e.g., true, false, nil).
static void literal(bool canAssign)
{
	switch (parser.previous.type) {
	case TOKEN_FALSE:
		emit_byte(OP_FALSE);
		break; // Compile `false` as bytecode.
	case TOKEN_NIL:
		emit_byte(OP_NIL);
		break; // Compile `nil` as bytecode.
	case TOKEN_TRUE:
		emit_byte(OP_TRUE);
		break; // Compile `true` as bytecode.
	default:
		return; // Unreachable.
	}
}

// Emit a return statement, handling methods with special behavior for initializers.
static void emit_return(void)
{
	// If the current function is an initializer, return `this`.
	if (current->type == TYPE_INITIALIZER) {
		emit_bytes(OP_GET_LOCAL, 0); // The first local (0) is `this`.
	} else {
		emit_byte(OP_NIL); // Otherwise, return `nil`.
	}

	emit_byte(OP_RETURN); // Emit the return bytecode.
}

// Emits bytecode for a loop operation in the VM.
// The loop begins at loopStart and wraps back to the current location.
static void emit_loop(int loopStart)
{
	emit_byte(OP_LOOP); // Emit the loop opcode.

	// Calculate the offset to jump back to loopStart.
	int offset = current_chunk()->count - loopStart + 2;

	// Ensure the offset fits within the maximum allowed size.
	if (offset > UINT16_MAX)
		error("Loop body too large.");

	// Emit the offset as two bytes.
	emit_byte((offset >> 8) & 0xff); // Higher byte.
	emit_byte(offset & 0xff); // Lower byte.
}

// Emits a jump instruction and reserves space for the jump offset.
// Returns the position in the bytecode where the offset needs to be patched later.
static int emit_jump(uint8_t instruction)
{
	emit_byte(instruction); // Emit the jump instruction.

	// Reserve two bytes for the offset (to be patched later).
	emit_byte(0xff);
	emit_byte(0xff);

	// Return the index of the first byte of the reserved offset.
	return current_chunk()->count - 2;
}

// Emits a constant value as bytecode by adding it to the constants table.
static void emit_constant(Value value)
{
	emit_bytes(
		OP_CONSTANT,
		make_constant(value)); // Emit constant instruction with index.
}

// Patches a previously emitted jump by updating its offset.
// The jump is relative to the position 'offset' in the bytecode.
static void patch_jump(int offset)
{
	// Calculate the jump distance from the offset to the current position.
	int jump = current_chunk()->count - offset - 2;

	// Ensure the jump fits within the maximum size.
	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}

	// Patch the two-byte offset in the bytecode.
	current_chunk()->code[offset] = (jump >> 8) & 0xff; // Higher byte.
	current_chunk()->code[offset + 1] = jump & 0xff; // Lower byte.
}

// Compiles a numeric literal into bytecode by converting the lexeme to a double.
static void number(bool can_assign)
{
	double value = strtod(parser.previous.start,
			      NULL); // Convert string to double.
	emit_constant(NUMBER_VAL(value)); // Emit the number as a constant.
}

// Compiles a string literal into bytecode by copying the string.
static void string(bool can_assign)
{
	// Copy the string (ignoring the quotes) and emit it as a constant.
	emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
					  parser.previous.length - 2)));
}

// Compiles a variable reference or assignment.
static void variable(bool can_assign)
{
	// Handle the variable by name, allowing assignment if possible.
	named_variable(parser.previous, can_assign);
}

// Creates a synthetic token from a given string (for internal use).
static Token synthetic_token(const char *text)
{
	Token token;
	token.start = text; // Set the start of the token to the string.
	token.length =
		(int)strlen(text); // Set the token length to the string length.
	return token;
}

// Compiles a 'super' method call, checking that it's within a class with a superclass.
static void super_(bool canAssign)
{
	// Ensure 'super' is used within a class that has a superclass.
	if (currentClass == NULL) {
		error("Can't use 'super' outside of a class.");
	} else if (!currentClass->has_super_class) {
		error("Can't use 'super' in a class with no superclass.");
	}

	// Expect a method call on 'super' (e.g., super.method()).
	consume(TOKEN_DOT, "Expect '.' after 'super'.");
	consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
	uint8_t name =
		identifier_constant(&parser.previous); // Get method name.

	// Check if it's a method call or property access.
	named_variable(synthetic_token("this"),
		       false); // Load 'this' before method call.
	if (match(TOKEN_LEFT_PAREN)) {
		uint8_t argCount =
			argument_list(); // Parse the method arguments.
		named_variable(synthetic_token("super"),
			       false); // Load 'super'.
		emit_bytes(OP_SUPER_INVOKE,
			   name); // Emit the super method invocation.
		emit_byte(argCount); // Emit argument count.
	} else {
		named_variable(synthetic_token("super"),
			       false); // Load 'super'.
		emit_bytes(OP_GET_SUPER,
			   name); // Emit bytecode for property access.
	}
}

// Compiles the 'this' keyword within a class, ensuring it's used correctly.
static void _this(bool canAssign)
{
	// Ensure 'this' is used inside a class.
	if (currentClass == NULL) {
		error("Can't use 'this' outside of a class");
		return;
	}

	// Treat 'this' as a variable and emit the corresponding bytecode.
	variable(false);
}

// Compiles a function declaration, including its parameters and body.
static void function(FunctionType type)
{
	Compiler compiler;
	init_compiler(&compiler,
		      type); // Initialize a new compiler for the function.
	begin_scope(); // Begin a new local scope.

	// Parse the function parameters.
	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if (!check(TOKEN_RIGHT_PAREN)) { // Parse each parameter.
		do {
			current->function
				->arity++; // Increase the function's arity.
			if (current->function->arity > 255) {
				error_at_current(
					"Can't have more than 255 parameters.");
			}
			uint8_t constant = parse_variable(
				"Expect parameter name."); // Parse parameter name.
			define_variable(constant); // Define the parameter.
		} while (match(
			TOKEN_COMMA)); // Continue parsing parameters if there are commas.
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

	// Parse the function body.
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();

	ObjFunction *function = end_compiler(); // End function compilation.
	emit_bytes(
		OP_CLOSURE,
		make_constant(OBJ_VAL(function))); // Emit the closure bytecode.

	// Emit upvalue data for the closure.
	for (int i = 0; i < function->upvalueCount; i++) {
		emit_byte(compiler.upvalues[i].isLocal ?
				  1 :
				  0); // Emit whether the upvalue is local.
		emit_byte(
			compiler.upvalues[i].index); // Emit the upvalue's index.
	}
}

// Compiles a method declaration within a class.
static void method()
{
	consume(TOKEN_IDENTIFIER, "Expect method name.");
	uint8_t constant = identifier_constant(
		&parser.previous); // Get the method name constant.

	// Check if this method is an initializer (constructor).
	FunctionType type = TYPE_METHOD;
	if (parser.previous.length == 4 &&
	    memcmp(parser.previous.start, "init", 4) == 0) {
		type = TYPE_INITIALIZER;
	}

	// Compile the method as a function.
	function(type);
	emit_bytes(OP_METHOD, constant); // Emit the method definition bytecode.
}

// Compiles a class declaration, including inheritance, methods, and properties.
static void class_declaration()
{
	consume(TOKEN_IDENTIFIER, "Expect class name.");
	Token className = parser.previous; // Save the class name token.
	uint8_t nameConstant = identifier_constant(
		&parser.previous); // Create a constant for the class name.
	declare_variable(); // Declare the class name in the current scope.

	emit_bytes(OP_CLASS,
		   nameConstant); // Emit bytecode to create the class.
	define_variable(nameConstant); // Define the class variable.

	// Set up class inheritance.
	ClassCompiler classCompiler;
	classCompiler.has_super_class = false;
	classCompiler.enclosing = currentClass; // Save the enclosing class.
	currentClass = &classCompiler; // Set the current class.

	// If the class extends a superclass, handle inheritance.
	if (match(TOKEN_EXTENDS)) {
		consume(TOKEN_IDENTIFIER, "Expected superclass name.");
		variable(false); // Load the superclass name.

		// Ensure the class doesn't inherit from itself.
		if (identifiers_equal(&className, &parser.previous)) {
			error("A class can't inherit from itself.");
		}

		begin_scope(); // Create a new scope for the superclass.
		add_local(synthetic_token(
			"super")); // Declare 'super' as a local variable.
		define_variable(0);

		named_variable(className, false); // Load the class name.
		emit_byte(OP_INHERIT); // Emit bytecode for inheritance.
		classCompiler.has_super_class = true;
	}

	// Compile the class body.
	named_variable(className, false); // Load the class name.
	consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		method(); // Compile each method.
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
	emit_byte(OP_POP); // Pop the class off the stack.

	// If the class has a superclass, end its scope.
	if (classCompiler.has_super_class) {
		end_scope();
	}

	// Restore the enclosing class.
	currentClass = currentClass->enclosing;
}

// Convert a value to a constant index within a chunk. Returns the index as a uint8_t.
// If the constant exceeds the maximum (UINT8_MAX), an error is reported.
static uint8_t make_constant(Value value)
{
	int constant = add_constant(current_chunk(), value);
	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0; // Returns 0 in case of error.
	}

	return (uint8_t)
		constant; // Returns the constant index as an 8-bit value.
}

// Parses a grouping expression, such as an expression enclosed in parentheses (e.g., (x + y)).
// It ensures that a closing ')' follows the expression.
static void grouping(bool can_assign)
{
	expression(); // Parse the enclosed expression.
	consume(TOKEN_RIGHT_PAREN,
		"Expect ')' after expression."); // Ensure the closing ')'.
}

// Parses a unary operation (e.g., -x). It processes the operand and emits the corresponding bytecode.
static void unary(bool can_assign)
{
	TokenType operatorType = parser.previous.type;

	// Parse the operand after the unary operator.
	parse_precedence(PREC_UNARY);

	// Emit the bytecode instruction based on the operator type.
	switch (operatorType) {
	case TOKEN_MINUS:
		emit_byte(OP_NEGATE); // Negate the operand (e.g., -x).
		break;
	default:
		return; // Unreachable case, should never occur.
	}
}

// Parsing rules for each token. Each rule specifies a function to handle prefix and infix expressions,
// as well as the precedence level for parsing.
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN] = { grouping, call,
			       PREC_CALL }, // Grouping or function call
	[TOKEN_RIGHT_PAREN] = { NULL, NULL,
				PREC_NONE }, // Right parenthesis (no action)
	[TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE }, // Left brace
	[TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE }, // Right brace
	[TOKEN_COMMA] = { NULL, NULL, PREC_NONE }, // Comma
	[TOKEN_DOT] = { NULL, dot,
			PREC_CALL }, // Dot operator for object property access
	[TOKEN_MINUS] = { unary, binary,
			  PREC_TERM }, // Minus for unary negation or subtraction
	[TOKEN_PLUS] = { NULL, binary, PREC_TERM }, // Plus for addition
	[TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE }, // Semicolon
	[TOKEN_SLASH] = { NULL, binary, PREC_FACTOR }, // Division operator
	[TOKEN_STAR] = { NULL, binary, PREC_FACTOR }, // Multiplication operator
	[TOKEN_BANG] = { unary, NULL, PREC_NONE }, // Logical NOT (unary)
	[TOKEN_BANG_EQUAL] = { NULL, binary,
			       PREC_EQUALITY }, // Not equal operator
	[TOKEN_EQUAL] = { NULL, NULL, PREC_NONE }, // Assignment operator
	[TOKEN_EQUAL_EQUAL] = { NULL, binary,
				PREC_EQUALITY }, // Equal comparison
	[TOKEN_GREATER] = { NULL, binary,
			    PREC_COMPARISON }, // Greater than comparison
	[TOKEN_GREATER_EQUAL] = { NULL, binary,
				  PREC_COMPARISON }, // Greater or equal comparison
	[TOKEN_LESS] = { NULL, binary,
			 PREC_COMPARISON }, // Less than comparison
	[TOKEN_LESS_EQUAL] = { NULL, binary,
			       PREC_COMPARISON }, // Less or equal comparison
	[TOKEN_IDENTIFIER] = { variable, NULL,
			       PREC_NONE }, // Identifier for variables
	[TOKEN_STRING] = { string, NULL, PREC_NONE }, // String literal
	[TOKEN_NUMBER] = { number, NULL, PREC_NONE }, // Number literal
	[TOKEN_AND] = { NULL, and_, PREC_AND }, // Logical AND
	[TOKEN_CLASS] = { NULL, NULL, PREC_NONE }, // Class declaration
	[TOKEN_ELSE] = { NULL, NULL,
			 PREC_NONE }, // Else branch in an if statement
	[TOKEN_FALSE] = { literal, NULL, PREC_NONE }, // False literal
	[TOKEN_FOR] = { NULL, NULL, PREC_NONE }, // For loop
	[TOKEN_FUN] = { NULL, NULL, PREC_NONE }, // Function declaration
	[TOKEN_IF] = { NULL, NULL, PREC_NONE }, // If statement
	[TOKEN_NIL] = { literal, NULL, PREC_NONE }, // Nil (null) literal
	[TOKEN_OR] = { NULL, or_, PREC_OR }, // Logical OR
	[TOKEN_PRINT] = { NULL, NULL, PREC_NONE }, // Print statement
	[TOKEN_RETURN] = { NULL, NULL, PREC_NONE }, // Return statement
	[TOKEN_SUPER] = { super_, NULL, PREC_NONE }, // Super class access
	[TOKEN_THIS] = { _this, NULL, PREC_NONE }, // "This" keyword
	[TOKEN_TRUE] = { literal, NULL, PREC_NONE }, // True literal
	[TOKEN_VAR] = { NULL, NULL, PREC_NONE }, // Variable declaration
	[TOKEN_WHILE] = { NULL, NULL, PREC_NONE }, // While loop
	[TOKEN_ERROR] = { NULL, NULL, PREC_NONE }, // Error token
	[TOKEN_EOF] = { NULL, NULL, PREC_NONE }, // End of file
};

// Parses expressions based on their precedence levels.
static void parse_precedence(Precedence precedence)
{
	advance(); // Advance to the next token.

	// Get the rule for the current token's prefix (e.g., handling unary operations).
	ParseFn prefixRule = get_rule(parser.previous.type)->prefix;
	if (prefixRule == NULL) {
		error("Expect expression."); // Error if no prefix rule exists for the token.
		return;
	}

	bool can_assign =
		precedence <=
		PREC_ASSIGNMENT; // Determine if the expression can be assigned.
	prefixRule(can_assign); // Parse the prefix expression.

	// Parse infix operators with higher precedence levels.
	while (precedence <= get_rule(parser.current.type)->precedence) {
		advance(); // Move to the infix operator.
		ParseFn infixRule = get_rule(parser.previous.type)->infix;
		infixRule(can_assign); // Parse the infix expression.
	}

	// Check for invalid assignments (e.g., attempting to assign to a non-variable).
	if (can_assign && match(TOKEN_EQUAL)) {
		error("Invalid assignment target.");
	}
}

// Retrieves the parsing rule for a specific token type.
static ParseRule *get_rule(TokenType type)
{
	return &rules[type];
}

// Advances to the next token in the input stream, handling any errors encountered.
static void advance(void)
{
	parser.previous = parser.current;

	// Skip over error tokens and continue until a valid token is found.
	for (;;) {
		parser.current =
			scan_token(); // Get the next token from the scanner.
		if (parser.current.type != TOKEN_ERROR) // Ignore error tokens.
			break;

		error_at_current(
			parser.current
				.start); // Report errors at the current token.
	}
}

// Reports an error at the current token.
static void error_at_current(const char *message)
{
	error_at(&parser.current, message);
}

// Reports an error at a specific token.
static void error(const char *message)
{
	error_at(&parser.previous, message);
}

// Displays an error message at a specific token, showing the line number and token information.
static void error_at(Token *token, const char *message)
{
	if (parser.panic_mode)
		return;

	parser.panic_mode =
		true; // Enter panic mode, suppressing further error reports.

	// Display the error location and message.
	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end"); // Error at the end of file.
	} else if (token->type == TOKEN_ERROR) {
		// No additional information needed for error tokens.
	} else {
		fprintf(stderr, " at '%.*s'", token->length,
			token->start); // Show the specific token.
	}

	fprintf(stderr, ": %s\n", message); // Display the error message.
	parser.had_error = true; // Mark that an error occurred.
}

// Consumes a token if it matches the expected type, or reports an error if it doesn't.
static void consume(TokenType type, const char *message)
{
	if (parser.current.type == type) {
		advance(); // Advance if the token matches the expected type.
		return;
	}

	error_at_current(
		message); // Report an error if the token doesn't match.
}

// Handles variable declarations and manages scoping.
static void declare_variable(void)
{
	if (current->scopeDepth == 0)
		return; // Global variables don't require further declaration.

	Token *name = &parser.previous;

	// Check for redeclarations within the same scope.
	for (int i = current->localCount - 1; i >= 0; i--) {
		Local *local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scopeDepth) {
			break; // Stop checking if the variable is declared in an outer scope.
		}

		if (identifiers_equal(name, &local->name)) {
			error("Variable with this name already declared in this scope.");
		}
	}

	add_local(*name); // Add the variable to the list of locals.
}

// Parses a declaration statement.
// Depending on the token type, it will call the appropriate declaration handler
// (class, function, or variable). If no declaration is found, it processes the statement.
// In case of a parsing error, it will attempt to recover by calling synchronize().
static void declaration(void)
{
	// Check for a class declaration and handle it
	if (match(TOKEN_CLASS)) {
		class_declaration();
	}
	// Check for a function declaration and handle it
	else if (match(TOKEN_FUN)) {
		fun_declaration();
	}
	// Check for a variable declaration and handle it
	else if (match(TOKEN_VAR)) {
		var_declaration();
	}
	// If none of the above, handle it as a general statement
	else {
		statement();
	}

	// If the parser is in panic mode due to an error, synchronize to recover
	if (parser.panic_mode)
		synchronize();
}

// Compares two identifier tokens for equality.
// Returns true if the identifiers have the same length and contents, otherwise returns false.
static bool identifiers_equal(Token *a, Token *b)
{
	if (a->length !=
	    b->length) // If the lengths are different, they can't be equal
		return false;
	// Compare the actual identifier strings by memory
	return memcmp(a->start, b->start, a->length) == 0;
}

// Adds a new local variable to the local variable array.
// The variable is initialized with default values and an error is thrown
// if the maximum local variable limit is exceeded.
static void add_local(Token name)
{
	// Check if the maximum number of local variables (UINT8_COUNT) is reached
	if (current->localCount == UINT8_COUNT) {
		error("Too many local variables in function."); // Throw error if limit is exceeded
		return;
	}

	// Get a pointer to the next available slot in the locals array and increment localCount
	Local *local = &current->locals[current->localCount++];
	local->name = name; // Assign the variable's name
	local->isCaptured = false; // Initialize as not captured by a closure
	local->depth =
		-1; // Mark as uninitialized, meaning it's in the process of being defined
}

// Define a variable either globally or in the current local scope.
// If inside a local scope, mark the variable as initialized.
// Otherwise, define the variable globally by emitting the appropriate bytecode.
static void define_variable(uint8_t global)
{
	if (current->scopeDepth > 0) {
		mark_initialized(); // Mark the variable as initialized in the current scope.
		return;
	}
	emit_bytes(OP_DEFINE_GLOBAL,
		   global); // Emit bytecode to define a global variable.
}

// Parse a list of function call arguments.
// Continues parsing expressions separated by commas until ')' is encountered.
// Returns the number of arguments parsed (max 255).
static uint8_t argument_list()
{
	uint8_t argCount = 0;
	if (!check(TOKEN_RIGHT_PAREN)) { // Check for the end of the argument list.
		do {
			expression(); // Parse the next argument.
			if (argCount == 255) {
				error("Can't have more than 255 arguments."); // Error if too many arguments.
			}
			argCount++; // Increment the argument count.
		} while (match(
			TOKEN_COMMA)); // Continue parsing until a comma is not found.
	}
	consume(TOKEN_RIGHT_PAREN,
		"Expect ')' after arguments."); // Ensure a closing parenthesis.
	return argCount; // Return the number of arguments parsed.
}

// Parse and compile a logical 'and' expression.
// If the left-hand side is false, skip over the right-hand side using a jump.
static void and_(bool canAssign)
{
	int endJump = emit_jump(
		OP_JUMP_IF_FALSE); // Emit jump if the left-hand side is false.

	emit_byte(OP_POP); // Discard the left-hand side result if true.
	parse_precedence(PREC_AND); // Parse the right-hand side.

	patch_jump(endJump); // Patch the jump to continue execution.
}

// Parse and compile a logical 'or' expression.
// If the left-hand side is false, evaluate the right-hand side.
static void or_(bool can_assign)
{
	int elseJump = emit_jump(
		OP_JUMP_IF_FALSE); // Emit jump if left-hand side is false.
	int endJump = emit_jump(
		OP_JUMP); // Jump to the end if the left-hand side is true.

	patch_jump(elseJump); // Patch the false jump.
	emit_byte(OP_POP); // Discard the left-hand side result if false.

	parse_precedence(PREC_OR); // Parse the right-hand side.
	patch_jump(
		endJump); // Patch the end jump to complete the 'or' evaluation.
}

// Parse and compile a variable declaration, resolving its scope (local/global).
// If it's global, return the index in the constant pool; otherwise, mark it locally.
static uint8_t parse_variable(const char *errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage); // Ensure a valid identifier.

	declare_variable(); // Declare the variable in the current scope.
	if (current->scopeDepth > 0)
		return 0; // Return 0 for locals (no need for global index).

	return identifier_constant(
		&parser.previous); // Return the constant index for global variables.
}

// Mark the most recently declared local variable as initialized.
// This is used to track whether a variable is ready to be used in the current scope.
static void mark_initialized(void)
{
	if (current->scopeDepth == 0)
		return; // Skip marking for global scope.

	// Mark the last local variable as initialized by setting its depth.
	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// Parse and compile a variable declaration statement (e.g., `var x = 10;`).
// Handles both initialization and default assignment (i.e., `nil`).
static void var_declaration(void)
{
	uint8_t global = parse_variable(
		"Expect variable name."); // Parse the variable name.

	if (match(TOKEN_EQUAL)) {
		expression(); // Parse the initializer expression.
	} else {
		emit_byte(OP_NIL); // Default to `nil` if no initializer.
	}
	consume(TOKEN_SEMICOLON,
		"Expect ';' after variable declaration."); // Ensure the declaration ends with a semicolon.

	define_variable(
		global); // Define the variable in the appropriate scope.
}

// Parse and compile a function declaration statement (e.g., `fun foo() {...}`).
static void fun_declaration(void)
{
	uint8_t global = parse_variable(
		"Expect function name."); // Parse the function name.
	mark_initialized(); // Mark the function name as initialized in the current scope.

	function(TYPE_FUNCTION); // Parse and compile the function body.
	define_variable(global); // Define the function globally or locally.
}

// Add a string identifier to the constant pool and return its index.
// This is used to reference variables, functions, etc., by their names.
static uint8_t identifier_constant(Token *name)
{
	return make_constant(OBJ_VAL(copy_string(
		name->start,
		name->length))); // Convert the identifier to a constant.
}

// Resolve a local variable by searching in the current compiler's local list.
// Returns the index of the local variable or -1 if not found.
static int resolve_local(Compiler *compiler, Token *name)
{
	for (int i = compiler->localCount - 1; i >= 0; --i) {
		Local *local = &compiler->locals[i];
		if (identifiers_equal(name, &local->name)) {
			if (local->depth == -1) {
				error("Can't read local variable in its own initializer."); // Error if trying to access uninitialized variable.
			}
			return i; // Return the index of the local variable.
		}
	}

	return -1; // Return -1 if the variable is not found.
}

// Compile a named variable access or assignment.
// If the variable is in the local or global scope, emit the appropriate bytecode.
static void named_variable(Token name, bool can_assign)
{
	uint8_t getOp, setOp;
	int arg = resolve_local(current, &name);

	if (arg != -1) {
		getOp = OP_GET_LOCAL; // Local variable.
		setOp = OP_SET_LOCAL;
	} else if ((arg = resolve_upvalue(current, &name)) != -1) {
		getOp = OP_GET_UPVALUE; // Upvalue (captured variable from an enclosing function).
		setOp = OP_SET_UPVALUE;
	} else {
		arg = identifier_constant(&name); // Global variable.
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (can_assign && match(TOKEN_EQUAL)) {
		expression(); // Compile the right-hand side of the assignment.
		emit_bytes(
			setOp,
			(uint8_t)arg); // Emit the appropriate bytecode for assignment.
	} else {
		emit_bytes(
			getOp,
			(uint8_t)arg); // Emit bytecode to load the variable's value.
	}
}

// Add an upvalue (captured variable) to the current function.
// Returns the index of the upvalue or reuses an existing one if it was already added.
static int add_upvalue(Compiler *compiler, uint8_t index, bool isLocal)
{
	int upvalueCount = compiler->function->upvalueCount;

	// Check if the upvalue was already added and return its index if found.
	for (int i = 0; i < upvalueCount; i++) {
		Upvalue *upvalue = &compiler->upvalues[i];
		if (upvalue->index == index && upvalue->isLocal == isLocal) {
			return i;
		}
	}

	// Error if too many upvalues are added.
	if (upvalueCount == UINT8_COUNT) {
		error("Too many closure variables in function.");
		return 0;
	}

	// Add the new upvalue.
	compiler->upvalues[upvalueCount].isLocal = isLocal;
	compiler->upvalues[upvalueCount].index = index;
	return compiler->function->upvalueCount++;
}

// Resolve an upvalue by searching the enclosing function's local variables or upvalues.
// Returns the index of the upvalue or -1 if not found.
static int resolve_upvalue(Compiler *compiler, Token *name)
{
	if (compiler->enclosing == NULL)
		return -1; // No enclosing function to search in.

	int local = resolve_local(compiler->enclosing, name);
	if (local != -1) {
		compiler->enclosing->locals[local].isCaptured =
			true; // Mark the local variable as captured.
		return add_upvalue(
			compiler, (uint8_t)local,
			true); // Add the local variable as an upvalue.
	}

	int upvalue = resolve_upvalue(compiler->enclosing, name);
	if (upvalue != -1) {
		return add_upvalue(
			compiler, (uint8_t)upvalue,
			false); // Add the upvalue from the enclosing function.
	}

	return -1; // Return -1 if the upvalue is not found.
}

// Synchronizes the parser after a panic (error recovery).
// It advances through tokens until it finds a point where it can resume parsing
// (e.g., at a statement boundary like `;` or keywords that begin a new statement).
static void synchronize(void)
{
	parser.panic_mode = false;

	while (parser.current.type != TOKEN_EOF) { // Loop until end of file
		if (parser.previous.type == TOKEN_SEMICOLON)
			return; // Stop at the end of a statement
		switch (parser.current.type) {
		// Stop at keywords that mark the start of a new statement or block
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;

		default:
			// Do nothing, continue advancing
			break;
		}
		advance(); // Move to the next token
	}
}

// Checks if the current token matches the specified type.
static bool check(TokenType type)
{
	return parser.current.type == type;
}

// Checks if the current token matches the expected type.
// If it does, the parser advances to the next token and returns true.
// Otherwise, it returns false.
static bool match(TokenType type)
{
	if (!check(type)) // If token doesn't match
		return false;

	advance(); // Move to the next token if matched
	return true;
}

// Returns the current chunk of bytecode being compiled.
static Chunk *current_chunk(void)
{
	return &current->function->chunk;
}

// Parses a block of code between braces `{ }`.
// This continues parsing declarations inside the block until it encounters
// the closing brace `}` or reaches the end of the file.
static void block(void)
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration(); // Parse each declaration inside the block
	}

	consume(TOKEN_RIGHT_BRACE,
		"Expect '}' after block."); // Ensure block ends with a closing brace
}

// Increases the scope depth when entering a new block or scope.
static void begin_scope(void)
{
	current->scopeDepth++;
}

// Decreases the scope depth when exiting a block or scope.
// Variables declared in the scope are popped off the stack.
static void end_scope(void)
{
	current->scopeDepth--; // Exit current scope

	// Pop variables from the stack that were declared in this scope
	while (current->localCount > 0 &&
	       current->locals[current->localCount - 1].depth >
		       current->scopeDepth) {
		// If the variable is captured by a closure, close the upvalue
		if (current->locals[current->localCount - 1].isCaptured) {
			emit_byte(OP_CLOSE_UPVALUE);
		} else {
			emit_byte(OP_POP); // Otherwise, just pop the variable
		}
		current->localCount--;
	}
}

// Compiles a single statement based on the current token.
static void statement(void)
{
	if (match(TOKEN_PRINT)) {
		print_statement(); // Handle print statement
	} else if (match(TOKEN_LEFT_BRACE)) {
		begin_scope(); // Start a new block scope
		block(); // Compile the block
		end_scope(); // End the block scope
	} else if (match(TOKEN_IF)) {
		if_statement(); // Compile an if statement
	} else if (match(TOKEN_FOR)) {
		for_statement(); // Compile a for loop
	} else if (match(TOKEN_RETURN)) {
		return_statement(); // Compile a return statement
	} else if (match(TOKEN_WHILE)) {
		while_statement(); // Compile a while loop
	} else {
		expression_statement(); // Default to compiling an expression statement
	}
}

// Compiles an expression statement, which evaluates an expression
// and discards the result. It must end with a semicolon.
static void expression_statement(void)
{
	expression(); // Compile the expression
	consume(TOKEN_SEMICOLON,
		"Expect ';' after expression."); // Ensure semicolon
	emit_byte(OP_POP); // Discard the result of the expression
}

// Compiles a for loop.
static void for_statement(void)
{
	begin_scope(); // New scope for the loop

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	// Handle the initializer clause.
	if (match(TOKEN_SEMICOLON)) {
		// No initializer
	} else if (match(TOKEN_VAR)) {
		var_declaration(); // Parse variable declaration in the initializer
	} else {
		expression_statement(); // Handle regular expression in the initializer
	}

	int loopStart = current_chunk()->count; // Mark the start of the loop

	// Parse the condition clause.
	int exitJump = -1;
	if (!match(TOKEN_SEMICOLON)) {
		expression(); // Parse loop condition
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
		exitJump = emit_jump(
			OP_JUMP_IF_FALSE); // Jump out if condition is false
		emit_byte(OP_POP); // Pop condition result
	}

	// Parse the increment clause.
	if (!match(TOKEN_RIGHT_PAREN)) {
		int bodyJump = emit_jump(OP_JUMP); // Jump to loop body
		int incrementStart = current_chunk()->count;
		expression(); // Parse increment expression
		emit_byte(OP_POP); // Discard result of increment expression
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

		emit_loop(loopStart); // Jump back to the start of the loop
		loopStart = incrementStart;
		patch_jump(bodyJump); // Jump back to the increment section
	}

	statement(); // Compile the loop body
	emit_loop(loopStart); // Jump to the start of the loop

	// Patch exit jump if there's a condition.
	if (exitJump != -1) {
		patch_jump(exitJump);
		emit_byte(OP_POP); // Pop condition result
	}

	end_scope(); // End the scope
}

// Compiles an if statement.
static void if_statement(void)
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression(); // Compile the condition expression
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int thenJump = emit_jump(
		OP_JUMP_IF_FALSE); // Jump to else if condition is false
	emit_byte(OP_POP); // Pop condition result
	statement(); // Compile the "then" branch

	int elseJump = emit_jump(OP_JUMP); // Jump over the else branch
	patch_jump(thenJump); // Patch the jump to the else branch
	emit_byte(OP_POP); // Pop the result if condition was false

	if (match(TOKEN_ELSE)) {
		statement(); // Compile the "else" branch
	}

	patch_jump(elseJump); // Patch the jump to the end of the statement
}

// Compiles a print statement that evaluates an expression
// and writes the result to standard output.
static void print_statement(void)
{
	expression(); // Compile the expression to print
	consume(TOKEN_SEMICOLON, "Expect ';' after value."); // Ensure semicolon
	emit_byte(OP_PRINT); // Emit the bytecode for print
}

// Compiles a return statement.
static void return_statement(void)
{
	// Return statements are not allowed at the top level.
	if (current->type == TYPE_SCRIPT) {
		error("Can't return from top-level code.");
	}
	if (match(TOKEN_SEMICOLON)) {
		emit_return(); // Emit return with no value
	} else {
		if (current->type == TYPE_INITIALIZER) {
			error("Can't return a value from an initializer.");
		}
		expression(); // Compile the return value expression
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emit_byte(OP_RETURN); // Emit return with value
	}
}

// Compiles a while loop.
static void while_statement(void)
{
	int loop_start =
		current_chunk()->count; // Mark the loop's start position

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression(); // Compile the condition expression
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	int exitJump = emit_jump(
		OP_JUMP_IF_FALSE); // Jump out of loop if condition is false
	emit_byte(OP_POP); // Pop condition result
	statement(); // Compile the loop body

	emit_loop(loop_start); // Jump back to the start of the loop

	patch_jump(exitJump); // Patch the jump to the end of the loop
	emit_byte(OP_POP); // Pop condition result
}

// Emits a single byte of bytecode to the current chunk.
static void emit_byte(uint8_t byte)
{
	write_chunk(current_chunk(), byte,
		    parser.previous.line); // Write the byte with line info
}

// Emits multiple bytes of bytecode to the current chunk.
static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
	emit_byte(byte1);
	emit_byte(byte2);
}

//#####################
