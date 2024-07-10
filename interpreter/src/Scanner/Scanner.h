#pragma once

#include "../Types/Token.h"
#include <complex>
#include <map>
#include <string>
#include <vector>

using namespace xanadu::Tokens;

namespace xanadu {
class Scanner {
private:
  const std::string source;
  std::vector<xanadu::Tokens::Token> tokens;
  int start = 0;
  int current = 0;
  int line = 1;

  // Type to initialize map
  typedef std::map<std::string, Tokens::TokenType> keywords_map;
  static keywords_map keywords;

  // Helper methods
  // Map chararcters to Token enums
  void scanToken() noexcept;
  // Check if the end of the string is reached
  inline bool isAtEnd() const noexcept;
  // Continue to the next character
  inline char advance() noexcept;
  // Add token enum to map
  inline void addToken(Tokens::TokenType type) noexcept;
  // Add token enum to map
  void addToken(Tokens::TokenType type,
                Types::OptionalLiteral literal) noexcept;
  // Consume current character if next character is the expected character
  bool match(char expected) noexcept;
  // look ahead and return next chararacter
  char peek() const noexcept;
  // look 2 chararacters ahead and return it
  char peekNext() const noexcept;
  // Add value of string literal
  void string() noexcept;
  // Check for number enum and add it to map
  void number() noexcept;
  // Check if character is alpharithmic
  inline bool isAlpha(char) const noexcept;
  // Check if character is alphanumeric
  inline bool isAlphaNumeric(char) const noexcept;
  // Check if token is an identifier or keyword
  // and add to map accordingly
  void identifier() noexcept;

public:
  Scanner(const std::string &_source) noexcept;
  Scanner() noexcept = delete;

  // Parse string for tokens
  std::vector<xanadu::Tokens::Token> scanTokens() noexcept;

  // Getters
  inline std::string getSource() const noexcept;
  inline std::vector<xanadu::Tokens::Token> getTokens() const noexcept;
};
} // namespace xanadu
