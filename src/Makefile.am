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

lib_LTLIBRARIES =	lib5250.la

bin_PROGRAMS =		scs2ascii\
			 scs2ps\
			tn5250\
			lp5250d\
			tn5250d\
			tn3270d

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
			sslstream.c\
			stream.c\
			telnetstr.c\
			utility.c\
			version.c\
			wtd.c

lib5250_la_LIBADD =	@with_extra_libs@

tn5250_SOURCES = 	tn5250.c
INCLUDES = -DSYSCONFDIR=\"$(sysconfdir)\"

tn5250_LDADD = 		@with_extra_libs@ lib5250.la

scs2ascii_SOURCES =	scs2ascii.c

scs2ascii_LDADD =	@with_extra_libs@ lib5250.la

lp5250d_SOURCES =	lp5250d.c

lp5250d_LDADD =		@with_extra_libs@ lib5250.la

tn5250d_SOURCES =	tn5250d.c\
			host5250.c

tn5250d_LDADD =		@with_extra_libs@ lib5250.la

tn3270d_SOURCES = 	tn3270d.c\
			host3270.c

tn3270d_LDADD =		@with_extra_libs@ lib5250.la

scs2ps_SOURCES =	scs2ps.c
scs2ps_LDADD =		@with_extra_libs@ lib5250.la

pkginclude_HEADERS = 	buffer.h\
			codes3270.h\
		 	codes5250.h\
			conf.h\
			config.h\
		 	cursesterm.h\
			debug.h\
			dbuffer.h\
			display.h\
			field.h\
			host3270.h\
			host5250.h\
			printsession.h\
			record.h\
			scs.h\
			session.h\
			slangterm.h\
			stream.h\
			terminal.h\
			tn3270d.h\
			tn5250d.h\
			utility.h\
			wtd.h

include_HEADERS =	tn5250.h

noinst_HEADERS =	getopt.h\
			transmaps.h\
			tn5250-private.h

EXTRA_DIST =		.lclintrc\
			transmaps\
			getopt.c

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
	  robodoc ../doc/index.xrefs ../doc/index.html INDEX HTML ; \
	  rm -f ../doc/*.xref ../doc/index.xrefs )

clean-local:
	rm -f *~ *.xref index.xrefs index.html

distclean-local:
	rm -f config.h

.PHONY: lclint

