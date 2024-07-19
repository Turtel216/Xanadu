#include "Parser.h"
#include "../Ast/Expr.h"
#include "../Types/Token.h"
#include "../Xanadu.h"
#include <list>
#include <memory>

using namespace xanadu::Tokens;

namespace xanadu {
//
// Parser definition
//

//
// Member functions
//
std::unique_ptr<Expr> Parser::parse() {
  try {
    return expression();
  } catch (ParseErr err) {
    return nullptr;
  }
}

std::unique_ptr<Expr> Parser::expression() noexcept { return equality(); }

std::unique_ptr<Expr> Parser::equality() noexcept {
  std::unique_ptr<Expr> expr = comparision();

  while (match(std::list<Tokens::TokenType>{TokenType::BANG_EQUAL,
                                            TokenType::EQUAL_EQUAL})) {
    auto _operator = previous();
    std::unique_ptr<Expr> right = comparision();
    expr.reset(new BinaryExpr(expr.release(), _operator, right.release()));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::comparision() noexcept {
  auto expr = term();

  while (
      match(std::list<TokenType>{TokenType::GREATER, TokenType::GREATER_EQUAL,
                                 TokenType::LESS, TokenType::LESS_EQUAL})) {
    Token _operator = previous();
    auto right = term();
    expr.reset(new BinaryExpr(expr.release(), _operator, right.release()));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::term() noexcept {
  auto expr = factor();

  while (match(std::list<TokenType>{TokenType::MINUS, TokenType::PLUS})) {
    Token _operator = previous();
    auto right = factor();
    expr.reset(new BinaryExpr(expr.release(), _operator, right.release()));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::factor() noexcept {
  auto expr = unary();

  while (match(std::list<TokenType>{TokenType::SLASH, TokenType::STAR})) {
    Token _operator = previous();
    auto right = unary();
    expr.reset(new BinaryExpr(expr.release(), _operator, right.release()));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::unary() noexcept {
  if (match(std::list<TokenType>{TokenType::BANG, TokenType::MINUS})) {
    Token _operator = previous();
    auto right = unary();
    return std::unique_ptr<Expr>(new UnaryExpr(_operator, right.release()));
  }

  return primary();
}

std::unique_ptr<Expr> Parser::primary() {
  if (match(FALSE))
    return std::unique_ptr<Expr>(new LiteralExpr(false));
  if (match(TRUE))
    return std::unique_ptr<Expr>(new LiteralExpr(true));
  if (match(NIL))
    return std::unique_ptr<Expr>(new LiteralExpr(nullptr));

  if (match(std::list<TokenType>{TokenType::NUMBER, TokenType::STRING}))
    return std::unique_ptr<Expr>(new LiteralExpr(previous().getLiteral()));

  if (match(LEFT_PAREN)) {
    auto expr = expression();
    consume(RIGHT_PAREN, "Expect ')' after expression.");
    return std::unique_ptr<Expr>(new GroupingExpr(expr.get()));
  }

  throw error(peek(), "Expect expression");
}

xanadu::Tokens::Token Parser::consume(Tokens::TokenType type,
                                      const std::string &message) {
  if (check(type))
    return advance();

  throw error(peek(), message);
}

// TODO
void Parser::synchronize() noexcept {
  advance();

  while (!isAtEnd()) {
    if (previous().getType() == Tokens::TokenType::SEMICOLON)
      return;

    switch (peek().getType()) {
    case Tokens::TokenType::CLASS:
    case Tokens::TokenType::FUN:
    case Tokens::TokenType::VAR:
    case Tokens::TokenType::FOR:
    case Tokens::TokenType::IF:
    case Tokens::TokenType::WHILE:
    case Tokens::TokenType::PRINT:
    case Tokens::TokenType::RETURN:
      return;
    }

    advance();
  }
}

ParseErr Parser::error(Tokens::Token token,
                       const std::string &message) noexcept {
  Xanadu::error(token, message);
  return ParseErr();
}

bool Parser::match(std::list<Tokens::TokenType> types) {
  for (auto type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }

  return false;
}

bool Parser::match(TokenType type) {

  if (check(type)) {
    advance();
    return true;
  }

  return false;
}

bool Parser::check(Tokens::TokenType type) {
  if (isAtEnd())
    return false;
  return peek().getType() == type;
}

Tokens::Token Parser::advance() {
  if (!isAtEnd())
    current++;
  return previous();
}

bool Parser::isAtEnd() { return peek().getType() == Tokens::_EOF; }

Tokens::Token Parser::peek() { return tokens.at(current); }

Tokens::Token Parser::previous() { return tokens.at(current - 1); }

//
// Constructors
//
Parser::Parser(std::vector<Tokens::Token> _tokens) noexcept
    : tokens(_tokens), current(0) {}
} // namespace xanadu
