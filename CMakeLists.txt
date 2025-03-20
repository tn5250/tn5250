cmake_minimum_required (VERSION 3.12)
project (tn5250 LANGUAGES C VERSION 0.9.8)

include(CheckIncludeFile)
include(GNUInstallDirs)

find_package(OpenSSL)
find_package(Curses)

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

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    set( PLATFORM_LIBRARIES Ws2_32 Winmm)
endif()

add_definitions ( -D_VERSION=${CMAKE_PROJECT_VERSION} -D_PACKAGE=${CMAKE_PROJECT_NAME} -DSYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}")

add_subdirectory(curses)
add_subdirectory(doc)
add_subdirectory(lib5250)
add_subdirectory(lp5250d)
add_subdirectory(termcaps)
add_subdirectory(win32)
