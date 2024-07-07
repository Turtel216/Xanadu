#include "Scanner.h"
#include "../Xanadu.h"
#include <cctype>
#include <string>
#include <vector>

using namespace xanadu::Tokens;

namespace xanadu {

// initialize keywords map
Scanner::keywords_map Scanner::keywords = {
    {"and", AND},          {"overtune", CLASS},    {"else", ELSE},
    {"false", FALSE},      {"circumstances", FOR}, {"subdivision", FUN},
    {"free_will", IF},     {"nil", NIL},           {"or", OR},
    {"blabla", PRINT},     {"return", RETURN},     {"super", SUPER},
    {"this", THIS},        {"true", TRUE},         {"yyz", VAR},
    {"here_again", WHILE},
};

// Getters
std::string Scanner::getSource() const noexcept { return this->source; }
std::vector<xanadu::Tokens::Token> Scanner::getTokens() const noexcept {
  return this->tokens;
}

// Parse string for tokens
std::vector<xanadu::Tokens::Token> Scanner::scanTokens() noexcept {
  while (!isAtEnd()) {
    start = current;
    scanToken();
  }

  tokens.push_back(Tokens::Token(Tokens::TokenType::_EOF, "", nullptr, line));
  return tokens;
}

// Map chararcters to Token enums
void Scanner::scanToken() noexcept {
  char _char = advance();
  switch (_char) {
  case '(':
    addToken(LEFT_PAREN);
    break;
  case ')':
    addToken(RIGHT_PAREN);
    break;
  case '{':
    addToken(LEFT_BRACE);
    break;
  case '}':
    addToken(RIGHT_BRACE);
    break;
  case ',':
    addToken(COMMA);
    break;
  case '.':
    addToken(DOT);
    break;
  case '-':
    addToken(MINUS);
    break;
  case '+':
    addToken(PLUS);
    break;
  case ';':
    addToken(SEMICOLON);
    break;
  case '*':
    addToken(STAR);
    break;
  case '!':
    addToken(match('=') ? BANG_EQUAL : BANG);
    break;
  case '=':
    addToken(match('=') ? EQUAL_EQUAL : EQUAL);
    break;
  case '<':
    addToken(match('=') ? LESS_EQUAL : LESS);
    break;
  case '>':
    addToken(match('=') ? GREATER_EQUAL : GREATER);
    break;
  case '/':
    if (match('/')) {
      // A comment goes until the end of the line.
      while (peek() != '\n' && !isAtEnd())
        advance();
    } else {
      addToken(SLASH);
    }
    break;
  // Ignore whitespace.
  case ' ':
    break;
  case '\r':
    break;
  case '\t':
    break;
  // Advance to next line
  case '\n':
    line++;
    break;
  case '"':
    string();
    break;

  default:
    if (isdigit(_char)) {
      number();
    } else if (isAlpha(_char)) {
      identifier();
    } else {
      xanadu::Xanadu::error(line, "Unexpected character.");
    }
    break;
  }
}

// Check if the end of the string is reached
bool Scanner::isAtEnd() const noexcept { return current >= source.length(); }
// Continue to the next character
char Scanner::advance() noexcept { return source.at(current++); }
// Add token enum to map
void Scanner::addToken(Tokens::TokenType type) noexcept {
  addToken(type, nullptr);
}
// Add token enum to map
void Scanner::addToken(Tokens::TokenType type,
                       Types::OptionalLiteral literal) noexcept {
  std::string text = source.substr(start, current);
  tokens.push_back(Tokens::Token(type, text, literal, line));
}

// Consume current character if next character is the expected character
bool Scanner::match(char expected) noexcept {
  if (isAtEnd())
    return false;
  if (source.at(current) != expected)
    return false;

  current++;
  return true;
}

// Check for number enum and add it to map
void Scanner::number() noexcept {
  while (isdigit(peek())) {
    // check if factorial part
    if (peek() == '.' && isdigit(peekNext())) {
      advance();

      while (isdigit(peek()))
        advance();
    }
  }

  // Convert string to double and add to tokens
  addToken(NUMBER, std::stod(source.substr(start, current)));
}

// look ahead and return next chararacter
char Scanner::peek() const noexcept {
  if (isAtEnd())
    return '\0';
  return source.at(current);
}

// look 2 chararacters ahead and return it
char Scanner::peekNext() const noexcept {
  if (current + 1 >= source.length())
    return '\n';
  return source.at(current + 1);
}

// Add value of string literal
void Scanner::string() noexcept {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n')
      ++line;

    advance();
  }

  if (isAtEnd()) {
    xanadu::Xanadu::error(line, "Unterminated string");
    return;
  }

  advance();

  // Trim the surrounding quotes
  auto value = source.substr(start + 1, current - 1);
  addToken(STRING, value);
}

// Check if token is an identifier or keyword
// and add to map accordingly
void Scanner::identifier() noexcept {
  while (isAlphaNumeric(peek()))
    advance();

  auto text = source.substr(start, current);

  // get the iterator pointing to the string value,
  // returns end iterator if not found
  auto type_iterator = keywords.find(text);

  // if the identifier is not a keyword,
  // add identifier
  if (type_iterator == keywords.end())
    addToken(IDENTIFIER);
  else
    // else add the keyword
    addToken(type_iterator->second);
}

// Check if character is alpharithmic
bool Scanner::isAlpha(char _char) const noexcept {
  return (_char >= 'a' && _char <= 'z') || (_char >= 'A' && _char <= 'Z') ||
         _char == '_';
}

// Check if character is alphanumeric
bool Scanner::isAlphaNumeric(char _char) const noexcept {
  return isAlpha(_char) || isdigit(_char);
}
} // namespace xanadu
