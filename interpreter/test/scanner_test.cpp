#include "../src/Scanner/Scanner.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace xanadu;

int main(void) {
  std::string input = "2 == 2\n2 != 3";

  auto tmp_literal = Types::makeOptionalLiteral("");
  std::vector<Token> expected_tokens = {
      Token(NUMBER, "2", tmp_literal, 1),
      Token(EQUAL_EQUAL, "==", tmp_literal, 1),
      Token(NUMBER, "2", tmp_literal, 1),
      Token(NUMBER, "2", tmp_literal, 2),
      Token(BANG_EQUAL, "!=", tmp_literal, 2),
      Token(NUMBER, "3", tmp_literal, 2),
      Token(_EOF, "", tmp_literal, 2)};

  auto scanner = Scanner(input);
  auto output = scanner.scanTokens();

  assert(expected_tokens.size() == output.size());

  for (auto token : output) {
    std::cout << "Token: " << token_to_string(token.getType()) << "\n";
  }

  int counter = 0;
  for (auto token : output) {
    assert(token == expected_tokens.at(counter));
    counter++;
  }
}
