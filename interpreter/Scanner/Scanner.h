#include "../Types/Token.h"
#include <complex>
#include <map>
#include <string>
#include <vector>

using namespace xanadu::Tokens;

namespace xanadu::Scanner {
class Scanner {
private:
  const std::string source;
  std::vector<xanadu::Tokens::Token> tokens;
  int start = 0;
  int current = 0;
  int line = 1;

  typedef std::map<std::string, Tokens::TokenType> keywords_map;
  static keywords_map keywords;

  // Helper methods
  void scanToken() noexcept;
  bool isAtEnd() const noexcept;
  char advance() noexcept;
  void addToken(Tokens::TokenType type) noexcept;
  void addToken(Tokens::TokenType type,
                Types::OptionalLiteral literal) noexcept;
  bool match(char expected) noexcept;
  char peek() const noexcept;
  char peekNext() const noexcept;
  void string() noexcept;
  void number() noexcept;
  bool isAlpha(char) const noexcept;
  bool isAlphaNumeric(char) const noexcept;
  void identifier() noexcept;

public:
  Scanner(const std::string _source) noexcept : source(_source) {}

  std::vector<xanadu::Tokens::Token> scanTokens() noexcept;

  // Getters
  std::string getSource() const noexcept;
  std::vector<xanadu::Tokens::Token> getTokens() const noexcept;
};
} // namespace xanadu::Scanner
