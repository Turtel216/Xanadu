#pragma once

#include <string>

namespace xanadu {
class Xanadu {
public:
  static bool hadErr;
  // interprete given file
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
} // namespace xanadu
