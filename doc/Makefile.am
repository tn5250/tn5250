
man_MANS =		scs2ascii.1\
			scs2pdf.1\
			scs2ps.1\
			tn5250.1\
			lp5250d.1\
			tn5250rc.5

EXTRA_DIST =		$(man_MANS)

html-local:
	robodoc --src $(top_srcdir)/src/ --doc lib5250 --singledoc --html \
		--toc --cmode --nodesc

clean-local:
	rm -f lib5250.html lib5250.css
