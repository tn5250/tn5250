#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tn5250],[0.17.4],[linux5250@midrange.com])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

#
# Automake
#
AH_TEMPLATE([SOCKET_TYPE], [Define to your system's socket descriptor type.])
AH_TEMPLATE([USE_OWN_KEY_PARSING], [Define to let cursesterm handle key mapping.])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h locale.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h pwd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gethostbyname memset putenv select socket strcasecmp strchr strerror strrchr strstr])

# Get various type sizes
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)

# True for anything other than Windoze.
AC_DEFINE_UNQUOTED(SOCKET_TYPE,int)

# User specified optional packages
AC_ARG_WITH([curses],AS_HELP_STRING([--with-curses],[curses terminal support (default=yes)]),[ac_cv_use_curses=$withval],[ac_cv_use_curses=yes]) AC_CACHE_CHECK([whether to build curses terminal],[ac_cv_use_curses], [ac_cv_use_curses=yes])
AC_ARG_WITH([curses-old-keys],AS_HELP_STRING([--with-curses-old-keys],[curses terminal will use old key handler (default=no)]),[ac_cv_use_old_keys=$withval],[ac_cv_use_old_keys=no]) AC_CACHE_CHECK([whether to use old key handler],[ac_cv_use_old_keys], [ac_cv_use_old_keys=no])


if test "$ac_cv_use_curses" = "yes"
then
	MAYBE_CURSES=curses
# run AC_CHECK_LIB this way to avoid linking curses into every executable
	AC_CHECK_LIB(ncurses, initscr, CURSES_LIB=-lncurses)
	if test "$ac_cv_lib_ncurses_initscr" != "yes"
	then
		# Check for -lcurses if -lncurses isn't found.
		AC_CHECK_LIB(curses, initscr, CURSES_LIB=-lcurses)
		if test "$ac_cv_lib_curses_initscr" != "yes"
		then
			AC_MSG_ERROR([** You need a curses-compatible library installed.])
		fi
	fi
	AC_SUBST([CURSES_LIB])
        if test "$ac_cv_use_old_keys" != "yes";
        then
              AC_DEFINE_UNQUOTED(USE_OWN_KEY_PARSING,1)
        fi
else
	MAYBE_CURSES=
fi

## SLang support is suffering from bit-rot. Nobody is maintaining it, and
## to the best of my knowledge, nobody is using it.  
## Therefore, I'm disabling it, for now. -SK
## # AC_ARG_WITH([slang],AS_HELP_STRING([--with-slang],[slang terminal support (default=no)]),[ac_cv_use_slang=$withval],[ac_cv_use_slang=no]) AC_CACHE_CHECK([whether to build slang terminal],[ac_cv_use_slang], [ac_cv_use_slang=no])

## if test "$ac_cv_use_slang" = "yes"
## then
## 	MAYBE_SLANG=slang
## # run AC_CHECK_LIB this way to avoid linking slang into every executable
## 	AC_CHECK_LIB(slang, SLang_init_tty, SLANG_LIB=-lslang)
## 	if test "$ac_cv_lib_slang_SLang_init_tty" != "yes"
## 	then
## 		AC_MSG_ERROR([** Can't find -lslang **])
## 	fi
## 	AC_SUBST([SLANG_LIB])
## else
## 	MAYBE_SLANG=
## fi
## 
AC_ARG_WITH([python],AS_HELP_STRING([--with-python],[python terminal support (default=no)]),[ac_cv_use_python=$withval],[ac_cv_use_python=no]) AC_CACHE_CHECK([whether to build python terminal],[ac_cv_use_python], [ac_cv_use_python=no])
## 
if test "$ac_cv_use_python" = "yes"
then
	MAYBE_PYTHON=python
else
	MAYBE_PYTHON=
fi

AC_ARG_WITH([ssl],AS_HELP_STRING([--with-ssl],[OpenSSL support (default=yes)]),[ac_cv_use_ssl=$withval],[ac_cv_use_ssl=yes]) AC_CACHE_CHECK([whether to build with OpenSSL support],[ac_cv_use_ssl], [ac_cv_use_ssl=yes])

if test "$ac_cv_use_ssl" = "yes"
then
dnl **
dnl ** Check for OpenSSL include files.
dnl ** They can be in any of a number of standard places.   Most
dnl ** operating systems want them in /usr/include or /usr/local/include,
dnl ** but the default OpenSSL install puts them in
dnl ** ${PREFIX}/ssl/include/openssl
checksslinclude() {
    if test -f "$1/include/openssl/ssl.h"; then
        sslincludedir="-I$1/include"
        return 0
    fi
    return 1
}
AC_MSG_CHECKING([for OpenSSL headers])
AC_ARG_WITH([ssl-dir],AS_HELP_STRING([--with-ssl-dir],[directory for OpenSSL headers]),[
    if test "$with_ssl_dir" != "no"; then
         checksslinclude "$with_ssl"
    else
        for topdir in /usr/local /usr /usr/pkg /usr/lib /var/ssl /opt; do
            for subdir in $topdir $topdir/openssl $topdir/ssl; do
                checksslinclude "$subdir" && break 2
            done
        done
     fi
     if test -z $sslincludedir; then
          AC_MSG_ERROR([** Unable to find OpenSSL headers! Specify --with-ssl=DIR to specify a location.])
     fi
],[
      for topdir in /usr/local /usr /usr/pkg /usr/lib /var/ssl /opt; do
          for subdir in $topdir $topdir/openssl $topdir/ssl; do
              checksslinclude "$subdir" && break 2
          done
      done
])
if test -z $sslincludedir; then
     AC_MSG_RESULT([not found])
else
     AC_MSG_RESULT([$sslincludedir])
fi
AC_SUBST(sslincludedir)

dnl **
dnl ** Check for OpenSSL libraries.
dnl ** happily, we don't have to hunt for them thanks to ldconfig!
dnl **
if test -n $sslincludedir; then
     CPPFLAGS="$CPPFLAGS $sslincludedir"
     AC_CHECK_LIB(crypto,CRYPTO_num_locks)
     if test "$ac_cv_lib_crypto_CRYPTO_num_locks" != "yes"
     then
             AC_MSG_ERROR([** Unable to find OpenSSL libraries!])
     fi
     AC_CHECK_LIB(ssl,SSL_library_init)
     if test "$ac_cv_lib_ssl_SSL_library_init" != "yes"
     then
             AC_MSG_ERROR([** Unable to find OpenSSL libraries!])
     fi
fi

dnl **
dnl ** Is OpenSSL going to require Kerberos?
dnl **
dnl ** In RedHat 9 (and who knows where else?) OpenSSL is compiled with
dnl ** kerberos support, and so it has to be able to find the krb5.h
dnl ** file... which naturally isn't in the standard /usr/include dir
dnl ** since that would be much too easy.
dnl **
checkkrb5config() {
   if test -x "$1/krb5-config"; then
       krb5includedir="`$1/krb5-config --cflags`"
       return 0
   fi
   if test -x "$1/bin/krb5-config"; then
       krb5includedir="`$1/bin/krb5-config --cflags`"
       return 0
   fi
   return 1
}
if test -n "$sslincludedir"; then
   AC_MSG_CHECKING([if OpenSSL needs krb5.h])
   needkrb5=yes
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
]], [[SSL_CTX *ssl_context=NULL;]])],[needkrb5=no],[])
   if test $needkrb5 = "yes"; then
       krb5config="`which krb5config 2> /dev/null`"
       if test -z "$krb5config"; then
           krb5config="nonexistent"
       fi
       if test -x "$krb5config"; then
           krb5includedir="`$krb5config --cflags`"
       else
           for topdir in /usr/local /usr /opt /usr/lib /usr/pkg; do
               for subdir in $topdir $topdir/kerberos $topdir/krb5; do
                    checkkrb5config "$subdir" && break 2
               done
           done
       fi
       if test -n $krb5includedir; then
           CPPFLAGS="$CPPFLAGS $krb5includedir"
           needkrb5=no
	      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
]], [[SSL_CTX *ssl_context=NULL;]])],[needkrb5=yes],[])
       fi
   fi
   AC_SUBST(krb5includedir)
   AC_MSG_RESULT([$needkrb5])
fi
fi

# These aren't working
#AM_CONDITIONAL([COND_CURSES], [test "$ac_cv_use_curses" = yes])
#AM_CONDITIONAL([COND_SLANG], [test "$ac_cv_use_slang" = yes])
#AM_CONDITIONAL([COND_PYTHON], [test "$ac_cv_use_python" = yes])
# Try these instead
AC_SUBST([MAYBE_CURSES])
## AC_SUBST([MAYBE_SLANG])
AC_SUBST([MAYBE_PYTHON])

AC_CONFIG_FILES([Makefile
		 curses/Makefile
		 doc/Makefile
		 lib5250/Makefile
		 lp5250d/Makefile
		 python/Makefile
		 slang/Makefile
		 freebsd/Makefile
		 linux/Makefile
		 sun/Makefile
		 win32/Makefile
		 xt5250])
AC_OUTPUT
