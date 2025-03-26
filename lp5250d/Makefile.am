## Process this file with automake to produce Makefile.in

EXTRA_DIST = CMakeLists.txt

bin_PROGRAMS =		scs2ascii\
			scs2pdf\
			scs2ps\
			lp5250d

LDADD = ../lib5250/lib5250.la

scs2ascii_SOURCES =	scs2ascii.c

scs2pdf_SOURCES =	scs2pdf.c

scs2ps_SOURCES = 	scs2ps.c

lp5250d_SOURCES =	lp5250d.c

AM_CPPFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\" -I$(top_srcdir)/lib5250
