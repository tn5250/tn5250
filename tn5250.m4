dnl   Automake macros for working with lib5250.
dnl  
dnl     Copyright (C) 2000 Jason M. Felice
dnl   
dnl   This file is part of Tn5250.
dnl   
dnl   TN5250 is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU Lesser General Public License as 
dnl   published by dnl the Free Software Foundation; either version 2.1,
dnl   or (at your option) any later version.
dnl   
dnl   TN5250 is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU Lesser General Public License for more details.
dnl   
dnl   You should have received a copy of the GNU Lesser General Public 
dnl   License along with TN5250; see the file COPYING.  If not, write to
dnl   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
dnl   Boston, MA 02111-1307 USA
dnl   
dnl

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
