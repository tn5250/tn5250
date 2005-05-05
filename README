tn5250 README
------------------------------------------------------------------------------

This is an implementation of the 5250 telnet protocol.  It was originally an
implementation for Linux, but it has been reportedly compiled on a number
of other platforms.  Contributed keyboard maps and termcap entries for
FreeBSD are in this tarball as well (see freebsd/README for more information).

This, the 0.17.x series, is the development series and may cause severe
headaches and stomach cramps, if only due to the quality of code :)  If you
are looking for a stable emulator, get the latest 0.16.x version.  Even so,
0.17.x is quite stable and in production at several sites.  Furthermore, for
the enhanced 5250 protocol 0.17.x is required.

Building from CVS
=================

Skip to ``Building and Installing'' below if you got these sources from a
.tar.gz release file or a CVS snapshot (as opposed to using `cvs checkout'
or `cvs update' to retreive the sources).

Certain files, such as the libtool support files and some shell scripts
which replace possibly missing commands on the target system are not in
CVS because we don't maintain them.  They can be installed with the
following command:

   ./autogen.sh

This command requires current versions of the following packages, and the
generated files may not work properly.

   automake
   autoconf
   libtool

You may receive an error the first time you run this script.  If so, run
the script a second time to make sure you don't get an error (this is a bug
with automake).


Building and Installing
=======================

To build the emulator simply type the following:

   ./configure
   make
   make install

Additional (but decidedly generic) installation instructions are available
in the file `INSTALL' included in this distribution.  Installation
instructions specific to your platform exist if you are using Linux or
FreeBSD -- they are in the linux/ and freebsd/ directories, respectively.
Please read these before telling us that the function keys don't work ;-)

The emulator uses the ncurses library for manipulating the console.  Make
sure you have the ncurses development libraries installed before trying to
compile the source.  There have been both reports of the standard BSD curses
working and not working, so you may have to install GNU curses (ncurses)
under *BSD.

X Windows
=========

To use the emulator under X Windows, use the provided `xt5250' shell script,
which sets up a standard `xterm' (it will *not* work with an `nxterm' or an 
`rxvt' terminal).

There is one common problem which would cause xt5250 to flash once on the
screen then disappear.  If the termcap or terminfo entry for the `xterm-5250'
terminal type does not exist, the `xterm' will exit immediately.

x5250 is an X11 front end (which does not use xterm) written by James Rich.
It can be found at http://www.chowhouse.com/~james/x5250.

Other Information
=================

Other information is available on the web.

http://tn5250.sourceforge.net/                       - linux5250 Homepage
http://www.midrange.com/linux5250.shtml              - linux5250 List Info
http://archive.midrange.com/linux5250/index.htm      - linux5250 List Archives
http://sourceforge.net/projects/tn5250/              - linux5250 at Sourceforge
http://perso.libertysurf.fr/plinux/tn5250-faq.html   - linux5250 FAQ
http://www.chowhouse.com/~james/tn5250-HOWTO.pdf     - linux5250 HOWTO

Comments, questions, bug reports and patches are much appreciated - please
subscribe to the list and post them there if at all possible.  If that's too
much trouble, email one of the maintainers below:

Jason M. Felice <jasonf@nacs.net>
Michael Madore <mmadore@blarg.net>

Enjoy!
