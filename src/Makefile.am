## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES =	lib5250.la

bin_PROGRAMS =		scs2ascii\
			tn5250\
			lp5250d

lib5250_la_SOURCES =	buffer.c\
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
			telnetstr.c\
			utility.c\
			version.c\
			wtd.c

lib5250_la_LIBADD =	@with_extra_libs@

tn5250_SOURCES = 	tn5250.c

tn5250_LDADD = 		@with_extra_libs@ lib5250.la

scs2ascii_SOURCES =	scs2ascii.c

scs2ascii_LDADD =	@with_extra_libs@ lib5250.la

lp5250d_SOURCES =	lp5250d.c

lp5250d_LDADD =		@with_extra_libs@ lib5250.la

pkginclude_HEADERS = 	buffer.h\
		 	codes5250.h\
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
			transmaps.h\
			utility.h\
			wtd.h

include_HEADERS =	tn5250.h

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

.PHONY: lclint
