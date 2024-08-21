cmake_minimum_required(VERSION 3.30)
project(Xanadu)

enable_testing()

add_executable ( xaninter src/main.c src/chunk.c src/memory.c src/debug.c src/value.c src/vm.c src/error.c src/compiler.c src/scanner.c src/object.c src/lookup_table.c )

#Tests
# add_executable ( scanner_test test/scanner_test.cpp src/Xanadu.cpp src/Types/Token.cpp src/Types/Literal.cpp src/Scanner/Scanner.cpp src/Parser/Parser.cpp)

# add_test(scanner-test scanner_test)
