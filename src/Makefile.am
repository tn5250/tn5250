## Process this file with automake to produce Makefile.in

#
# LIBSOURCES are sources which implement functions which might be missing
# on this system, but we don't want to include them in the built libs. We
# also need to put them in EXTRA_DIST.
#
if LIB_GETOPT
LIBSOURCES =		getopt.c
else
LIBSOURCES =		
endif

BUILT_SOURCES		= _tn5250.i _tn5250_wrap.c

lib_LTLIBRARIES =	lib5250.la

bin_PROGRAMS =		scs2ascii\
			tn5250\
			lp5250d

lib5250_la_SOURCES =	buffer.c\
			conf.c\
			cursesterm.c\
			debug.c\
			dbuffer.c\
			display.c\
			field.c\
			printsession.c\
			record.c\
			session.c\
			slangterm.c\
			stream.c\
			terminal.c\
			telnetstr.c\
			utility.c\
			version.c\
			wtd.c

lib5250_la_LIBADD =	@with_extra_libs@

# Here's where we build the python module.
if PYTHON
pysodir =		@PYTHON_PREFIX@/lib/python@PYTHON_VERSION@/lib-dynload
pyso_LTLIBRARIES =	lib_tn5250module.la
INCLUDES +=		-I@PYTHON_PREFIX@/include/python@PYTHON_VERSION@

# For some icky reason, automake or libtool pukes on the below line.  So we
# just add all the SOURCES (sigh).
# lib_tn5250module_la_LIBADD =	lib5250.la

lib_tn5250module_la_SOURCES =	_tn5250_wrap.c\
			$(lib5250_la_SOURCES)
endif


tn5250_SOURCES = 	tn5250.c\
		        $(LIBSOURCES)

tn5250_LDADD = 		@with_extra_libs@ lib5250.la

scs2ascii_SOURCES =	scs2ascii.c\
			$(LIBSOURCES)

scs2ascii_LDADD =	@with_extra_libs@ lib5250.la

lp5250d_SOURCES =	lp5250d.c\
			$(LIBSOURCES)

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
			printsession.h\
			record.h\
			scs.h\
			session.h\
			slangterm.h\
			stream.h\
			terminal.h\
			utility.h\
			wtd.h

include_HEADERS =	tn5250.h

noinst_HEADERS =	getopt.h\
			transmaps.h\
			tn5250-private.h\
			swig-5250.h

EXTRA_DIST =		.lclintrc\
			transmaps\
			getopt.c

# This hack probably only works with gcc
_tn5250.i: swig-5250.h
	$(CC) -E -P $(CFLAGS) swig-5250.h |sed -e '/^[ 	]*$$/d' \
		>_tn5250.i

_tn5250_wrap.c: _tn5250.i _tn5250_extra.i
	$(SWIG) -python _tn5250.i
	
lclint:
	@echo "lint checking with lclint... "
	@( lclint tn5250.c $(lib5250_la_SOURCES) > lclint.err 2>&1 ; \
	  retval=$$? ; \
	  echo "`grep -c :.*:.*: lclint.err` errors." ; \
	  exit $$retval )

config.h: tn5250-config.h
	cat tn5250-config.h |sed -e 's/VERSION/TN5250_LIB_VERSION/g' -e '/PACKAGE/d' >config.h

#
# We generate our library docs using robodoc.
#
DOCFILES = $(lib5250_la_SOURCES) $(pkginclude_HEADERS)

docs:
	( rm -f index.xrefs ; \
	  for file in $(DOCFILES) ; do \
	    robodoc $$file ../doc/$$file.html GENXREF ../doc/$$file.xref ; \
	    echo "../doc/$$file.xref" >>../doc/index.xrefs ; \
	  done ; \
	  for file in $(DOCFILES) ; do \
	    robodoc $$file ../doc/$$file.html HTML -x ../doc/index.xrefs ; \
	  done ; \
	  robodoc ../doc/index.xrefs ../doc/index.html INDEX ; \
	  rm -f ../doc/*.xref ../doc/index.xrefs )

clean-local:
	rm -f *~ *.xref index.xrefs index.html

if PYTHON
install-exec-local:
	( cd $(pysodir) && ln -sf lib_tn5250module.so _tn5250module.so )
endif

.PHONY: lclint

