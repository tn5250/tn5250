cmake_minimum_required (VERSION 3.12)
project (tn5250 LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250)

if (${CURSES_FOUND})
    add_executable(curses cursesterm.c tn5250.c cursesterm.h)
    add_dependencies(curses 5250)
    target_link_libraries(curses 5250 ${CURSES_LIBRARIES})
endif()
