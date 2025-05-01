cmake_minimum_required(VERSION 3.12)
project(tn5250-win32 LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250 )

add_executable(tn5250 WIN32 tn5250-win.c tn5250-res.rc winterm.c)
target_link_libraries(tn5250 5250)

# We have a special version of lp5250d for Windows
add_executable(lp5250d WIN32 lp5250d-win.c)
target_link_libraries(lp5250d 5250)

add_executable(dftmap WIN32 dftmap.c)
target_link_libraries(dftmap 5250)
