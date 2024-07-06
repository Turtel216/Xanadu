#include "Token.h"
#include "Literal.h"
#include <string>

namespace xanadu::Tokens 
{
// Convert token enum to string
std::string token_toString(TokenType type) noexcept 
{
  switch (type) {
    case LEFT_PAREN: return "(";
    default: return "";
      break;
  }
}

// Getters
TokenType Token::getType() const noexcept { return type; }
std::string Token::getLexeme() const noexcept { return lexeme; }
xanadu::Types::OptionalLiteral Token::getLiteral() const noexcept { return literal; }
int Token::getLine() const noexcept { return line; }

// Convert Token class to string
std::string Token::to_string() const noexcept
{
  return token_toString(type) + " " + lexeme + " " + xanadu::Types::getLiteralString(literal.value());
}

}
