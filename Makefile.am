## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = tn5250-48x48.xpm\
        tn5250-48x48.png\
        tn5250-62x48.xpm\
        tn5250-62x48.png\
        XTerm\
        README.ssl

SUBDIRS = lib5250 lp5250d curses doc termcaps/freebsd termcaps/linux termcaps/sun win32
DIST_SUBDIRS = lib5250 lp5250d curses doc termcaps/freebsd termcaps/linux termcaps/sun win32

bin_SCRIPTS = xt5250

pkgdata_DATA = XTerm

CLEANFILES = *~
