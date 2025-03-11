#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tn5250],[0.18.0],[https://github.com/tn5250/tn5250/issues])
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
AC_CHECK_HEADERS([fcntl.h locale.h sys/wait.h sys/time.h syslog.h unistd.h pwd.h])

# True for anything other than Windoze.
AC_DEFINE_UNQUOTED(SOCKET_TYPE,int)

# User specified optional packages
AC_ARG_WITH([curses-old-keys],
    AS_HELP_STRING([--with-curses-old-keys],[curses terminal will use old key handler (default=no)]),
    [ac_cv_use_old_keys=$withval],
    [ac_cv_use_old_keys=no])
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
AC_CHECK_LIB(ncurses, initscr, CURSES_LIBS=-lncurses)
if test "$ac_cv_lib_ncurses_initscr" = "yes"
then
    # Check if we need libtinfo too, see https://bugs.gentoo.org/457530#c0
    AC_CHECK_LIB(tinfo, nodelay, CURSES_LIBS="$CURSES_LIBS -ltinfo")
else
    # Check for -lcurses if -lncurses isn't found.
    AC_CHECK_LIB(curses, initscr, CURSES_LIBS='-lcurses', AC_MSG_ERROR([** You need a curses-compatible library installed.]))
fi

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
		 tn5250.pc
		 win32/Makefile
		 xt5250])
AC_OUTPUT
