cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++1z")

project(web_server)
add_executable(web_server main.cpp)
install (TARGETS web_server DESTINATION bin)