cmake_minimum_required(VERSION 3.12)
project(tn5250 LANGUAGES C VERSION 0.19.0)

include(CheckIncludeFile)
include(GNUInstallDirs)

find_package(OpenSSL)

if (NOT WIN32)
    # For now, assume Windows users don't want curses build
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses REQUIRED)
endif()

option(CURSES_OLD_KEYS "Use curses built-in key handling" YES)

check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("locale.h" HAVE_LOCALE_H)
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
endif()

# Add macOS-specific configurations
if (APPLE)
    # macOS-specific settings can be added here
    message(STATUS "Building for macOS")
endif()
