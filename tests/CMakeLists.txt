cmake_minimum_required(VERSION 3.12)
project(tn5250-tests LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250)

# The character maps of lib5250, and what goes with them.
add_executable(charmaptest charmaptest.c)
target_link_libraries(charmaptest 5250)
add_test(NAME charmap COMMAND charmaptest)

# The emulator from end to end, which takes Python and a pseudo terminal.
find_package(Python3 COMPONENTS Interpreter)
if (Python3_FOUND)
    if (HAVE_CURSES_WIDECHAR)
        set(TEST_UTF8 yes)
    else()
        set(TEST_UTF8 no)
    endif()
    if (CURSES_OLD_KEYS)
        set(TEST_OWN_KEYS no)
    else()
        set(TEST_OWN_KEYS yes)
    endif()
    add_test(NAME terminal
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_CURRENT_SOURCE_DIR}/test_terminal.py)
    set(TEST_ENVIRONMENT
        "TN5250=$<TARGET_FILE:tn5250>"
        "TN5250_CHARMAPTEST=$<TARGET_FILE:charmaptest>"
        "TN5250_UTF8=${TEST_UTF8}"
        "TN5250_OWN_KEYS=${TEST_OWN_KEYS}")
    set_tests_properties(terminal PROPERTIES
        ENVIRONMENT "${TEST_ENVIRONMENT}"
        SKIP_RETURN_CODE 77)
endif()
