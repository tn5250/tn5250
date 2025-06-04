cmake_minimum_required(VERSION 3.12)
project(termcaps)

# Add platform-specific subdirectories
add_subdirectory(freebsd)
add_subdirectory(linux)
add_subdirectory(sun)

if (APPLE)
    add_subdirectory(macos)
endif()
