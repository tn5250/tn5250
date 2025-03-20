cmake_minimum_required (VERSION 3.12)
project (lp5250d LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR} ../lib5250)

if (HAVE_SYSLOG_H)
    foreach(program scs2ascii scs2pdf scs2ps lp5250d)
        add_executable(${program} ${program}.c)
        add_dependencies(${program} 5250)
        target_link_libraries(${program} 5250 ${PLATFORM_LIBRARIES})
        install(TARGETS ${program})
    endforeach(program)
endif()
