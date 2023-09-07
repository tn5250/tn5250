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

SUBDIRS = lib5250 lp5250d $(MAYBE_CURSES) doc freebsd linux sun win32
DIST_SUBDIRS = lib5250 lp5250d curses doc freebsd linux sun win32

bin_SCRIPTS = xt5250

pkgdata_DATA = XTerm

CLEANFILES = *~
