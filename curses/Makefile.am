## Process this file with automake to produce Makefile.in

bin_PROGRAMS =		tn5250

LDADD = ../lib5250/lib5250.la

tn5250_SOURCES = 	cursesterm.c tn5250.c

tn5250_CFLAGS = -DUSE_CURSES $(AM_CFLAGS)

tn5250_LDFLAGS = $(CURSES_LIB)

pkginclude_HEADERS = 	cursesterm.h

INCLUDES = -DSYSCONFDIR=\"$(sysconfdir)\" -I$(top_srcdir)/lib5250
