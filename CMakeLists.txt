cmake_minimum_required(VERSION 3.0)
project(press)

set(executable presstest)
set(CMAKE_CXX_STANDARD 11)

set(sources
	press.hpp
	test.cpp
)

add_executable(${executable} ${sources})
