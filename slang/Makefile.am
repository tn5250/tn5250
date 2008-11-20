## Process this file with automake to produce Makefile.in

bin_PROGRAMS =		tn5250-slang

LDADD = ../lib5250/lib5250.la

tn5250_slang_SOURCES = 	slangterm.c

tn5250_slang_CFLAGS = -DUSE_SLANG $(AM_CFLAGS)

tn5250_slang_LDFLAGS = $(SLANG_LIB)

pkginclude_HEADERS = 	slangterm.h

INCLUDES = -DSYSCONFDIR=\"$(sysconfdir)\" -I$(top_srcdir)/lib5250
