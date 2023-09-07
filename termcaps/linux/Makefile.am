
EXTRA_DIST	= 5250.tcap\
                  5250.terminfo\
		  README\
		  uk5250.map\
		  us5250.map

pkgdata_DATA	= uk5250.map\
		  us5250.map

#
# Here we automagically run tic if we can determine that the system
# is Linux and we can find the tic command (and make install was
# run as root).
#
install-data-local:
	@( good=no ; \
	  if [ "$$(uname -s)" = "Linux" ]; then \
            if which tic >/dev/null 2>&1 ; then \
	      if [ "`whoami`" = "root" ]; then \
		rm -f /usr/share/terminfo/x/xterm-5250 ; \
		rm -f /usr/share/terminfo/5/5250 ; \
		rm -f /usr/lib/terminfo/x/xterm-5250 ; \
		rm -f /usr/lib/terminfo/5/5250 ; \
		good=yes ; \
		tic $(srcdir)/5250.terminfo || good=no ; \
	      else \
		mkdir -p ~/.terminfo || exit $$? ; \
		rm -f ~/.terminfo/x/xterm-5250 ; \
		rm -f ~/.terminfo/5/5250 ; \
		good=yes ; \
		tic $(srcdir)/5250.terminfo || good=no ; \
	      fi ; \
	    fi ; \
	  else \
	    good=yes ; \
	  fi ; \
	  if [ x$$good = xno ] ; then \
	    echo "**" >&2 ; \
	    echo "** Could not install terminfo entry automatically.  Possible" >&2 ; \
	    echo "** reasons for this include:" >&2 ; \
	    echo "**   - Could not find 'tic' command." >&2 ; \
	    echo "**   - 'make install' was not run as root." >&2 ; \
	    echo "**" >&2 ; \
	    echo "** Some keys may not work until this is installed, please read" >&2 ; \
	    echo "** the linux/README file for instructions on how to install" >&2 ; \
	    echo "** this manually." >&2 ; \
	    echo "**" >&2 ; \
	  fi )

