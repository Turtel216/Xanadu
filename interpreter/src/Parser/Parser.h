#pragma once

#include "../Ast/Expr.h"
#include "../Types/Token.h"
#include <memory>
#include <vector>

namespace xanadu {

class Parser {
private:
  const std::vector<xanadu::Tokens::Token> tokens;
  int current;

  std::unique_ptr<Expr> expression();
  std::unique_ptr<Expr> equality();
  bool match(xanadu::Tokens::TokenType types, ...);
  bool check(xanadu::Tokens::TokenType type);
  xanadu::Tokens::Token advance();
  bool isAtEnd();
  xanadu::Tokens::Token peek();
  xanadu::Tokens::Token previous();

public:
  //
  // Constructors
  //
  Parser(std::vector<xanadu::Tokens::Token> _tokens) noexcept;
};
} // namespace xanadu
