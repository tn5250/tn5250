CONFIGURING, BUILDING AND INSTALLING UNDER WIN32:
---------------------------------------------------------------

First of all, it should be noted that this project was originally a
Linux implementation of TN5250.  Consequently, most of the documentation
is geared towards Linux and similar operating systems.  In this file, I
will try to give enough information for you to get by under Windows.

Let me introduce myself, I am Scott Klement, one of the developers who
works with the TN5250 project, and it was I who created this Windows port.

SUPPORT:
---------------------
The first place to go for support is our home page:
    http://tn5250.sourceforge.net

If you can't find an answer to your question listed there, you should
check the archives for our mailing list next.  The home page has a link
where you can search the archives.

If you still don't have an answer, click on the "Subscribe to the
Linux5250 mailing list" link on the home page.  Here you will be able
to sign-up for our E-mail discussion list.  All of our developers
(including myself) read this list and we will provide you with as much
assistance as we can.


SYSTEM REQUIREMENTS:
---------------------
   -- Microsoft Windows 95 (R), Windows NT 4.0(R), or higher.

   -- The MinGW compiler & make program

        Note: MinGW is open-source, and available at no charge.
              I will give instructions below on how to get and
              install it.

   -- If you want SSL support, OpenSSL 0.9.6b or higher is
        also required. It was at version 0.9.7f at the time of writing.

        Note: This is also open-source, and documented below.


The slowest computer that I've tested this package on is a Pentium 166
running Win95.  If you have experience with this software on slower
machines, I'd be interested in how it worked out for you.  Please report
your findings to the linux5250 mailing list. (see SUPPORT above for info
on how to sign-up for the mailing list)


GETTING & INSTALLING THE COMPILER:
----------------------------------

The Windows port of TN5250 was developed and tested using the compiler
provided with MinGW.  In theory, it should work with just about any
C compiler capable of making 32-bit Windows programs, but I have only
tried MinGW.

Here's how you install MinGW:

1) Go to MinGW's home page at:
         http://www.mingw.org

2) Click on "Download".  Follow the instructions for downloading it.
     When I did this, it gave me a link to SourceForge, so I clicked
     that.  When the SourceForge "File List" page came up, I clicked
     on MinGW-1.1.tar.gz, which was the newest version available at
     the time.  My web browser downloaded this file to my hard drive.

3) The ".tar.gz" in the filename means that this is a gzipped tarball.
     You'll need to unpack this.  I used WinZip(R) to do this under
     Windows, since the standard UN*X tar command is unavailable. You
     can get WinZip from:   http://www.winzip.com

4) After installing WinZip, I double-clicked on the "MinGW-1.1.tar.gz"
     file that I downloaded, and WinZip said to me:
        "Arhive contains one file:  MinGW-1.1.tar
         Should WinZip decompress it to temporary folder and open it?"
     I clicked "Yes".

5) Now, in WinZip (I'm using the "Classic" interface) I clicked
     "Extract".  I selected  "All files", and made sure "Use folder names"
     was checked.I created a new folder called "C:\MINGW" and I extracted
     everything to that folder.

6) Browsing my hard drive, I see that I now have folders called "bin",
     "include", "lib", etc in the C:\MinGW folder.  That's good.  That's
      what I wanted.

7) I now edit my c:\autoexec.bat file and I add C:\MINGW\BIN to my
     path.  I reboot Windows to activate my new path.

8) Now, to verify that MinGW was installed properly, I go to an MS-DOS
     prompt and type "gcc -v" to check the compiler.  Here's what that
     looked like for me:

     C:\WINDOWS>gcc -v
     Reading specs from C:/MINGW/BIN/../lib/gcc-lib/mingw32/2.95.3-6/specs
     gcc version 2.95.3-6 (mingw special)

9) Good! It worked...  More information on installing MinGW can be found
     at their website.  If you're having problems with this part, that
     might be the best place to go.


GETTING AND INSTALLING OPENSSL:
----------------------------------

You only need to do this part if you need to run TN5250 over SSL.
SSL is an encryption mechanism for network communications that
protects your data from being observed by 3rd parties.

Encryption is not legal everywhere in the world, and there are many
different laws.  I am not a lawyer, and I will not give you legal
advice regarding these laws.  Please make sure you follow the laws
that are applicable to your area of the world.

If you wish to use it, and are able, here are the instructions:

    1) Point your web browser at:
          http://www.openssl.org

    2) Click on "Source".  It will list the various versions of OpenSSL
       that are available.  Take the latest version and download it to
       your hard drive.

       Note:  For some versions of OpenSSL there is an "Engine" version
              and a "regular" version.  Unless you have a special
              cryptographic processor that you know is supported by
              OpenSSL, you'll want the "regular" version.

    3) Once again, I use WinZip to unpack the tarball.  Again, I
        select "All files" and make sure that "Use folder names" is
        checked.   This time, however, you can extract the contents to
        the c:\ directory of your hard drive.  OpenSSL will automatically
        put everything in it's own directory.

    4) Open up an MS-DOS prompt, and switch to the directory that OpenSSL
        was extracted to.  On my system, I type:
         cd c:\openssl-0.9.7f

    5) In this directory there is a file called "INSTALL.W32" which
        contains some instructions for compiling OpenSSL on Win32 systems.
        This file tells us that we need Perl for Win32 to build OpenSSL
        and that we can get from:
            http://www.activestate.com/ActivePerl

        So, I go to that link and I get myself a copy of Perl, and I install
        it...  installing Perl is quite self-explanitory, so I won't
        bug you with the details.

    6) Now, I edit my C:\autoexec.bat again, and I add the C:\Perl\bin
        directory to my path.  My path now looks like this:
               path=%path%;C:\MINGW\BIN;C:\PERL\BIN

        Once again, I re-boot my computer to activate the changes to my
        path.

    7) Open up an MS-DOS prompt and go back to the openssl directory:
         cd \openssl-0.9.7f

    8) Once again looking at the INSTALL.W32 file, it tells me that
         "Mingw32" is a supported compiler for OpenSSL.  How fortunate!
         And unlike some of the other compilers mentioned, Mingw32 comes
         with it's own assembler, so I don't need to download NASM or
         MASM.

    9) OpenSSL takes a while to build.  To start the process, I type the
         following MS-DOS command from the C:\openssl-0.9.7f directory:
            ms\Mingw32

         Note: If you have Cygwin installed on your system, you should
               make sure it is not in your path.  I've found that having
               Cygwin in my path will cause the GNU make utility to act
               try to use sh.exe as my shell instead of the typical
               Windows command.com or cmd.exe.  This causes strange
               results.

       While you're waiting... go get a can of Coke, or a sandwich or
       something.  Relax.  

   10) Once OpenSSL has finished building, you can have it do it's own
        self-test by typing:
             cd out
             ..\ms\test

       When the tests complete, the last line of text should say:
       "passed all tests"

   11) I then installed OpenSSL on my system by typing:
             cd c:\openssl-0.9.7f
             md C:\openssl
             md C:\openssl\bin
             md c:\openssl\lib
             md c:\openssl\include
             md c:\openssl\include\openssl
             xcopy /e .\outinc\* c:\openssl\include
             copy /b out\*.a C:\openssl\lib
             copy /b out\openssl.exe c:\openssl\bin
             copy /b *.dll c:\openssl\bin

       If you choose to install things differently, make sure
       you keep track of the directories where you put the
       include files, and the directories where you put the lib
       files.  You'll need this when you configure TN5250.

   12) Now you should have the following files in their respective
       directories:
              c:\openssl\bin\openssl.exe
              c:\openssl\lib\libssl.a
              c:\openssl\lib\libcrypto.a
              c:\openssl\include\openssl\ssl.h

   13) Once again, edit your C:\autoexec.bat and add in the path
        for OpenSSL.   Add the path C:\openssl\bin  this made my
        path look like this:

 path=%path%;C:\MINGW\BIN;C:\PERL\BIN;C:\OPENSSL\BIN

   14) Re-boot your computer to activate the new path.

   15) After you've finished building TN5250 (the next step below)
        check out the README.ssl file that came with TN5250.  It
        explains how to use OpenSSL in TN5250.


BUILDING TN5250:
------------------------
We've finally finished installing all of the dependencies, and it's
time to finally build the 5250 emulator itself.

    1) If you aren't already there, open up an MS-DOS prompt and switch
         to the directory where you found this README.txt file.  It
         should be the Win32 subdirectory in the TN5250 package.

    2) From that directory, type:
          .\configure

    3) It will ask if you want OpenSSL support.  Press the "y" key or
          the "n" key, as appropriate, followed by ENTER.

    4) If you're using OpenSSL, it will now ask you for the directory
          where the OpenSSL libraries are.   This is the directory where
          the libssl.a and libcrypto.a files are stored.  If you installed
          the OpenSSL objects to the same directories that I did, you can
          type:
                  c:\openssl\lib

          and press ENTER.

    5) If you're using OpenSSL, it will now ask you where the OpenSSL
          header files are.  Again, if you did it the same way that
          I did (in step 11, above) you can simply type:
                   c:\openssl\include

          and press ENTER.

    6) It will ask you where you'd like to put the compiled programs
          when you get to the install part of this.  You should answer
          this question with the name of a directory that already
          exists that you'd like to install TN5250 into.   Personally,
          I created a C:\tn5250 dir, and used that.  

    7) To build TN5250, type:
          make

    8) To install the programs to the directory that you typed two
         steps ago, type:
           make install


RUNNING TN5250:
------------------------

    1) Open an MS-DOS prompt and switch to the directory where you just
         installed TN5250 to (if you followed what I was doing, you'll
         type "cd \tn5250").

    2) The UN*X versions of tn5250 allow you to create configuration
          files in your home directory, and in the system wide configuration
          directory.   These places don't really exist under Windows, so
          instead, I'm just checking for a configuration file in the
          same location as the executables.

    3) Create a new configuration file by typing:
          edit tn5250rc

    4) In that file, add a new configuration profile that looks like this:

          display {
             host = ssl:as400.example.com
             env.DEVNAME=DSP01
             font_80=Terminal
          }

        Only include the "ssl:" part if you want to use SSL to connect
        to your AS/400.  For a normal TN5250, it would just be i
        "host = as400.example.com"

    5) For a list of other options you can supply, type:
           tn5250 -help

    6) Try out your new session by typing (from the MS-DOS prompt):
           tn5250 display


More documentation to come later.   Good luck!


         
