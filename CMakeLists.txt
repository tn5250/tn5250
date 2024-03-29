cmake_minimum_required (VERSION 3.12)
project (tn5250 LANGUAGES C VERSION 0.9.8)

include(CheckIncludeFile)

find_package(OpenSSL)
find_package(Curses)

check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("locale.h" HAVE_LOCALE_H)
check_include_file("pwd.h" HAVE_PWD_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdio.h" HAVE_STDIO_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("syslog.h" HAVE_SYSLOG_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

configure_file(config-cmake.h.in config.h @ONLY)

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    set( PLATFORM_LIBRARIES Ws2_32 Winmm)
endif()

add_definitions ( -D_VERSION=${CMAKE_PROJECT_VERSION} -D_PACKAGE=${CMAKE_PROJECT_NAME} -DSYSCONFDIR=${CMAKE_INSTALL_FULL_SYSCONFDIR})

add_subdirectory(curses)
add_subdirectory(doc)
add_subdirectory(lib5250)
add_subdirectory(lp5250d)
add_subdirectory(termcaps)
add_subdirectory(win32)
