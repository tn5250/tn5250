tn5250
======

This is an implementation of the 5250 telnet protocol. It was originally an
implementation for Linux, but it has been reportedly compiled on a number
of other platforms. Contributed keyboard maps and termcap entries for
FreeBSD are in this tarball as well (see freebsd/README for more information).

Building from Git
-----------------

Skip to "Building and Installing" below if you got these sources from a
.tar.gz release file.

Certain files, such as the libtool support files and some shell scripts
which replace possibly missing commands on the target system are not in
git because we don't maintain them. They can be installed with the
following command:

```bash
./autogen.sh
```

This command requires current versions of the following packages, and the
generated files may not work properly.

```bash
automake
autoconf
libtool
```

You may receive an error the first time you run this script. If so, run
the script a second time to make sure you don't get an error (this is a bug
with automake).


Building and Installing
-----------------------

To build the emulator simply type the following:

```bash
./configure
make
make install
```

Additional (but decidedly generic) installation instructions are available
in the INSTALL file included in this distribution tarfile. Installation
instructions specific to your platform exist if you are using Linux or
FreeBSD -- they are in the linux/ and freebsd/ directories, respectively.
Please read these before telling us that the function keys don't work ;-)

The emulator uses the ncurses library for manipulating the console. Make
sure you have the ncurses development libraries installed before trying to
compile the source. There have been both reports of the standard BSD curses
working and not working, so you may have to install ncurses under *BSD.

X Windows
---------

To use the emulator under X Windows, use the provided `xt5250` shell script,
which sets up a standard `xterm` (it will *not* work with an `nxterm` or an
`rxvt` terminal).

There is one common problem which would cause `xt5250` to flash once on the
screen then disappear. If the termcap or terminfo entry for the "xterm-5250"
terminal type does not exist, `xterm` will exit immediately.

Other Information
-----------------

Other information is available on the web.

- [tn5250 Homepage](https://tn5250.github.io)
- [Issue tracker](https://github.com/tn5250/tn5250/issues)
- [Former home at SourceForge](http://sourceforge.net/projects/tn5250/)

Enjoy!
