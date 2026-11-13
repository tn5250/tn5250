## Process this file with automake to produce Makefile.in

EXTRA_DIST = CMakeLists.txt\
		test_terminal.py\
		tn5250test.py

check_PROGRAMS =	charmaptest

charmaptest_SOURCES =	charmaptest.c

LDADD = ../lib5250/lib5250.la

AM_CPPFLAGS = -I$(top_srcdir)/lib5250

# The character maps of lib5250, and what goes with them.
TESTS = charmaptest

# The emulator from end to end, which takes Python and a pseudo terminal.
if HAVE_PYTHON
TESTS += test_terminal.py
endif

TEST_EXTENSIONS = .py
PY_LOG_COMPILER = $(PYTHON)

if CURSES_WIDECHAR
test_utf8 = yes
else
test_utf8 = no
endif

if OWN_KEY_PARSING
test_own_keys = yes
else
test_own_keys = no
endif

AM_TESTS_ENVIRONMENT = \
	TN5250='$(abs_top_builddir)/curses/tn5250'; export TN5250; \
	TN5250_CHARMAPTEST='$(abs_builddir)/charmaptest'; \
	export TN5250_CHARMAPTEST; \
	TN5250_UTF8=$(test_utf8); export TN5250_UTF8; \
	TN5250_OWN_KEYS=$(test_own_keys); export TN5250_OWN_KEYS;
