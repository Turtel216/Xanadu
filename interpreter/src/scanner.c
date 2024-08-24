// copyright 2024 dimitrios papakonstantinou. all rights reserved.
// use of this source code is governed by a MIT
// license that can be found in the license file.

#include <string.h>
#include <stdbool.h>

#include "scanner.h"

// Lexer's meta data
typedef struct {
	const char *start; // Start position
	const char *current; // Current position on string
	int line; // line number in source file
} Scanner;

// Global scanner struct
Scanner scanner;

// Start up scanner
void init_scanner(const char *source)
{
	// initialize scanner meta data
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

// Check if at the end of source string
static inline bool is_at_end()
{
	return *scanner.current == '\0';
}

// Initialize and return a new Token struct
static Token make_token(TokenType type)
{
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.length = (int)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

// Initialize and return a new Token struct of type error
static Token error_token(const char *message)
{
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}

// Get next character
static char advance()
{
	scanner.current++;
	return scanner.current[-1];
}

// Check if current char is the expected char
static bool match(char expected)
{
	if (is_at_end())
		return false;

	if (*scanner.current != expected)
		return false;

	scanner.current++;
	return true;
}

// Get current char
static char peek()
{
	return *scanner.current;
}

// Get next char
static char peek_next()
{
	// if at end return string termination character
	if (is_at_end())
		return '\0';

	return scanner.current[1];
}

// Skip all kinds of whitespace of source string
static void skip_whitespace()
{
	for (;;) {
		char c = peek();
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n':
			scanner.line++;
			advance();
			break;
		case '/': // Comments
			if (peek_next() == '/') {
				// A comment goes until the end of the line.
				while (peek() != '\n' && !is_at_end())
					advance();
			} else {
				return;
			}
			break;
		default:
			return;
		}
	}
}

// Get string type
static Token string()
{
	while (peek() != '"' && !is_at_end()) {
		if (peek() == '\n')
			scanner.line++;
		advance();
	}

	if (is_at_end())
		return error_token("Unterminated string.");

	// The closing quote.
	advance();
	return make_token(TOKEN_STRING);
}

// Check if character is a digit
static bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

// Get number type
static Token number()
{
	while (is_digit(peek()))
		advance();

	// Look for a fractional part.
	if (peek() == '.' && is_digit(peek_next())) {
		// Consume the ".".
		advance();

		while (is_digit(peek()))
			advance();
	}

	return make_token(TOKEN_NUMBER);
}

// Check if character is alphanumeric
static inline bool is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Check if current string is the rest of a keyword
static TokenType check_keyword(int start, int length, const char *rest,
			       TokenType type)
{
	if (scanner.current - scanner.start == start + length &&
	    memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}

	return TOKEN_IDENTIFIER;
}

//TODO add all identifiers and fix start/length
static TokenType identifier_type()
{
	switch (scanner.start[0]) {
	case 'a':
		return check_keyword(1, 2, "nd", TOKEN_AND);
	case 'O':
		return check_keyword(1, 4, "vertune", TOKEN_CLASS);
	case 'f':
		return check_keyword(1, 7, "reewill", TOKEN_IF);
	case 'c':
		if (scanner.current - scanner.start > 1) {
			switch (scanner.start[1]) {
			case 'y':
				return check_keyword(2, 4, "gnus", TOKEN_NIL);
			case 'o':
				return check_keyword(2, 10, "unterpoint",
						     TOKEN_ELSE);
			case 'i':
				return check_keyword(2, 11, "rcumstances",
						     TOKEN_FOR);
			}
		}
		break;
	case 'o':
		if (scanner.current - scanner.start > 1) {
			switch (scanner.start[1]) {
			case 'a':
				return check_keyword(1, 1, "r", TOKEN_OR);
			case 'v':
				return check_keyword(1, 4, "ertune",
						     TOKEN_CLASS);
			}
		}
		break;
	case 'b':
		return check_keyword(1, 5, "labla", TOKEN_PRINT);
	case 'l':
		return check_keyword(1, 8, "imelight", TOKEN_RETURN);
	case 's':
		return check_keyword(1, 10, "ubdivision", TOKEN_FUN);
	case 'y':
		return check_keyword(1, 2, "yz", TOKEN_VAR);
	case 'w':
		return check_keyword(1, 16, "orkingmans_grind", TOKEN_WHILE);
	case 't':
		if (scanner.current - scanner.start > 1) {
			switch (scanner.start[1]) {
			case 'h':
				return check_keyword(2, 2, "is", TOKEN_THIS);
			case 'r':
				return check_keyword(2, 2, "ue", TOKEN_TRUE);
			}
		}
		break;
	}
	return TOKEN_IDENTIFIER;
}

static Token identifier()
{
	while (is_alpha(peek()) || is_digit(peek()))
		advance();
	return make_token(identifier_type());
}

// Run scanner
Token scan_token()
{
	skip_whitespace();
	scanner.start = scanner.current;

	if (is_at_end())
		return make_token(TOKEN_EOF);

	char c = advance();

	if (is_alpha(c))
		return identifier();
	if (is_digit(c))
		return number();

	switch (c) {
	case '(':
		return make_token(TOKEN_LEFT_PAREN);
	case ')':
		return make_token(TOKEN_RIGHT_PAREN);
	case '{':
		return make_token(TOKEN_LEFT_BRACE);
	case '}':
		return make_token(TOKEN_RIGHT_BRACE);
	case ';':
		return make_token(TOKEN_SEMICOLON);
	case ',':
		return make_token(TOKEN_COMMA);
	case '.':
		return make_token(TOKEN_DOT);
	case '-':
		return make_token(TOKEN_MINUS);
	case '+':
		return make_token(TOKEN_PLUS);
	case '/':
		return make_token(TOKEN_SLASH);
	case '*':
		return make_token(TOKEN_STAR);
	case '!':
		return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
	case '=':
		return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
	case '<':
		return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
	case '>':
		return make_token(match('=') ? TOKEN_GREATER_EQUAL :
					       TOKEN_GREATER);
	case '"':
		return string();
	}

	return error_token("Unexpected character.");
}
