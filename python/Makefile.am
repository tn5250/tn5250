## Process this file with automake to produce Makefile.in

INCLUDES = @PYTHON_CFLAGS@ \
	-DSYSCONFDIR=\"$(sysconfdir)\"

# Here's where we build the python module.
pysodir =		@PYTHON_PREFIX@/lib/python@PYTHON_VERSION@/lib-dynload
pyso_LTLIBRARIES =	libtn5250module.la

# For some icky reason, automake or libtool pukes on the below line.  So we
# just add all the SOURCES (sigh).
# libtn5250module_la_LIBADD =	lib5250.la

libtn5250module_la_LIBADD =	@with_exra_libs@

libtn5250module_la_SOURCES =	tn5250-python.c\
			$(lib5250_la_SOURCES)

install-exec-local:
	( cd $(pysodir) && ln -sf libtn5250module.so tn5250module.so )
