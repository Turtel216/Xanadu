#include <complex>
#include <map>
#include <string>
#include <vector>
#include "../Types/Token.h"

using namespace xanadu::Tokens;

namespace xanadu::Scanner
{
class Scanner
{
private:
  const std::string source;
  std::vector<xanadu::Tokens::Token> tokens;
  int start = 0;
  int current = 0;
  int line = 1;

  static std::map<std::string, Tokens::TokenType> keywords;

  // Helper methods
  void scanToken() noexcept;
  bool isAtEnd() const noexcept;
  char advance() noexcept;
  void addToken(Tokens::TokenType type) noexcept;
  void addToken(Tokens::TokenType type, Types::OptionalLiteral literal) noexcept;
  bool match(char expected) noexcept;
  char peek() const noexcept;
  char peekNext() const noexcept;
  void string() noexcept;
  void number(char) noexcept;
  bool isAlpha(char) const noexcept;
  bool isAlphaNumeric(char) const noexcept;
  void identifier() noexcept;

public:
  Scanner(const std::string _source) noexcept
  : source(_source) 
  {
    keywords.insert_or_assign("and",    AND);
    keywords.insert_or_assign("overtune",  CLASS);
    keywords.insert_or_assign("else",   ELSE);
    keywords.insert_or_assign("false",  FALSE);
    keywords.insert_or_assign("for",    FOR);
    keywords.insert_or_assign("subdivision",    FUN);
    keywords.insert_or_assign("if",     IF);
    keywords.insert_or_assign("nil",    NIL);
    keywords.insert_or_assign("or",     OR);
    keywords.insert_or_assign("print",  PRINT);
    keywords.insert_or_assign("return", RETURN);
    keywords.insert_or_assign("super",  SUPER);
    keywords.insert_or_assign("this",   THIS);
    keywords.insert_or_assign("true",   TRUE);
    keywords.insert_or_assign("yyz",    VAR);
    keywords.insert_or_assign("while",  WHILE);
  }

  std::vector<xanadu::Tokens::Token> scanTokens() noexcept;

  // Getters
  std::string getSource() const noexcept;
  std::vector<xanadu::Tokens::Token> getTokens() const noexcept;

};

}
