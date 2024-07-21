#include "Xanadu.h"
#include "Types/Token.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace xanadu {

// instanciete bool variable
bool Xanadu::hadErr = false;

// interprete given file
int Xanadu::runFile(const std::string &file) noexcept {
  std::string text;
  std::ifstream source_file(file);

  // read from file
  while (std::getline(source_file, text))
    continue;
  source_file.close();

  // run interpreter on read text
  run(text);

  // an error had been encountered, exit program with error code 65
  if (hadErr)
    return 65;

  return 0;
}

// run command prompt interpreter
void Xanadu::runPrompt() noexcept {
  std::string input = "";

  // read command prompt input from user
  while (true) {
    std::cout << "> ";
    std::cin >> input;

    // interprete given command prompt
    run(input);
    hadErr = false;

    std::cout << std::endl;
  }
}

// TODO
//  runs the interpreter
void Xanadu::run(const std::string &input) noexcept {
  // vector holding tokens
  std::vector<std::string> tokens;

  std::stringstream stream(input);
  std::string intermediate;
  // tokenizing input string based on space ' '
  while (std::getline(stream, intermediate, ' '))
    tokens.push_back(intermediate);

  for (auto token : tokens)
    std::cout << token;
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
  std::cout << "[line " << line << "] Error" << where << ": " << message
            << std::endl;

  hadErr = true;
}
} // namespace xanadu
