## Process this file with automake to produce Makefile.in

EXTRA_DIST = tn5250-48x48.xpm\
        tn5250-48x48.png\
        tn5250-62x48.xpm\
        tn5250-62x48.png\
        XTerm\
        README.ssl

# These aren't working
#if COND_CURSES
#	MAYBE_CURSES = curses
#else
#	MAYBE_CURSES =
#endif

#if COND_SLANG
#	MAYBE_SLANG = slang
#else
#	MAYBE_SLANG =
#endif

#if COND_PYTHON
#	MAYBE_PYTHON = python
#else
#	MAYBE_PYTHON =
#endif

SUBDIRS = lib5250 lp5250d $(MAYBE_CURSES) $(MAYBE_SLANG) $(MAYBE_PYTHON) doc freebsd linux sun win32
DIST_SUBDIRS = lib5250 lp5250d curses slang python doc freebsd linux sun win32

bin_SCRIPTS = xt5250

pkgdata_DATA = XTerm

CLEANFILES = *~
