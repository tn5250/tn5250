#!/usr/bin/env python3

# This file is part of TN5250.
#
# TN5250 is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1, or (at your option)
# any later version.
#
# TN5250 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA

"""Tests of tn5250 from end to end: what it shows on a terminal of the
characters a host sends, and what it sends to the host of the ones which
are typed, with a terminal which uses UTF-8 and with one which does not.

`make check' and `ctest' run this.  To run it by hand, or to run some of
the tests only, tell it where things are:

    TN5250=.../curses/tn5250 TN5250_CHARMAPTEST=.../tests/charmaptest \\
        python3 test_terminal.py [-v] [TestClass[.test_method] ...]

and also set TN5250_UTF8=no if tn5250 was built without the wide character
curses library, and TN5250_OWN_KEYS=yes if it was built without the key
handling of curses.

The exit status is 77 when nothing could be tested, which is what happens
on systems without pseudo terminals or without the xterm terminal type.
"""

import os
import sys
import unicodedata
import unittest

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import tn5250test  # noqa: E402
from tn5250test import (  # noqa: E402
    AID_ENTER, AID_F3, ALPHA_ONLY, DIGITS_ONLY, ERROR_ALPHA_ONLY,
    ERROR_DIGITS_ONLY, ERROR_NUMERIC_ONLY, MONOCASE, NUMERIC_ONLY,
    SIGNED_NUMERIC, field, text)

TN5250 = os.environ.get('TN5250', 'curses/tn5250')
CHARMAPTEST = os.environ.get('TN5250_CHARMAPTEST', 'tests/charmaptest')
HAS_UTF8 = os.environ.get('TN5250_UTF8', 'yes') != 'no'
OWN_KEYS = os.environ.get('TN5250_OWN_KEYS', 'no') == 'yes'

# The keys of an xterm.
TAB, BACKTAB, LEFT, INSERT, DELETE = b'\t', b'\x1b[Z', b'\x1bOD', b'\x1b[2~', \
    b'\x1b[3~'
F1, F2, F3 = b'\x1bOP', b'\x1bOQ', b'\x1bOR'
RESET, RECORD, PLAY = b'\x12', b'\x13', b'\x17'  # Ctrl+R, Ctrl+S, Ctrl+W

# The locales tn5250 switches to when it is told to use UTF-8 and the locale
# is not a UTF-8 one.
FORCED_UTF8_LOCALES = ['C.UTF-8', 'C.utf8', 'en_US.UTF-8', 'en_US.utf8',
                       'EN_US.UTF-8']

# These are set by setUpModule.
MAPS = None      # the character maps of lib5250
UTF8 = None      # the environment of a UTF-8 locale, if there is one
LATIN1 = None    # the same for ISO-8859-1
LATIN9 = None    # the same for ISO-8859-15
C_LOCALE = {'LC_ALL': 'C'}
TEMPORARY = None
SKIP_ALL = None  # why nothing can be tested, if so


def setUpModule():
    global MAPS, UTF8, LATIN1, LATIN9, TEMPORARY, SKIP_ALL
    for program in (TN5250, CHARMAPTEST):
        if not os.access(program, os.X_OK):
            sys.exit('%s: not found, see the top of %s' % (program, __file__))
    if not hasattr(os, 'openpty'):
        SKIP_ALL = 'this system has no pseudo terminals'
        return
    if not tn5250test.has_terminfo('xterm'):
        SKIP_ALL = 'curses does not know the xterm terminal type'
        return
    MAPS = tn5250test.CharMaps(CHARMAPTEST)
    TEMPORARY = tn5250test.temporary_directory()
    if HAS_UTF8:
        current = [os.environ.get(name, '')
                   for name in ('LC_ALL', 'LC_CTYPE', 'LANG')]
        UTF8 = tn5250test.find_locale(
            'UTF-8', FORCED_UTF8_LOCALES + [name for name in current if name])
    LATIN1 = (tn5250test.find_locale(
        'ISO-8859-1', ['en_US.ISO-8859-1', 'en_US.ISO8859-1', 'en_US',
                       'fr_FR.ISO-8859-1', 'fr_FR.ISO8859-1', 'fr_FR',
                       'de_DE.ISO-8859-1', 'de_DE.ISO8859-1', 'de_DE'])
        or tn5250test.make_locale('ISO-8859-1', TEMPORARY.name))
    LATIN9 = (tn5250test.find_locale(
        'ISO-8859-15', ['en_US.ISO-8859-15', 'en_US.ISO8859-15',
                        'fr_FR.ISO-8859-15', 'fr_FR.ISO8859-15', 'fr_FR@euro',
                        'de_DE.ISO-8859-15', 'de_DE.ISO8859-15', 'de_DE@euro'])
        or tn5250test.make_locale('ISO-8859-15', TEMPORARY.name))


def tearDownModule():
    if TEMPORARY is not None:
        TEMPORARY.cleanup()


def shown(code_point):
    """What a UTF-8 terminal is expected to show in a cell for a Unicode
    character, when it is not the character itself."""
    char = chr(code_point)
    if char == '\xad':
        return '-'  # the syllable hyphen of EBCDIC
    if unicodedata.category(char) == 'Cf':
        return ' '  # directional marks and the like
    if unicodedata.category(char) in ('Mn', 'Me'):
        return ' ' + char  # combining characters
    return char


def same_cells(cells, code_points):
    """Whether the cells of a screen show these Unicode characters.  The C
    library has a say on the ones it knows nothing about, which may be the
    case of the replacement character and of the Turkish lira sign."""
    if len(cells) != len(code_points):
        return False
    for cell, code_point in zip(cells, code_points):
        if cell != shown(code_point) and not (
                cell == '?' and code_point in (0xFFFD, 0x20BA)):
            return False
    return True


def position_after(row, col, count, cols=80):
    """Where the cursor is after typing `count' characters from row/col."""
    offset = (row - 1) * cols + (col - 1) + count
    return offset // cols + 1, offset % cols + 1


class TerminalTest(unittest.TestCase):
    """What the tests have in common."""

    env = None
    encoding = 'utf-8'

    def setUp(self):
        if SKIP_ALL:
            self.skipTest(SKIP_ALL)

    def session(self, args=(), env=None, encoding=None, **kwargs):
        """Run tn5250, which gets quit at the end of the test."""
        session = tn5250test.Session(TN5250, args,
                                     self.env if env is None else env,
                                     encoding or self.encoding, **kwargs)
        self.addCleanup(session.close)
        return session

    def finish(self, session):
        """Quit tn5250 the way a user does, and check that it took it
        well."""
        status = session.close()
        self.assertTrue(os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0,
                        'tn5250 ended with status %r' % status)

    def cells(self, session, row, col, count):
        """The cells of a screen from row/col on, going to the next rows
        as an input field does."""
        cells = []
        for row in range(row, session.rows):
            cells.extend(session.screen.cells[row - 1][col - 1:])
            col = 1
        return cells[:count]

    def assert_cells(self, session, row, col, code_points, what):
        cells = self.cells(session, row, col, len(code_points))
        if not same_cells(cells, code_points):
            self.fail('%s: %r is shown instead of %r\n%s' % (
                what, cells, [shown(c) for c in code_points],
                session.screen.dump()))

    def check_strpccmd(self, name, chars):
        """Have the host send a command which writes characters to a file,
        and check what the file is made of."""
        directory = os.path.join(TEMPORARY.name, 'strpccmd-' + self.encoding)
        os.mkdir(directory)
        command = MAPS.encode(name, "printf %%s '%s' > output" % chars)
        # an attribute which hides the rest, a few bytes which tell that
        # this is a command to run, five bytes we know nothing about, and
        # the command in 123 characters
        data = (bytes([0x80, 0xFC]) + MAPS.encode(name, 'PCO ')
                + bytes([0x40] * 5) + command.ljust(123, b'\x40'))
        session = self.session(['map=' + name, '+allow_strpccmd'],
                               cwd=directory)
        session.host.send(tn5250test.screen(text(1, 2, data, 0x27), (1, 1)))
        session.wait_for(lambda: session.host.records, 'the Enter of tn5250')
        self.assertEqual(session.host.records[0][2], AID_ENTER)
        with open(os.path.join(directory, 'output'), 'rb') as output:
            self.assertEqual(output.read(), chars.encode(self.encoding))
        self.finish(session)

    def assert_refused(self, session, key, error):
        """Type a key which the field under the cursor does not take, and
        check why the host is told it was refused."""
        del session.host.errors[:]
        session.type(key)
        session.wait_for(lambda: session.host.errors and session.inhibited(),
                         'the keyboard to refuse %r' % key)
        self.assertEqual(session.host.errors, [error])
        session.type(RESET)
        session.wait_for(lambda: not session.inhibited(), 'the reset')


class Utf8Test(TerminalTest):
    """Tests with a terminal which uses UTF-8."""

    def setUp(self):
        super().setUp()
        if not HAS_UTF8:
            self.skipTest('tn5250 was built without UTF-8 support')
        if UTF8 is None:
            self.skipTest('no UTF-8 locale was found')
        self.env = UTF8


class Utf8Characters(Utf8Test):
    def check_map(self, name):
        table = MAPS.unicode[name]
        codes, chars = MAPS.typable(name)
        # four rows with every EBCDIC code which gets displayed, and an
        # input field with room for every character
        orders = b''
        for i in range(4):
            orders += text(2 + i, 3, range(0x40 + 48 * i,
                                           min(0xFF, 0x70 + 48 * i)))
        session = self.session(['map=' + name])
        session.show(orders + field(10, 3, len(codes) + 6), (10, 3))
        for i in range(4):
            first = 0x40 + 48 * i
            self.assert_cells(session, 2 + i, 3,
                              table[first:min(0xFF, first + 48)],
                              'EBCDIC %02X and following' % first)
        for row in (1, 6, 7, 8, 9, 11, 12, 13, 24):
            self.assertEqual(session.screen.text(row).strip(), '',
                             'row %d is not blank' % row)

        session.type(chars)
        session.wait_cursor(*position_after(10, 3, len(codes)))
        self.assert_cells(session, 10, 3, [table[code] for code in codes],
                          'what was typed')
        reply = session.reply()
        self.assertEqual(reply.aid, AID_ENTER)
        self.assertEqual(reply.fields.get((10, 3)), codes)
        self.assertEqual(session.screen.bells, 0)
        self.finish(session)

    def test_every_character_of_every_map(self):
        for name in MAPS.names:
            with self.subTest(map=name):
                self.check_map(name)

    def test_issue_12(self):
        """The pound sign of Italy, which started it all."""
        session = self.session(['map=280'])
        session.show(text(5, 10, MAPS.encode('280', '£UI_SRC'))
                     + field(10, 3, 20), (10, 3))
        self.assertEqual(session.screen.text(5, 10, 7), '£UI_SRC')
        self.assertIn('£UI_SRC'.encode('utf-8'), session.screen.raw)
        self.finish(session)

    def test_codes_which_are_not_characters(self):
        """NUL, DUP, 0x1F which is shown as a block, a control code and
        0xFF."""
        session = self.session(['map=37'])
        data = [0xC1, 0x00, 0xC2, 0x1C, 0xC3, 0x1F, 0xC4, 0x0E, 0xC5, 0xFF,
                0xC6]
        session.show(text(2, 3, data) + field(10, 3, 20), (10, 3))
        self.assertEqual(session.screen.text(2, 3, 11), 'A B C D E F')
        self.assertIn('reverse', session.screen.cell_attrs(2, 8))
        self.assertNotIn('reverse', session.screen.cell_attrs(2, 7))
        self.assertNotIn('reverse', session.screen.cell_attrs(2, 9))
        self.finish(session)

    def test_attributes(self):
        """Characters which take several bytes get their attributes like
        the other ones."""
        session = self.session(['map=37'])
        data = MAPS.encode('37', 'é£')
        session.show(text(2, 3, data, 0x20) + text(3, 3, data, 0x21)
                     + text(4, 3, data, 0x24) + text(5, 3, data, 0x27)
                     + text(6, 3, data, 0x22) + field(10, 3, 20), (10, 3))
        for row in (2, 3, 4, 6):
            self.assertEqual(session.screen.text(row, 3, 2), 'é£')
        self.assertEqual(session.screen.text(5, 3, 2), '  ')  # not displayed
        for col in (3, 4):
            green = session.screen.cell_attrs(2, col)
            self.assertIn('fg2', green)
            self.assertNotIn('reverse', green)
            self.assertNotIn('underline', green)
            self.assertIn('reverse', session.screen.cell_attrs(3, col))
            self.assertIn('underline', session.screen.cell_attrs(4, col))
            white = session.screen.cell_attrs(6, col)
            self.assertIn('fg7', white)
            self.assertIn('bold', white)
        self.finish(session)

    def test_ruler(self):
        session = self.session(['map=280', '+ruler'])
        session.show(text(2, 3, MAPS.encode('280', '£é £'))
                     + field(10, 3, 20), (10, 5))
        self.assertEqual(session.screen.text(2, 3, 4), '£é £')
        self.assertIn('reverse', session.screen.cell_attrs(2, 5))
        self.assertNotIn('reverse', session.screen.cell_attrs(2, 4))
        self.assertIn('reverse', session.screen.cell_attrs(10, 40))
        self.finish(session)

    def test_underscores(self):
        session = self.session(['map=37', '+underscores'])
        session.show(text(2, 3, MAPS.encode('37', 'é £'), 0x24)
                     + field(10, 3, 20), (10, 3))
        self.assertEqual(session.screen.text(2, 3, 3), 'é_£')
        self.assertNotIn('underline', session.screen.cell_attrs(2, 3))
        self.finish(session)


class Utf8Keyboard(Utf8Test):
    def test_one_byte_at_a_time(self):
        """A character whose bytes come one by one is one character."""
        session = self.session(['map=1140'])
        session.show(field(3, 3, 20), (3, 3))
        for byte in 'a€éb'.encode('utf-8'):
            session.type(bytes([byte]))
            session.pump(0.05)
        session.wait_cursor(3, 7)
        self.assertEqual(session.screen.text(3, 3, 4), 'a€éb')
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('1140', 'a€éb'))
        self.finish(session)

    def test_characters_which_are_not_in_the_map(self):
        """They are refused, and nothing else happens."""
        session = self.session(['map=37'])
        session.show(field(3, 3, 20), (3, 3))
        session.type('a')
        session.wait_cursor(3, 4)
        self.assertEqual(session.screen.bells, 0)
        # the euro sign, a CJK character, one which is not in the basic
        # plane and the replacement character
        for count, char in enumerate('€漢\U0001F600�', 1):
            session.type(char)
            session.wait_for(lambda: session.screen.bells == count,
                             'a beep for U+%04X' % ord(char))
            self.assertFalse(session.inhibited())
        session.type('b')
        session.wait_cursor(3, 5)
        self.assertEqual(session.screen.text(3, 3, 4), 'ab  ')
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', 'ab'))
        self.finish(session)

    def test_invalid_utf8(self):
        """Bytes which are not UTF-8 are dropped, and do not get in the way
        of what follows."""
        session = self.session(['map=37'])
        session.show(field(3, 3, 30), (3, 3))
        # a byte of ISO-8859-1, a lone continuation byte, an overlong
        # form, a truncated character followed by ASCII, another one
        # followed by a character, a surrogate, and bytes which are never
        # found in UTF-8
        session.type(b'a\xe9b\x80c\xc0\xafd\xe2\x82e\xe2\xc3\xa9f'
                     b'\xed\xa0\x80g\xfe\xffh')
        session.wait_cursor(3, 12)
        self.assertEqual(session.screen.text(3, 3, 9), 'abcdeéfgh')
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', 'abcdeéfgh'))
        self.finish(session)

    def test_editing_keys(self):
        """Function and editing keys are still keys."""
        session = self.session(['map=37'])
        session.show(field(3, 3, 10) + field(4, 3, 10), (3, 3))
        session.type('é')
        session.wait_cursor(3, 4)
        session.type(TAB)
        session.wait_cursor(4, 3)
        session.type('ü')
        session.wait_cursor(4, 4)
        session.type(LEFT)
        session.wait_cursor(4, 3)
        session.type('ö')  # this replaces the u with diaeresis
        session.wait_cursor(4, 4)
        session.type(BACKTAB)  # back to the start of the field
        session.wait_cursor(4, 3)
        session.type(DELETE)
        session.type('ñç')
        session.wait_cursor(4, 5)
        session.type(LEFT + LEFT)
        session.wait_cursor(4, 3)
        session.type(INSERT)
        session.wait_for(lambda: session.indicators()[30:32] == 'IM',
                         'insert mode')
        session.type('ß')
        session.wait_cursor(4, 4)
        self.assertEqual(session.screen.text(4, 3, 4), 'ßñç ')
        reply = session.reply(F3)
        self.assertEqual(reply.aid, AID_F3)
        self.assertEqual(reply.fields.get((3, 3)), MAPS.encode('37', 'é'))
        self.assertEqual(reply.fields.get((4, 3)), MAPS.encode('37', 'ßñç'))
        self.finish(session)

    def test_escape_prefix(self):
        """Esc followed by a key is a function key, and by a character
        which is not ASCII, nothing."""
        if OWN_KEYS:
            self.skipTest('Esc is no prefix without the key handling of '
                          'curses')
        session = self.session(['map=37'])
        session.show(field(3, 3, 10), (3, 3))
        session.type('é')
        session.wait_cursor(3, 4)
        # the key has to come right after Esc
        session.type(b'\x1b' + 'é'.encode('utf-8'))
        session.wait_for(lambda: session.screen.bells == 1, 'a beep')
        session.type('à')
        session.wait_cursor(3, 5)
        self.assertEqual(session.screen.text(3, 3, 3), 'éà ')
        reply = session.reply(b'\x1b3')
        self.assertEqual(reply.aid, AID_F3)
        self.assertEqual(reply.fields.get((3, 3)), MAPS.encode('37', 'éà'))
        self.assertEqual(session.screen.bells, 1)
        self.finish(session)


class Utf8Fields(Utf8Test):
    def test_monocase(self):
        """Characters get their upper case, when the host has one."""
        cases = (('37', 'abcéàçñøæþðÿßµ1-', 'ABCÉÀÇÑØÆÞÐÿßµ1-'),
                 ('1140', 'é€', 'É€'),
                 ('870', 'ąčęłźő', 'ĄČĘŁŹŐ'),
                 ('875', 'αβγάς', 'ΑΒΓΆΣ'),
                 ('880', 'абвёя', 'АБВЁЯ'),
                 ('1026', 'ığşi', 'IĞŞI'),
                 ('424', 'אבג', 'אבג'))
        for name, typed, expected in cases:
            with self.subTest(map=name):
                session = self.session(['map=' + name])
                session.show(field(3, 3, 30, ffw2=MONOCASE), (3, 3))
                session.type(typed)
                session.wait_cursor(3, 3 + len(typed))
                self.assertEqual(session.screen.text(3, 3, len(typed)),
                                 expected)
                self.assertEqual(session.reply().fields.get((3, 3)),
                                 MAPS.encode(name, expected))
                self.finish(session)

    def test_alpha_only(self):
        """Letters of any alphabet, and , . - blank."""
        session = self.session(['map=37'])
        session.show(field(3, 3, 30, ALPHA_ONLY), (3, 3))
        session.type('aéÑ, .-ß')
        session.wait_cursor(3, 11)
        for char in '1£×':
            self.assert_refused(session, char, ERROR_ALPHA_ONLY)
        session.type('z')
        session.wait_cursor(3, 12)
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', 'aéÑ, .-ßz'))
        self.finish(session)

    def test_numeric_only(self):
        session = self.session(['map=37'])
        session.show(field(3, 3, 12, NUMERIC_ONLY), (3, 3))
        session.type('12,5. 7')
        session.wait_cursor(3, 10)
        for char in 'é²':
            self.assert_refused(session, char, ERROR_NUMERIC_ONLY)
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', '12,5. 7'))
        self.finish(session)

    def test_digits_only(self):
        session = self.session(['map=37'])
        session.show(field(3, 3, 12, DIGITS_ONLY), (3, 3))
        session.type('42')
        session.wait_cursor(3, 5)
        for char in 'é³,':
            self.assert_refused(session, char, ERROR_DIGITS_ONLY)
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', '42'))
        self.finish(session)

    def test_signed_numeric(self):
        """The - and + keys are Field- and Field+ there."""
        session = self.session(['map=37'])
        session.show(field(3, 3, 6, SIGNED_NUMERIC)
                     + field(4, 3, 6, SIGNED_NUMERIC), (3, 3))
        session.type('12')
        session.wait_cursor(3, 5)
        self.assert_refused(session, 'é', ERROR_DIGITS_ONLY)
        session.type('-')
        session.wait_cursor(4, 3)
        session.type('34+')
        session.wait_cursor(3, 3)
        reply = session.reply()
        # blanks on the left, and the sign in the zone of the last digit
        self.assertEqual(reply.fields.get((3, 3)),
                         bytes([0x40, 0x40, 0x40, 0xF1, 0xD2]))
        self.assertEqual(reply.fields.get((4, 3)),
                         bytes([0x40, 0x40, 0x40, 0xF3, 0xF4]))
        self.finish(session)


class Utf8Extras(Utf8Test):
    def test_macro(self):
        """Macros are recorded to a UTF-8 file and played from it."""
        macros = os.path.join(TEMPORARY.name, 'macros-utf8')
        keys = 'é€[x]ÿ'.encode('utf-8') + TAB + 'ça'.encode('utf-8')
        fields = {(3, 3): MAPS.encode('1140', 'é€[x]ÿ'),
                  (4, 3): MAPS.encode('1140', 'ça')}
        orders = field(3, 3, 30) + field(4, 3, 30)

        session = self.session(['map=1140', 'macros=' + macros])
        session.show(orders, (3, 3))
        session.type(RECORD + F1 + keys + RECORD)
        session.wait_cursor(4, 5)
        self.assertEqual(session.reply().fields, fields)
        self.finish(session)
        with open(macros, 'rb') as macro_file:
            self.assertIn('é€[x]ÿ[TAB]ça'.encode('utf-8'), macro_file.read())

        session = self.session(['map=1140', 'macros=' + macros])
        session.show(orders, (3, 3))
        session.type(PLAY + F1)
        session.wait_cursor(4, 5)
        self.assertEqual(session.reply().fields, fields)
        self.finish(session)

    def test_long_macro(self):
        """No character is cut in two by the end of a line of the file."""
        macros = os.path.join(TEMPORARY.name, 'macros-long')
        typed = '€é' * 70
        session = self.session(['map=1140', 'macros=' + macros])
        session.show(field(3, 3, 200), (3, 3))
        session.type(RECORD + F2 + typed.encode('utf-8') + RECORD)
        session.wait_cursor(*position_after(3, 3, len(typed)))
        self.finish(session)
        with open(macros, 'rb') as macro_file:
            lines = macro_file.read().split(b'\n')
        for line in lines:
            line.decode('utf-8')
            self.assertLessEqual(len(line), 100)
        self.assertEqual(b''.join(lines[1:]).decode('utf-8'), typed)

        session = self.session(['map=1140', 'macros=' + macros])
        session.show(field(3, 3, 200), (3, 3))
        session.type(PLAY + F2)
        session.wait_cursor(*position_after(3, 3, len(typed)))
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('1140', typed))
        self.finish(session)

    def test_macro_of_an_8bit_terminal(self):
        """A macro file which is not UTF-8 holds characters of the local
        character set of the map: ISO-8859-2 for 870."""
        macros = os.path.join(TEMPORARY.name, 'macros-latin2')
        with open(macros, 'wb') as macro_file:
            macro_file.write(b'[M01]\n' + 'ąčx'.encode('iso8859-2') + b'\n\n')
        session = self.session(['map=870', 'macros=' + macros])
        session.show(field(3, 3, 30), (3, 3))
        session.type(PLAY + F1)
        session.wait_cursor(3, 6)
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('870', 'ąčx'))
        self.finish(session)

    def test_strpccmd(self):
        """A command of the host gets its characters in UTF-8."""
        self.check_strpccmd('1140', 'é£€')

    def test_trace(self):
        """Characters make it through a trace and its replay."""
        trace = os.path.join(TEMPORARY.name, 'trace')
        session = self.session(['map=1147', 'trace=' + trace])
        session.show(text(3, 5, MAPS.encode('1147', 'Prix : 12 €'))
                     + field(10, 3, 20), (10, 3))
        session.type('é€ù')
        session.wait_cursor(10, 6)
        self.assertEqual(session.reply().fields.get((10, 3)),
                         MAPS.encode('1147', 'é€ù'))
        self.finish(session)
        if not os.path.exists(trace):
            self.skipTest('tn5250 was built without traces')
        with open(trace, errors='replace') as trace_file:
            keys = [line.split()[1] for line in trace_file
                    if line.startswith('@key ')]
        self.assertIn(str(0x40000000 | ord('€')), keys)

        # The replay waits for a key before each key of the trace, and
        # eats the keys which come too fast.
        session = self.session(['map=1147'], host='debug:' + trace)
        session.wait_for(lambda: session.indicators().startswith('5250'),
                         'the replay to start')

        def replayed():
            if session.screen.text(10, 3, 3) == 'é€ù':
                return True
            session.type(b' ')
            session.pump(0.05)
            return False

        session.wait_for(replayed, 'the keys of the trace')
        self.assertEqual(session.screen.text(3, 5, 11), 'Prix : 12 €')
        self.finish(session)


class Utf8Option(TerminalTest):
    """The utf8 option, which overrides what the locale says."""

    def setUp(self):
        super().setUp()
        if not HAS_UTF8:
            self.skipTest('tn5250 was built without UTF-8 support')

    def check_8bit(self, args, env, encoding):
        """No UTF-8 on the screen nor from the keyboard."""
        session = self.session(args, env, encoding)
        session.show(text(5, 10, MAPS.encode('280', 'A£B'))
                     + field(10, 3, 20), (10, 3))
        self.assertNotIn('£'.encode('utf-8'), session.screen.raw)
        self.assertEqual(session.screen.text(5, 10, 1), 'A')
        session.type(b'a\xe9')
        session.wait_cursor(10, 5)
        self.assertEqual(session.reply().fields.get((10, 3)),
                         bytes([0x81, MAPS.remote['280'][0xE9]]))
        self.finish(session)

    def test_minus_utf8(self):
        if UTF8 is None:
            self.skipTest('no UTF-8 locale was found')
        self.check_8bit(['map=280', '-utf8'], UTF8, 'utf-8')
        self.check_8bit(['map=280', 'utf8=0'], UTF8, 'utf-8')

    def test_no_utf8_without_a_utf8_locale(self):
        self.check_8bit(['map=280'], C_LOCALE, 'iso8859-1')

    def test_plus_utf8(self):
        """UTF-8 whatever the locale says."""
        if not any(tn5250test.locale_codeset(name) == 'UTF8'
                   for name in FORCED_UTF8_LOCALES):
            self.skipTest('none of the locales tn5250 looks for was found')
        environments = [C_LOCALE, {'LANG': 'POSIX'}, {}]
        # a locale we made hides the ones of the system
        if LATIN1 is not None and 'LOCPATH' not in LATIN1:
            environments.append(LATIN1)
        for env in environments:
            for option in ('+utf8', 'utf8=1'):
                with self.subTest(env=env, option=option):
                    session = self.session(['map=280', option], env)
                    session.show(text(5, 10, MAPS.encode('280', '£UI_SRC'))
                                 + field(10, 3, 20), (10, 3))
                    self.assertEqual(session.screen.text(5, 10, 7), '£UI_SRC')
                    session.type('é£')
                    session.wait_cursor(10, 5)
                    self.assertEqual(session.reply().fields.get((10, 3)),
                                     MAPS.encode('280', 'é£'))
                    self.finish(session)


class WithoutUtf8Support(TerminalTest):
    """What a tn5250 built without the wide character curses library does
    about UTF-8: nothing."""

    def setUp(self):
        super().setUp()
        if HAS_UTF8:
            self.skipTest('tn5250 was built with UTF-8 support')

    def test_plus_utf8(self):
        # tn5250 leaves once connected, without a word to the host
        host = tn5250test.FakeHost()
        self.addCleanup(host.close)
        session = self.session(['+utf8'], C_LOCALE,
                               host='127.0.0.1:%d' % host.port)
        output = session.wait_exit()
        self.assertIn(b'built without the wide character curses library',
                      output)
        self.assertNotIn(b'\x1b[', output)  # curses was not started
        self.assertTrue(os.WIFEXITED(session.close()))

    def test_utf8_locale(self):
        env = tn5250test.find_locale('UTF-8', FORCED_UTF8_LOCALES)
        if env is None:
            self.skipTest('no UTF-8 locale was found')
        for args in (['map=280'], ['map=280', '-utf8']):
            session = self.session(args, env)
            session.show(text(5, 10, MAPS.encode('280', 'A£B'))
                         + field(10, 3, 20), (10, 3))
            self.assertNotIn('£'.encode('utf-8'), session.screen.raw)
            session.type(b'a\xe9')
            session.wait_cursor(10, 5)
            self.assertEqual(session.reply().fields.get((10, 3)),
                             bytes([0x81, MAPS.remote['280'][0xE9]]))
            self.finish(session)


class EightBitTerminal(TerminalTest):
    """Tests with a terminal which uses ISO-8859-1: this has to go on
    working as it did before there was any UTF-8 here."""

    encoding = 'iso8859-1'

    def setUp(self):
        super().setUp()
        if LATIN1 is None:
            self.skipTest('no ISO-8859-1 locale was found, nor made')
        self.env = LATIN1

    def check_map(self, name):
        local, remote = MAPS.local[name], MAPS.remote[name]
        codes = bytes(range(0x40, 0xFF))
        keys = bytes(local[code] for code in codes)
        orders = b''
        for i in range(4):
            orders += text(2 + i, 3, range(0x40 + 48 * i,
                                           min(0xFF, 0x70 + 48 * i)))
        session = self.session(['map=' + name])
        session.show(orders + field(10, 3, len(codes) + 6), (10, 3))
        for i in range(4):
            first = 0x40 + 48 * i
            expected = bytes(local[first:min(0xFF, first + 48)])
            self.assertEqual(
                session.screen.text(2 + i, 3, len(expected)),
                expected.decode('iso8859-1'),
                'EBCDIC %02X and following' % first)
        # one byte for each character, and no more
        self.assertIn(bytes([local[0x42]]), session.screen.raw)
        self.assertNotIn(chr(local[0x42]).encode('utf-8'), session.screen.raw)

        session.type(keys)
        session.wait_cursor(*position_after(10, 3, len(codes)))
        self.assertEqual(''.join(self.cells(session, 10, 3, len(codes))),
                         keys.decode('iso8859-1'))
        self.assertEqual(session.reply().fields.get((10, 3)),
                         bytes(remote[key] for key in keys))
        self.assertEqual(bytes(remote[key] for key in keys), codes)
        self.assertEqual(session.screen.bells, 0)
        self.finish(session)

    def test_every_character(self):
        # the maps whose local character set is ISO-8859-1, and one which
        # shares the tables of another one
        for name in ('37', '273', '280', '297', '500', '1147'):
            with self.subTest(map=name):
                self.check_map(name)

    def test_monocase(self):
        session = self.session(['map=37'])
        session.show(field(3, 3, 30, ffw2=MONOCASE), (3, 3))
        session.type('abcéàç1-')
        session.wait_cursor(3, 11)
        self.assertEqual(session.screen.text(3, 3, 8), 'ABCÉÀÇ1-')
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', 'ABCÉÀÇ1-'))
        self.finish(session)

    def test_alpha_only(self):
        session = self.session(['map=37'])
        session.show(field(3, 3, 30, ALPHA_ONLY), (3, 3))
        session.type('aéÑ, .-')
        session.wait_cursor(3, 10)
        for char in '1£':
            self.assert_refused(session, char, ERROR_ALPHA_ONLY)
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', 'aéÑ, .-'))
        self.finish(session)

    def test_macro(self):
        """Macros are recorded and played as the bytes they are."""
        macros = os.path.join(TEMPORARY.name, 'macros-latin1')
        session = self.session(['map=37', 'macros=' + macros])
        session.show(field(3, 3, 30), (3, 3))
        session.type(RECORD + F1 + 'éa£'.encode('iso8859-1') + RECORD
                     + PLAY + F1)
        session.wait_cursor(3, 9)
        self.assertEqual(session.reply().fields.get((3, 3)),
                         MAPS.encode('37', 'éa£éa£'))
        self.finish(session)
        with open(macros, 'rb') as macro_file:
            self.assertIn('éa£'.encode('iso8859-1'), macro_file.read())

    def test_strpccmd(self):
        """A command of the host gets its characters in ISO-8859-1."""
        self.check_strpccmd('37', 'é£ç')

    def test_euro_sign(self):
        """ISO-8859-15 has the euro sign where the CCSIDs which have one
        send it: in place of the currency sign."""
        if LATIN9 is None:
            self.skipTest('no ISO-8859-15 locale was found, nor made')
        session = self.session(['map=1147'], LATIN9, 'iso8859-15')
        session.show(text(5, 10, [0x9F, 0xC1]) + field(10, 3, 20), (10, 3))
        self.assertEqual(session.screen.text(5, 10, 2), '€A')
        self.assertIn(b'\xa4A', session.screen.raw)
        session.type('€é')
        session.wait_cursor(10, 5)
        self.assertEqual(session.reply().fields.get((10, 3)),
                         bytes([0x9F, 0xC0]))
        self.finish(session)


def main():
    program = unittest.main(exit=False)
    result = program.result
    if not result.wasSuccessful():
        return 1
    if result.testsRun == len(result.skipped):
        return 77  # which tells Automake and CTest that nothing was tested
    return 0


if __name__ == '__main__':
    sys.exit(main())
