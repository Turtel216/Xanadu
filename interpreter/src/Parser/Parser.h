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
  const std::vector<Tokens::Token> tokens;
  int current;

  std::unique_ptr<Expr> expression() noexcept;
  std::unique_ptr<Expr> equality() noexcept;
  std::unique_ptr<Expr> comparision() noexcept;
  std::unique_ptr<Expr> term() noexcept;
  std::unique_ptr<Expr> factor() noexcept;
  std::unique_ptr<Expr> unary() noexcept;
  std::unique_ptr<Expr> primary() noexcept;
  Tokens::Token consume(Tokens::TokenType type, const std::string &message);
  ParseErr error(Tokens::Token token, const std::string &message) noexcept;
  bool match(std::list<Tokens::TokenType> types);
  bool match(Tokens::TokenType type);
  bool check(Tokens::TokenType type);
  Tokens::Token advance();
  bool isAtEnd();
  Tokens::Token peek();
  Tokens::Token previous();

public:
  //
  // Constructors
  //
  Parser(std::vector<Tokens::Token> _tokens) noexcept;
};
} // namespace xanadu
