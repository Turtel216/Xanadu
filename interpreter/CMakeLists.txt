cmake_minimum_required(VERSION 3.30)
project(Xanadu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable ( xaninter src/main.cpp src/Xanadu.cpp src/Types/Token.cpp src/Types/Literal.cpp src/Scanner/Scanner.cpp src/Parser/Parser.cpp)

#Tests
add_executable ( scanner_test test/scanner_test.cpp src/Xanadu.cpp src/Types/Token.cpp src/Types/Literal.cpp src/Scanner/Scanner.cpp src/Parser/Parser.cpp)

add_test(scanner-test scanner_test)
