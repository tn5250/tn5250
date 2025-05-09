cmake_minimum_required(VERSION 3.12)
project(tn5250-curses LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250)
add_executable(tn5250 cursesterm.c tn5250.c cursesterm.h)
target_link_libraries(tn5250 5250 ${CURSES_LIBRARIES})

if (NOT CURSES_OLD_KEYS)
    target_compile_definitions(tn5250 PUBLIC -D USE_OWN_KEY_PARSING=1)
endif()

install(TARGETS tn5250)
