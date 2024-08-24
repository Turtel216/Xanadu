// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#ifndef xanadu_scanner_h
#define xanadu_scanner_h

// Types of language tokens
typedef enum {
	// Single-character tokens.
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_MINUS,
	TOKEN_PLUS,
	TOKEN_SEMICOLON,
	TOKEN_SLASH,
	TOKEN_STAR,
	// One or two character tokens.
	TOKEN_BANG,
	TOKEN_BANG_EQUAL,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,
	// Literals.
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_NUMBER,
	// Keywords.
	TOKEN_AND,
	TOKEN_CLASS,
	TOKEN_ELSE,
	TOKEN_FALSE,
	TOKEN_FOR,
	TOKEN_FUN,
	TOKEN_IF,
	TOKEN_NIL,
	TOKEN_OR,
	TOKEN_PRINT,
	TOKEN_RETURN,
	TOKEN_SUPER,
	TOKEN_THIS,
	TOKEN_TRUE,
	TOKEN_VAR,
	TOKEN_WHILE,

	TOKEN_ERROR,
	TOKEN_EOF
} TokenType;

// Token's meta data
typedef struct {
	TokenType type; // The type of language token
	const char *start; // String containing token
	int length; // The lenght of the string
	int line; // Line number of token in source file
} Token;

// Start up scanner
void init_scanner(const char *source);
// Run scanner
Token scan_token();

#endif
