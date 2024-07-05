#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Xenadu
{
public:
  static bool hadErr;
  // interpreter given file
  static int runFile(std::string file) noexcept;
  // run command prompt interpreter
  static void runPrompt() noexcept;
  // runs the interpreter
  static void run(std::string str) noexcept;
  // throw an interpreter error
  static void error(int line, std::string message) noexcept;
  // display an interpreter error
  static void report(int line, std::string where, std::string message) noexcept;
};

int main (int argc, char *argv[]) {
  Xenadu::hadErr = false;
  // check command line arguments
  // if more then one is given exit
  if (argc > 1)
  {
    std::cout << "Usage: xanadu [script]" << std::endl;
    return 0;
  } else if (argc == 1) // if a file name is given
    // interpreter given file
    return Xenadu::runFile(argv[0]);
  else 
    // open command prompt interpreter
    Xenadu::runPrompt();

  return 0;
}

// interpreter given file
int Xenadu::runFile(std::string file) noexcept
{
  std::string text;
  std::ifstream source_file(file);

  // read from file
  while (std::getline (source_file, text)) continue;
  source_file.close();

  // run interpreter on read text
  run(text);

  // an error had been encountered, exit program with error code 65
  if (hadErr) return 65;

  return 0;
}

// run command prompt interpreter
void Xenadu::runPrompt() noexcept
{
  std::string input = "";

  // read command prompt input from user
  while (true) 
  {
    std::cout << "> ";
    std::cin >> input;

    // interprete given command prompt
    run(input);
    hadErr = false;

    std::cout << std::endl;
  }
}

// runs the interpreter
void Xenadu::run(std::string input) noexcept
{
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
void Xenadu::error(int line, std::string message) noexcept
{
  Xenadu::report(line, "", message);
}

// display an interpreter error
void Xenadu::report(int line, std::string where, std::string message) noexcept
{
  std::cout << "[line " << line << "] Error"
    << where << ": " << message << std::endl;

  hadErr = true;
}
