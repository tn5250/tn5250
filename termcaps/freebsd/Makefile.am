
EXTRA_DIST	= us.5250.kbd\
		  5250keys\
		  README\
		  sample.termcap

pkgdata_DATA	= us.5250.kbd \
		  sample.termcap \
		  README

bin_SCRIPTS 	= 5250keys

#
# here we install our termcap entry 
#
install-data-local:
	@( good=no ; \
	   termcapfile=$(prefix)/../share/misc/termcap ; \
	  if [ "`uname -s`" = "FreeBSD" ]; then \
	      if [ ! -f $$termcapfile ]; then \
	         good=yes ; \
	      else \
                 grep -q "xterm-5250" $$termcapfile || good=yes; \
                 if [ x$$good = xyes ]; then \
		     echo "Adding termcap entry..." ; \
	             cat sample.termcap >> $$termcapfile; \
                     cap_mkdb $$termcapfile ; \
	         fi ; \
	      fi ; \
	  else \
	      good=yes ; \
	  fi ; \
	  if [ x$$good = xno ] ; then \
	    echo "**" >&2 ; \
	    echo "** Could not install termcap entry automatically.  Possible" >&2 ; \
	    echo "** reasons for this include:" >&2 ; \
	    echo "**   - You already had an xterm-5250 in your termcap " >&2 ; \
	    echo "**   - termcap isn't installed in its usual place " >&2 ; \
	    echo "**" >&2 ; \
	    echo "** Some keys may not work until this is installed, please read" >&2 ; \
	    echo "** the freebsd/README file for instructions on how to install" >&2 ; \
	    echo "** this manually." >&2 ; \
	    echo "**" >&2 ; \
	  fi )

