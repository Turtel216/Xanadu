#pragma once

#include "../Ast/Expr.h"
#include "../Types/Token.h"
#include <list>
#include <memory>
#include <stdexcept>
#include <vector>

namespace xanadu {

class ParseErr : public std::runtime_error {};

class Parser {
private:
  const std::vector<xanadu::Tokens::Token> tokens;
  int current;

  std::unique_ptr<Expr> expression() noexcept;
  std::unique_ptr<Expr> equality() noexcept;
  std::unique_ptr<Expr> comparision() noexcept;
  std::unique_ptr<Expr> term() noexcept;
  std::unique_ptr<Expr> factor() noexcept;
  std::unique_ptr<Expr> unary() noexcept;
  std::unique_ptr<Expr> primary() noexcept;
  xanadu::Tokens::Token consume(xanadu::Tokens::TokenType type,
                                const std::string &message);
  ParseErr error(Tokens::Token token, const std::string &message) noexcept;
  bool match(std::list<xanadu::Tokens::TokenType> types);
  bool match(xanadu::Tokens::TokenType type);
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
