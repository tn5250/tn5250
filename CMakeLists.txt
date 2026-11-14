cmake_minimum_required(VERSION 3.12)
project(tn5250 LANGUAGES C VERSION 0.19.0)

include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CMakePushCheckState)
include(GNUInstallDirs)

find_package(OpenSSL)

if (NOT WIN32)
    # For now, assume Windows users don't want curses build
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses REQUIRED)

    # Displaying UTF-8 takes the wide character functions of curses.  They
    # often are in a library of their own (ncursesw), which is then the one
    # to use, or else the curses library we just found may have them.
    find_library(CURSES_NCURSESW_LIBRARY NAMES ncursesw)
    if (CURSES_NCURSESW_LIBRARY)
        set(CURSES_WIDE_LIBRARIES ${CURSES_NCURSESW_LIBRARY})
        # As FindCurses does: some of ncurses may be in a terminfo library.
        check_library_exists("${CURSES_NCURSESW_LIBRARY}" nodelay ""
                             CURSES_NCURSESW_HAS_NODELAY)
        if (NOT CURSES_NCURSESW_HAS_NODELAY)
            find_library(CURSES_TINFOW_LIBRARY NAMES tinfow tinfo)
            if (CURSES_TINFOW_LIBRARY)
                list(APPEND CURSES_WIDE_LIBRARIES ${CURSES_TINFOW_LIBRARY})
            endif()
        endif()
    else()
        set(CURSES_WIDE_LIBRARIES ${CURSES_LIBRARIES})
    endif()

    # The header file which declares them is the one of ncursesw if it has
    # its own, or else the one cursesterm.h includes anyway.
    if (CURSES_HAVE_NCURSES_NCURSES_H)
        set(CURSES_PLAIN_HEADER ncurses/ncurses.h)
    elseif (CURSES_HAVE_NCURSES_H)
        set(CURSES_PLAIN_HEADER ncurses.h)
    else()
        set(CURSES_PLAIN_HEADER curses.h)
    endif()
    foreach (header ncursesw/ncurses.h ncursesw/curses.h ${CURSES_PLAIN_HEADER})
        string(MAKE_C_IDENTIFIER "CURSES_WIDECHAR_IN_${header}" curses_result)
        if (header MATCHES "^ncursesw/")
            set(curses_term_h "#include <ncursesw/term.h>")
        else()
            set(curses_term_h "")
        endif()
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES ${CURSES_INCLUDE_DIRS})
        set(CMAKE_REQUIRED_LIBRARIES ${CURSES_WIDE_LIBRARIES})
        check_c_source_compiles("
            #define _XOPEN_SOURCE_EXTENDED 1
            #define NCURSES_WIDECHAR 1
            #include <${header}>
            ${curses_term_h}
            #include <wchar.h>
            int main(void) {
                cchar_t cell;
                wchar_t text[2] = { L'x', 0 };
                setcchar(&cell, text, A_NORMAL, 0, 0);
                return add_wch(&cell);
            }" ${curses_result})
        cmake_pop_check_state()
        if (${curses_result})
            set(HAVE_CURSES_WIDECHAR TRUE)
            set(CURSES_LIBRARIES ${CURSES_WIDE_LIBRARIES})
            if (header STREQUAL "ncursesw/ncurses.h")
                set(HAVE_NCURSESW_NCURSES_H TRUE)
            elseif (header STREQUAL "ncursesw/curses.h")
                set(HAVE_NCURSESW_CURSES_H TRUE)
            endif()
            break()
        endif()
    endforeach()
    if (NOT HAVE_CURSES_WIDECHAR)
        message(STATUS "No wide character curses library: UTF-8 support is disabled")
    endif()
endif()

option(CURSES_OLD_KEYS "Use curses built-in key handling" YES)

check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("pwd.h" HAVE_PWD_H)
check_include_file("syslog.h" HAVE_SYSLOG_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("termcap.h" HAVE_TERMCAP_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

configure_file(config-cmake.h.in config.h @ONLY)

add_subdirectory(doc)
add_subdirectory(lib5250)
add_subdirectory(termcaps)

if (WIN32)
    add_subdirectory(win32)
else()
    add_subdirectory(curses)
    add_subdirectory(lp5250d)

    # Run the tests with ctest, once all of this is built.
    enable_testing()
    add_subdirectory(tests)
endif()
