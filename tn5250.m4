dnl   Automake macros for working with lib5250.
dnl  
dnl     Copyright (C) 2000 Jason M. Felice
dnl   
dnl   This program is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License as published by
dnl   the Free Software Foundation; either version 2, or (at your option)
dnl   any later version.
dnl   
dnl   This program is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl   
dnl   You should have received a copy of the GNU General Public License
dnl   along with this software; see the file COPYING.  If not, write to
dnl   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
dnl   Boston, MA 02111-1307 USA
dnl   
dnl   As a special exception, the Free Software Foundation gives permission
dnl   for additional uses of the text contained in its release of GUILE.
dnl   
dnl   The exception is that, if you link the GUILE library with other files
dnl   to produce an executable, this does not by itself cause the
dnl   resulting executable to be covered by the GNU General Public License.
dnl   Your use of that executable is in no way restricted on account of
dnl   linking the GUILE library code into it.
dnl   
dnl   This exception does not however invalidate any other reasons why
dnl   the executable file might be covered by the GNU General Public License.
dnl   
dnl   This exception applies only to the code released by the
dnl   Free Software Foundation under the name GUILE.  If you copy
dnl   code from other Free Software Foundation releases into a copy of
dnl   GUILE, as the General Public License permits, the exception does
dnl   not apply to the code that you add in this way.  To avoid misleading
dnl   anyone as to the status of such modified files, you must delete
dnl   this exception notice from them.
dnl   
dnl   If you write modifications of your own for TN5250, it is your choice
dnl   whether to permit this exception to apply to your modifications.
dnl   If you do not wish that, delete this exception notice.

# We might eventually want to provide a --with-tn5250-prefix option like
# GTK+ does.  This will eliminate some confusion when there are multiple
# installed copies or cross-compiling.
AC_DEFUN([AM_PATH_TN5250],[
  ifelse($#,1,[
    AC_MSG_CHECKING(for tn5250 >= $1)
  ],[
    AC_MSG_CHECKING(for tn5250)
  ])
  if ! command -v tn5250-config >/dev/null 2>&1 ; then
    TN5250=no
  else
    TN5250=yes
    TN5250_CFLAGS="`tn5250-config --cflags`"
    TN5250_LIBS="`tn5250-config --libs`"
    if tn5250-config --version >/dev/null 2>&1 ; then
      TN5250_VERSION="`tn5250-config --version`"
    else
      TN5250_VERSION="`tn5250 -V |sed 's/^.*version *//'`"
    fi

    dnl   If another argument is provided, it is the minimum version
    dnl   required by this program.
    ifelse($#,1,[
      # Make sure the version is acceptable.
      eval `echo "$TN5250_VERSION" |sed 's/^\(.*\)\.\(.*\)\.\(.*\)$/tn5250_major=\1 tn5250_minor=\2 tn5250_micro=\3/'`
      eval `echo "$1" |sed 's/^\(.*\)\.\(.*\)\.\(.*\)$/t_major=\1 t_minor=\2 t_micro=\3/'`
      if test $t_major -gt $tn5250_major ; then
        TN5250=no 
      else
        if test $t_major -eq $tn5250_major -a $t_minor -gt $tn5250_minor ; then
	  TN5250=no
	else
	  if test $t_minor -eq $tn5250_minor -a $t_micro -gt $tn5250_micro ; then
	    TN5250=no
	  fi
	fi
      fi
    ])
  fi
  if test x$TN5250 = xyes ; then
    CFLAGS="$CFLAGS $TN5250_CFLAGS"
    LIBS="$LIBS $TN5250_LIBS"
  fi
  AC_MSG_RESULT($TN5250)
])
