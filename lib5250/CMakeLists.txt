cmake_minimum_required(VERSION 3.12)
project(lib5250 LANGUAGES C)

include_directories(${CMAKE_BINARY_DIR})

add_library(5250 STATIC buffer.c conf.c dbuffer.c debug.c display.c field.c macro.c menu.c printsession.c record.c scrollbar.c scs.c session.c sslstream.c stream.c telnetstr.c terminal.c utility.c version.c window.c wtd.c buffer.h codes5250.h conf.h dbuffer.h debug.h display.h field.h macro.h menu.h printsession.h record.h scrollbar.h scs.h session.h stream.h terminal.h utility.h window.h wtd.h transmaps.h scs-private.h tn5250-private.h)

if (${OPENSSL_FOUND})
    target_link_libraries(5250 OpenSSL::Crypto OpenSSL::SSL)
endif()

if (WIN32)
    target_link_libraries(5250 Ws2_32 Winmm)
endif()
