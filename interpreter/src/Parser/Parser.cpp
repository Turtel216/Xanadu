#include "Parser.h"
#include "../Ast/Expr.h"
#include "../Types/Token.h"
#include <memory>

namespace xanadu {
//
// Parser definition
//

//
// Member functions
//
std::unique_ptr<Expr> Parser::expression() { return equality(); }

std::unique_ptr<Expr> Parser::equality() {}

// TODO
bool Parser::match(xanadu::Tokens::TokenType types, ...) {
  for (auto type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }

  return false;
}
bool Parser::check(xanadu::Tokens::TokenType type) {
  if (isAtEnd())
    return false;
  return peek().getType() == type;
}

xanadu::Tokens::Token Parser::advance() {
  if (!isAtEnd())
    current++;
  return previous();
}

bool Parser::isAtEnd() {
  return peek().getType() == EOF; // TODO
}
xanadu::Tokens::Token Parser::peek() { return tokens.at(current); }

xanadu::Tokens::Token Parser::previous() { return tokens.at(current - 1); }

//
// Constructors
//
Parser::Parser(std::vector<xanadu::Tokens::Token> _tokens) noexcept
    : tokens(_tokens), current(0) {}
} // namespace xanadu
