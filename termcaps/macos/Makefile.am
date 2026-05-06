EXTRA_DIST	= 5250.terminfo\
		  README

if INSTALL_TERMINFO
if MACOS
all: 5/5250 x/xterm-5250

# XXX: This executes tic twice
5/5250 x/xterm-5250: 5250.terminfo
	tic -x -o . $<

# macOS terminfo is typically in /usr/share/terminfo
install-data-local:
	$(MKDIR_P) $(DESTDIR)$(datadir)/terminfo/5
	$(INSTALL_DATA) 5/5250 $(DESTDIR)$(datadir)/terminfo/5/5250
	$(MKDIR_P) $(DESTDIR)$(datadir)/terminfo/x
	$(INSTALL_DATA) x/xterm-5250 $(DESTDIR)$(datadir)/terminfo/x/xterm-5250

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/terminfo/5/5250
	rm -f $(DESTDIR)$(datadir)/terminfo/x/xterm-5250

clean-local:
	rm -rf ./5 ./x
endif
endif
