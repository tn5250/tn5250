#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tn5250],[0.19.0],[https://github.com/tn5250/tn5250/issues])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS(config.h)

#
# Automake
#
AH_TEMPLATE([SOCKET_TYPE], [Define to your system's socket descriptor type.])
AH_TEMPLATE([USE_OWN_KEY_PARSING], [Define to let cursesterm handle key mapping.])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h sys/wait.h sys/time.h syslog.h unistd.h pwd.h])

# True for anything other than Windoze.
AC_DEFINE_UNQUOTED(SOCKET_TYPE,int)

# User specified optional packages
AC_ARG_WITH([curses-old-keys],
    AS_HELP_STRING([--with-curses-old-keys],[use curses built-in key handling (default=yes)]),
    [ac_cv_use_old_keys=$withval],
    [ac_cv_use_old_keys=yes])
AC_CACHE_CHECK([whether to use old key handler],
    [ac_cv_use_old_keys],
    [ac_cv_use_old_keys=no])

AC_ARG_WITH([terminfo],
    AS_HELP_STRING([--with-terminfo],[install terminfo entries for your platform (default=no)]),
    [ac_cv_install_terminfo=$withval],
    [ac_cv_install_terminfo=no])
AC_CACHE_CHECK([whether to install terminfo entries],
    [ac_cv_install_terminfo],
    [ac_cv_install_terminfo=no])
AM_CONDITIONAL([INSTALL_TERMINFO], [test "$ac_cv_install_terminfo" = "yes"])

# run AC_CHECK_LIB this way to avoid linking curses into every executable
#
# Displaying UTF-8 takes the wide character functions of curses.  They often
# are in a library of their own (ncursesw), which is then the one to use, or
# else the plain curses library may have them.
AC_CHECK_LIB(ncursesw, initscr, CURSES_LIBS=-lncursesw)
if test "$ac_cv_lib_ncursesw_initscr" = "yes"
then
    # Check if we need libtinfo too, see https://bugs.gentoo.org/457530#c0
    AC_CHECK_LIB(tinfow, nodelay, CURSES_LIBS="$CURSES_LIBS -ltinfow",
        [AC_CHECK_LIB(tinfo, nodelay, CURSES_LIBS="$CURSES_LIBS -ltinfo")])
    # ncursesw may come with header files of its own.
    AC_CHECK_HEADERS([ncursesw/ncurses.h ncursesw/curses.h], [break])
else
    AC_CHECK_LIB(ncurses, initscr, CURSES_LIBS=-lncurses)
    if test "$ac_cv_lib_ncurses_initscr" = "yes"
    then
        # Check if we need libtinfo too, see https://bugs.gentoo.org/457530#c0
        AC_CHECK_LIB(tinfo, nodelay, CURSES_LIBS="$CURSES_LIBS -ltinfo")
    else
        # Check for -lcurses if -lncurses isn't found.
        AC_CHECK_LIB(curses, initscr, CURSES_LIBS='-lcurses', AC_MSG_ERROR([** You need a curses-compatible library installed.]))
    fi
fi

AC_CACHE_CHECK([whether curses has wide character functions],
    [tn5250_cv_curses_widechar],
    [tn5250_save_LIBS="$LIBS"
    LIBS="$CURSES_LIBS $LIBS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#define _XOPEN_SOURCE_EXTENDED 1
#define NCURSES_WIDECHAR 1
#if defined(HAVE_NCURSESW_NCURSES_H)
#include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSESW_CURSES_H)
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif
#include <wchar.h>
]], [[
cchar_t cell;
wchar_t text[2] = { L'x', 0 };
setcchar(&cell, text, A_NORMAL, 0, 0);
return add_wch(&cell);
]])],
        [tn5250_cv_curses_widechar=yes],
        [tn5250_cv_curses_widechar=no])
    LIBS="$tn5250_save_LIBS"])
if test "$tn5250_cv_curses_widechar" = "yes"
then
    AC_DEFINE([HAVE_CURSES_WIDECHAR], [1], [Define to 1 if curses has the wide character functions, which UTF-8 support needs.])
fi

# What "make check" needs to know: some of the tests take Python, and what
# they expect depends on the choices above.
AM_PATH_PYTHON([3.6],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])
AM_CONDITIONAL([CURSES_WIDECHAR], [test "$tn5250_cv_curses_widechar" = "yes"])
AM_CONDITIONAL([OWN_KEY_PARSING], [test "$ac_cv_use_old_keys" != "yes"])

if test "$ac_cv_use_old_keys" != "yes";
then
    AC_DEFINE_UNQUOTED(USE_OWN_KEY_PARSING,1)
fi

AC_ARG_WITH([ssl],AS_HELP_STRING([--with-ssl@<:@=DIR@:>@],[OpenSSL support (yes/no/path to OpenSSL) @<:@default=yes@:>@]))

case "$with_ssl" in
    no) ;;
    yes | '')
        OPENSSL_DIRS="/usr/local /usr /usr/pkg /usr/lib /var/ssl /opt"
        with_ssl=yes
        ;;
    *)
        OPENSSL_DIRS="$with_ssl"
        with_ssl=yes
        ;;
esac

if test "$with_ssl" = "yes"; then
    AC_MSG_CHECKING([for OpenSSL headers])
    for topdir in $OPENSSL_DIRS
    do
        if test -e "$topdir/include/openssl/ssl.h"; then
            OPENSSLDIR="$topdir"
            break
        fi
    done

    if test -z "$OPENSSLDIR"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([** Unable to find OpenSSL headers! Specify --with-ssl=DIR to specify the path to the OpenSSL installation.])
      else
        AC_MSG_RESULT([$OPENSSLDIR])
     fi

    CPPFLAGS="$CPPFLAGS -I$OPENSSLDIR/include"
    LDFLAGS="$LDFLAGS -L$OPENSSLDIR/lib"

    AC_CHECK_LIB(crypto, OPENSSL_init, [], AC_MSG_ERROR([** Unable to find OpenSSL libraries!]))
    AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [], AC_MSG_ERROR([** Unable to find OpenSSL libraries!]))
fi

AC_SUBST([CURSES_LIBS])

dnl Host-specific terminal information; these often are based on the OS's built
dnl in console terminal capabilities, so they aren't portable. Especially if
dnl they differ between termcap vs. terminfo...
dnl
dnl XXX: Is the Linux terminfo entry geneerally useful?
dnl XXX: Currently, only Linux is refactored to not trample system directories.
dnl FreeBSD termcap needs refactoring, Solaris makefiles don't install.
case "${host_os}" in
    linux*)
        ac_cv_build_linux=yes
        ;;
esac

AM_CONDITIONAL([LINUX], [test "$ac_cv_build_linux" = "yes"])

AC_CONFIG_FILES([Makefile
		 curses/Makefile
		 doc/Makefile
		 lib5250/Makefile
		 lp5250d/Makefile
		 termcaps/freebsd/Makefile
		 termcaps/linux/Makefile
		 termcaps/sun/Makefile
		 tests/Makefile
		 tn5250.pc
		 win32/Makefile
		 xt5250])
AC_OUTPUT
