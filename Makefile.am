## Process this file with automake to produce Makefile.in

DIST_SUBDIRS = src doc linux freebsd win32 sun
SUBDIRS = src doc @SUBDIRS_ADD@

bin_SCRIPTS = tn5250-config\
	      xt5250

pkgdata_DATA = dialogrc\
	       XTerm
	       

aclocal_DATA = tn5250.m4

aclocaldir = $(datadir)/aclocal

EXTRA_DIST = acconfig.h\
	tn5250.spec.in\
	tn5250.spec\
	tn5250-48x48.xpm\
	tn5250-48x48.png\
	tn5250-62x48.xpm\
	tn5250-62x48.png\
	BUGS\
	XTerm\
	dialogrc\
	tn5250.m4\
	README.ssl
