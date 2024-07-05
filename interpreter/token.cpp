#include "token_type.h"
#include <string>

namespace token 
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

// Convert Token class to string
std::string Token::to_string() const noexcept
{
  return token_toString(type) + " " + lexeme + " " + literal;
}
}
