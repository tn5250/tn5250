cmake_minimum_required(VERSION 3.12)
project(macos-termcaps)

# Install terminfo files for macOS
set(TERMINFO_FILES
    5250.terminfo
)

set(DOC_FILES
    README
)

# Install documentation
install(FILES ${DOC_FILES}
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/termcaps/macos
)

# Install terminfo source files
install(FILES ${TERMINFO_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/tn5250/termcaps/macos
)

# Optionally compile and install terminfo entries
option(INSTALL_TERMINFO "Install compiled terminfo entries" OFF)

if(INSTALL_TERMINFO)
    find_program(TIC_PROGRAM tic)
    if(TIC_PROGRAM)
        # Create custom target to compile terminfo
        add_custom_target(compile-terminfo-macos ALL
            COMMAND ${TIC_PROGRAM} -x -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/5250.terminfo
            COMMENT "Compiling terminfo entries for macOS"
            VERBATIM
        )
        
        # Install compiled terminfo entries
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/5/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/terminfo/5
            FILES_MATCHING PATTERN "5250"
        )
        
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/x/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/terminfo/x
            FILES_MATCHING PATTERN "xterm-5250"
        )
    else()
        message(WARNING "tic program not found - terminfo entries will not be compiled")
    endif()
endif()
