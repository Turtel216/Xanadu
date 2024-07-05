#include <string>
namespace token 
{
enum TokenType
{
   // Single-character tokens.
  LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
  COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

  // One or two character tokens.
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,

  // Literals.
  IDENTIFIER, STRING, NUMBER,

  // Keywords.
  AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
  PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

  _EOF
};

class Token 
{
public:
  const TokenType type;
  const std::string lexeme;
  const std::string literal;
  const int line;

  Token(const TokenType _type, std::string _lexeme,
        std::string _literal, int _line) noexcept
        : type(_type), lexeme(_lexeme),
        literal(_literal), line(_line) {}

  // Convert Token class to string
  std::string to_string() const noexcept;
};
}
