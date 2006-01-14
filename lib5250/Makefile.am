## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES =	lib5250.la

lib5250_la_SOURCES =	buffer.c\
			conf.c\
			dbuffer.c\
			debug.c\
			display.c\
			field.c\
			macro.c\
			menu.c\
			printsession.c\
			record.c\
			scrollbar.c\
			scs.c\
			session.c\
			sslstream.c\
			stream.c\
			telnetstr.c\
			terminal.c\
			utility.c\
			version.c\
			window.c\
			wtd.c

INCLUDES = -DSYSCONFDIR=\"$(sysconfdir)\"

pkginclude_HEADERS = 	buffer.h\
		 	codes5250.h\
			conf.h\
			dbuffer.h\
			debug.h\
			display.h\
			field.h\
			macro.h\
			menu.h\
			printsession.h\
			record.h\
			scrollbar.h\
			scs.h\
			session.h\
			stream.h\
			terminal.h\
			utility.h\
			window.h\
			wtd.h

include_HEADERS =	tn5250.h

noinst_HEADERS =	transmaps.h\
			tn5250-private.h
