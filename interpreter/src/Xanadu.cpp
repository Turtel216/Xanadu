#include "Xanadu.h"
#include "Parser/Parser.h"
#include "Scanner/Scanner.h"
#include "Types/Token.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace xanadu {

const int ERROR_EXIT = 65;

// instanciete bool variable
bool Xanadu::hadErr = false;

// interprete given file
int Xanadu::runFile(const std::string &file) noexcept {
  // Read from file
  const auto source = ([&]() -> std::string {
    try {
      std::ifstream in(file, std::ios::in);
      in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      return std::string{std::istreambuf_iterator<char>{in},
                         std::istreambuf_iterator<char>{}};
    } catch (std::exception &e) {
      std::cout << "Could'nt open file" << "\n";
      return "";
    }
  })();

  if (source.empty())
    return ERROR_EXIT;

  // run interpreter on read text
  run(source);

  // if an error had been encountered, exit the program with error code 65
  if (hadErr)
    return ERROR_EXIT;

  // Exit program with code 0(success)
  return 0;
}

// run command prompt interpreter
void Xanadu::runPrompt() noexcept {
  std::string line = "";

  // read command prompt input from user
  while (std::cout << "> " && std::getline(std::cin, line)) {
    // interprete given command prompt
    run(line);
    hadErr = false;
  }
}

// TODO
//  runs the interpreter
void Xanadu::run(const std::string &input) noexcept {
  // vector holding tokens
  std::vector<Tokens::Token> tokens;

  // Lexer
  Scanner scanner(input);
  // Call lexer
  tokens = scanner.scanTokens();

  for (auto token : tokens)
    std::cout << token.getLexeme() << "\n";

  // Parser
  Parser parser(tokens);
  auto expression = parser.parse();

  // Stop if there was a syntax error
  if (hadErr)
    return;
}

// throw an interpreter error
void Xanadu::error(int line, const std::string &message) noexcept {
  Xanadu::report(line, "", message);
}

// throw an interpreter error
void Xanadu::error(Tokens::Token token, const std::string &message) noexcept {
  if (token.getType() == Tokens::TokenType::_EOF)
    report(token.getLine(), "at end", message);
  else
    report(token.getLine(), "at '" + token.getLexeme() + "'", message);
}

// display an interpreter error
void Xanadu::report(int line, const std::string &where,
                    const std::string &message) noexcept {
  std::cout << "[line " << line << "] Error " << where << ": " << message
            << std::endl;

  hadErr = true;
}
} // namespace xanadu
