#include <string>
#include <vector>
#include "Scanner.h"
#include "../Xanadu.h"

using namespace xanadu::Tokens;

namespace xanadu::Scanner
{
// Getters
std::string Scanner::getSource() const noexcept { return this->source; }
std::vector<xanadu::Tokens::Token> Scanner::getTokens() const noexcept { return this->tokens; }

std::vector<xanadu::Tokens::Token> Scanner::scanTokens() noexcept
{
  while (!isAtEnd())
  {
    start = current;
    scanToken();
  }

  tokens.push_back(Tokens::Token(Tokens::TokenType::_EOF, "", nullptr, line));
  return tokens;
}

void Scanner::scanToken() noexcept 
{
  char _char = advance();
  switch (_char) {
    case '(': addToken(LEFT_PAREN); break;
      case ')': addToken(RIGHT_PAREN); break;
      case '{': addToken(LEFT_BRACE); break;
      case '}': addToken(RIGHT_BRACE); break;
      case ',': addToken(COMMA); break;
      case '.': addToken(DOT); break;
      case '-': addToken(MINUS); break;
      case '+': addToken(PLUS); break;
      case ';': addToken(SEMICOLON); break;
      case '*': addToken(STAR); break;
    default:
      xanadu::Xanadu::error(line, "Unexpected character.");
      break;
  }
}

bool Scanner::isAtEnd() const noexcept { return current >= source.length(); }
char Scanner::advance() noexcept { return source.at(current++); }
void Scanner::addToken(Tokens::TokenType type) noexcept { addToken(type, nullptr); }
void Scanner::addToken(Tokens::TokenType type,
                       Types::OptionalLiteral literal) noexcept  
{
  std::string text = source.substr(start, current);
  tokens.push_back(Tokens::Token(type, text, literal, line));
}
}
