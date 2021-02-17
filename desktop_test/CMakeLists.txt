cmake_minimum_required(VERSION 3.9)
project(desktop_test)

set(CMAKE_CXX_STANDARD 11)

add_executable(desktop_test main.cpp lib/pt-1.4 declarations.h lib/Arduino.h)