#!/bin/sh
#
# autogen.sh - Create generated files which aren't stored in the CVS
#              repository.
#

aclocal || exit $?
autoconf || exit $?
autoheader || exit $?

#
# Sometimes we need to run automake twice, as it will fail the first time when
# installing ltconfig/ltmain.sh even though it successfully installs the files.
#
automake --add-missing --include-deps \
	|| automake --add-missing --include-deps \
	|| exit $?
