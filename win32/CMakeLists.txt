cmake_minimum_required(VERSION 3.12)
project(tn5250-win32 LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250 )

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    add_executable(tn5250 WIN32 tn5250-win.c tn5250-res.rc winterm.c)
    target_link_libraries(tn5250 5250 ${PLATFORM_LIBRARIES})
endif()
