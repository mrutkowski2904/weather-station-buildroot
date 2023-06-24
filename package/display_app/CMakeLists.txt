cmake_minimum_required(VERSION 3.10)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra")

project(display_app)
add_executable(display_app main.c graphics.c)

install (TARGETS display_app DESTINATION bin)