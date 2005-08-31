## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES =	lib5250.la

bin_PROGRAMS =		scs2ascii\
			scs2pdf\
                        scs2ps\
			tn5250\
			lp5250d

lib5250_la_SOURCES =	buffer.c\
			conf.c\
			cursesterm.c\
			debug.c\
			dbuffer.c\
			display.c\
			field.c\
			menu.c\
			printsession.c\
			record.c\
			scrollbar.c\
			scs.c\
			session.c\
			slangterm.c\
			sslstream.c\
			stream.c\
			terminal.c\
			telnetstr.c\
			utility.c\
			version.c\
			wtd.c\
			macro.c\
			window.c

lib5250_la_LIBADD =	@with_extra_libs@

INCLUDES = @PYTHON_CFLAGS@ \
	-DSYSCONFDIR=\"$(sysconfdir)\"

# Here's where we build the python module.
if PYTHON
pysodir =		@PYTHON_PREFIX@/lib/python@PYTHON_VERSION@/lib-dynload
pyso_LTLIBRARIES =	libtn5250module.la

# For some icky reason, automake or libtool pukes on the below line.  So we
# just add all the SOURCES (sigh).
# libtn5250module_la_LIBADD =	lib5250.la

libtn5250module_la_LIBADD =	@with_exra_libs@

libtn5250module_la_SOURCES =	tn5250-python.c\
			$(lib5250_la_SOURCES)
endif


tn5250_SOURCES = 	tn5250.c

tn5250_LDADD = 		@with_extra_libs@ lib5250.la

scs2ascii_SOURCES =	scs2ascii.c

scs2ascii_LDADD =	@with_extra_libs@ lib5250.la

scs2pdf_SOURCES =	scs2pdf.c

scs2pdf_LDADD =		@with_extra_libs@ lib5250.la

scs2ps_SOURCES = 	scs2ps.c

scs2ps_LDADD = 		@with_extra_libs@ lib5250.la

lp5250d_SOURCES =	lp5250d.c

lp5250d_LDADD =		@with_extra_libs@ lib5250.la

pkginclude_HEADERS = 	buffer.h\
		 	codes5250.h\
			conf.h\
			config.h\
		 	cursesterm.h\
			debug.h\
			dbuffer.h\
			display.h\
			field.h\
			menu.h\
			printsession.h\
			record.h\
			scrollbar.h\
			scs.h\
			session.h\
			slangterm.h\
			stream.h\
			terminal.h\
			utility.h\
			wtd.h\
			window.h\
			macro.h

include_HEADERS =	tn5250.h

noinst_HEADERS =	transmaps.h\
			tn5250-private.h

EXTRA_DIST =		.lclintrc\
			transmaps

lclint:
	@echo "lint checking with lclint... "
	@( lclint tn5250.c $(lib5250_la_SOURCES) > lclint.err 2>&1 ; \
	  retval=$$? ; \
	  echo "`grep -c :.*:.*: lclint.err` errors." ; \
	  exit $$retval )

config.h: tn5250-config.h
	cat tn5250-config.h |sed -e 's/VERSION/TN5250_LIB_VERSION/g' -e '/PACKAGE/d' >config.h

clean-local:
	rm -f *~

distclean-local:
	rm -f config.h

install-exec-local:
if PYTHON
	( cd $(pysodir) && ln -sf libtn5250module.so tn5250module.so )
else
	: 
endif

.PHONY: lclint

