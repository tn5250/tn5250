
man_MANS =		scs2ascii.1\
 			scs2pdf.1\
			scs2ps.1\
			tn5250.1\
			lp5250d.1\
			tn5250rc.5

#
# These are documentation of the lib5250 API
#
HTMLDOCS =		buffer.c.html\
			buffer.h.html\
			codes5250.h.html\
			config.h.html\
			cursesterm.c.html\
			cursesterm.h.html\
			dbuffer.c.html\
			dbuffer.h.html\
			debug.c.html\
			debug.h.html\
			display.c.html\
			display.h.html\
			field.c.html\
			field.h.html\
			host5250.h.html\
			index.html\
			printsession.c.html\
			printsession.h.html\
			record.c.html\
			record.h.html\
			scs.h.html\
			session.c.html\
			session.h.html\
			slangterm.c.html\
			slangterm.h.html\
			sslstream.c.html\
			stream.c.html\
			stream.h.html\
			telnetstr.c.html\
			terminal.h.html\
			tn5250d.h.html\
			utility.c.html\
			utility.h.html\
			version.c.html\
			wtd.c.html\
			wtd.h.html

#
# This means 'make dist' and 'make distcheck' won't work if you
# don't have robodoc installed (oh, well).
#
EXTRA_DIST =		$(man_MANS)\
			$(HTMLDOCS)

$(addprefix $(top_srcdir),$(HTMLDOCS)): $(top_srcdir)/src/*.c $(top_srcdir)/src/*.h
	$(MAKE) -C ../src docs
