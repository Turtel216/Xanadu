#include "Xanadu.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // check command line arguments
  // if more then one is given exit
  if (argc > 1) {
    std::cout << "Usage: xanadu [script]" << std::endl;
    return 0;
  } else if (argc == 1) // if a file name is given
    // interprete given file
    return xanadu::Xanadu::runFile(argv[0]);
  else
    // open command prompt interpreter
    xanadu::Xanadu::runPrompt();

  return 0;
}
