cmake_minimum_required(VERSION 3.10)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra -lgpiod")

project(ui_app)

add_executable(ui_app main.c graphics.c display.c sensors.c led.c)
install (TARGETS ui_app DESTINATION bin)