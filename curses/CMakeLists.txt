cmake_minimum_required (VERSION 3.12)
project (tn5250 LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250)

if (${CURSES_FOUND})
    add_executable(curses cursesterm.c tn5250.c cursesterm.h)
    add_dependencies(curses 5250)
    target_link_libraries(curses 5250 ${CURSES_LIBRARIES})
    # install as tn5250 on unix, but don't conflict with the graphical tn5250 on windows
    if (NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
        set_target_properties(curses PROPERTIES OUTPUT_NAME tn5250)
    endif()
    install(TARGETS curses)
endif()
