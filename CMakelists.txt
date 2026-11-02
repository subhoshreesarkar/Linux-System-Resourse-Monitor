cmake_minimum_required(VERSION 3.10)
project(SystemMonitor C)

set(CMAKE_C_STANDARD 11)

set(CURSES_LIBRARY /usr/lib/x86_64-linux-gnu/libncurses.so)
set(CURSES_INCLUDE_DIR /usr/include)

include_directories(${CURSES_INCLUDE_DIR})

add_executable(SystemMonitor main.c)
target_link_libraries(SystemMonitor ${CURSES_LIBRARY})
