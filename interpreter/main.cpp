#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void runFile(std::string file);
void runPrompt();
void run(std::string str);

int main (int argc, char *argv[]) {
  // check command line arguments
  // if more then one is given exit
  if (argc > 1)
  {
    std::cout << "Usage: xanadu [script]" << std::endl;
    return 0;
  } else if (argc == 1) // if a file name is given
    // interpreter given file
    runFile(argv[0]);
  else 
    // open command prompt interpreter
    runPrompt();

  return 0;
}

// interpreter given file
void runFile(std::string file)
{
  std::string text;
  std::ifstream source_file(file);

  // read from file
  while (std::getline (source_file, text)) continue;
  source_file.close();

  // run interpreter on read text
  run(text);
}

// run command prompt interpreter
void runPrompt()
{
  std::string input = "";

  // read command prompt input from user
  while (true) 
  {
    std::cout << "> ";
    std::cin >> input;

    // interprete given command prompt
    run(input);

    std::cout << std::endl;
  }
}

void run(std::string input)
{
  // vector holding tokens
  std::vector<std::string> tokens;

  std::stringstream stream(input);
  std::string intermediate;
  // tokenizing input string based on space ' '
  while (std::getline(stream, intermediate, ' '))
  {
    tokens.push_back(intermediate);
  }

  for (auto token : tokens)
  {
       std::cout << token;
  }
}
