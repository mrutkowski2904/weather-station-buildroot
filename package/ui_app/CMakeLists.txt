cmake_minimum_required(VERSION 3.10)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra")

project(ui_app)
add_executable(ui_app main.c graphics.c display.c)

install (TARGETS ui_app DESTINATION bin)