cmake_minimum_required(VERSION 2.8)

project(httpServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(final main.cpp)

target_compile_options(final
	PRIVATE -Wall -Wextra -pedantic -Werror)
