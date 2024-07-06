#include <string>
#include <vector>
#include "../Types/Token.h"
namespace xanadu::Scanner
{
class Scanner
{
private:
  const std::string source;
  const std::vector<xanadu::Tokens::Token> tokens;
  int start = 0;
  int current = 0;
  int line = 1;

  void scanToken() noexcept;

public:
  Scanner(const std::string _source) noexcept
  : source(_source) {}

  std::vector<xanadu::Tokens::Token> scanTokens() noexcept;

};

}
