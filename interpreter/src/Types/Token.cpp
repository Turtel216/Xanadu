#include "Token.h"
#include "Literal.h"
#include <map>
#include <string>

namespace xanadu::Tokens {
using OptionalLiteral = xanadu::Types::OptionalLiteral;

// Convert token enum to string
const std::string &token_to_string(const TokenType type) noexcept {
  static const std::map<TokenType, std::string> lookUpTable{
      {TokenType::LEFT_PAREN, "LEFT_PAREN"},
      {TokenType::RIGHT_PAREN, "RIGHT_PAREN"},
      {TokenType::LEFT_BRACE, "LEFT_BRACE"},
      {TokenType::RIGHT_BRACE, "RIGHT_BRACE"},
      {TokenType::COMMA, "COMMA"},
      {TokenType::DOT, "DOT"},
      {TokenType::SEMICOLON, "SEMICOLON"},
      {TokenType::SLASH, "SLASH"},
      {TokenType::STAR, "STAR"},
      {TokenType::BANG, "BANG"},
      {TokenType::BANG_EQUAL, "BANG_EQUAL"},
      {TokenType::EQUAL, "EQUAL"},
      {TokenType::EQUAL_EQUAL, "EQUAL_EQUAL"},
      {TokenType::GREATER, "GREATER"},
      {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
      {TokenType::LESS, "LESS"},
      {TokenType::LESS_EQUAL, "LESS_EQUAL"},
      {TokenType::MINUS, "MINUS"},
      {TokenType::PLUS, "PLUS"},
      {TokenType::IDENTIFIER, "IDENTIFIER"},
      {TokenType::STRING, "STRING"},
      {TokenType::NUMBER, "NUMBER"},
      {TokenType::AND, "AND"},
      {TokenType::CLASS, "CLASS"},
      {TokenType::ELSE, "ELSE"},
      {TokenType::FUN, "FUN"},
      {TokenType::FOR, "FOR"},
      {TokenType::IF, "IF"},
      {TokenType::NIL, "NIL"},
      {TokenType::OR, "OR"},
      {TokenType::PRINT, "PRINT"},
      {TokenType::RETURN, "RETURN"},
      {TokenType::SUPER, "SUPER"},
      {TokenType::THIS, "THIS"},
      {TokenType::VAR, "VAR"},
      {TokenType::WHILE, "WHILE"},
      {TokenType::_EOF, "EOF"}};

  return lookUpTable.find(type)->second;
}

// Token constructor
Token::Token(const TokenType _type, std::string _lexeme,
             xanadu::Types::OptionalLiteral _literal, int _line) noexcept
    : type(_type), lexeme(_lexeme), literal(_literal), line(_line) {}

// Getters
TokenType Token::getType() const noexcept { return type; }
std::string Token::getLexeme() const noexcept { return lexeme; }
OptionalLiteral Token::getLiteral() const noexcept { return literal; }
int Token::getLine() const noexcept { return line; }

// Convert Token class to string
std::string Token::to_string() const noexcept {
  return token_to_string(type) + " " + lexeme + " " +
         xanadu::Types::getLiteralString(literal.value());
}
} // namespace xanadu::Tokens
