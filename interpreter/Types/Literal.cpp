#include "Literal.h"

namespace xanadu::Types {

std::string getLiteralString(const Literal& value)
{
  // Literal = std::variant<std::string, double>;
  switch (value.index()) {
    case 0:  // string
      return std::get<0>(value);
    case 1: {  // double
      std::string result = std::to_string(std::get<1>(value));
      auto pos = result.find(".000000");
      if (pos != std::string::npos)
        result.erase(pos, std::string::npos);
      else
        result.erase(result.find_last_not_of('0') + 1, std::string::npos);
      return result;
    }
    default:
      static_assert(
          std::variant_size_v<Literal> == 2,
          "Looks like you forgot to update the cases in getLiteralString()!");
      return "";
  }
}

OptionalLiteral makeOptionalLiteral(double dVal)
{
  return OptionalLiteral(std::in_place, dVal);
}

OptionalLiteral makeOptionalLiteral(const std::string& lexeme)
{
  return OptionalLiteral(std::in_place, lexeme);
}
}
