cmake_minimum_required(VERSION 3.30)
project(Xanadu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable ( xanadu_interpreter src/main.cpp src/Xanadu.cpp src/Types/Token.cpp src/Types/Literal.cpp src/Scanner/Scanner.cpp)
