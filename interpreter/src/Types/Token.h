#pragma once

#include "Literal.h"
#include <string>

namespace xanadu::Tokens {

using OptionalLiteral = xanadu::Types::OptionalLiteral;

enum TokenType {
  // Single-character tokens.
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,

  // One or two character tokens.
  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,

  // Literals.
  IDENTIFIER,
  STRING,
  NUMBER,

  // Keywords.
  AND,
  CLASS,
  ELSE,
  FALSE,
  FUN,
  FOR,
  IF,
  NIL,
  OR,
  PRINT,
  RETURN,
  SUPER,
  THIS,
  TRUE,
  VAR,
  WHILE,

  _EOF
};

const std::string &token_to_string(const TokenType type) noexcept;

class Token {
private:
  const TokenType type;
  const std::string lexeme;
  xanadu::Types::OptionalLiteral literal = std::nullopt;
  const int line;

public:
  //
  // Constructors
  //
  Token(const TokenType _type, const std::string &_lexeme,
        const xanadu::Types::OptionalLiteral &_literal, int _line) noexcept;
  Token() noexcept = delete;

  //
  // Getters
  //
  inline TokenType getType() const noexcept { return type; }
  inline std::string getLexeme() const noexcept { return lexeme; }
  inline OptionalLiteral getLiteral() const noexcept { return literal; }
  inline int getLine() const noexcept { return line; }

  // Convert Token class to string
  std::string to_string() const noexcept;

  //
  // Overloaded operators
  //

  // Mainly used for testing
  bool operator==(Token lhs) const noexcept;
};
} // namespace xanadu::Tokens
