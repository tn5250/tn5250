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

"""What it takes to test tn5250 from end to end.

tn5250 is run in a pseudo terminal, with a small terminal emulator at the
other end to tell what it shows, and it connects to a fake host which sends
it screens and gets what is typed on them.  See test_terminal.py.
"""

import fcntl
import os
import pty
import select
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import termios
import time
import unicodedata

# How long to wait for something to happen before deciding it never will.
TIMEOUT = float(os.environ.get('TN5250_TEST_TIMEOUT', '20'))

IAC, DONT, DO, WONT, WILL, SB, SE, EOR = 255, 254, 253, 252, 251, 250, 240, 239
TERMINAL_TYPE, END_OF_RECORD, TRANSMIT_BINARY, NEW_ENVIRON = 24, 25, 0, 39

# 5250 commands and orders.
ESC = 0x04
CMD_CLEAR_UNIT = 0x40
CMD_WRITE_TO_DISPLAY = 0x11
CMD_READ_MDT_FIELDS = 0x52
SBA, IC, SF = 0x11, 0x13, 0x1D
OPCODE_PUT_GET = 3

AID_ENTER = 0xF1
AID_F3 = 0x33

# What the emulator tells the host about the keys it refuses.
ERROR_ALPHA_ONLY = 0x0008
ERROR_NUMERIC_ONLY = 0x0009
ERROR_DIGITS_ONLY = 0x0010

# Field format words: the kind of data a field takes (second parameter of
# field() below), and what happens to it.
ALPHA_SHIFT, ALPHA_ONLY, NUMERIC_ONLY, DIGITS_ONLY, SIGNED_NUMERIC = (
    0x40, 0x41, 0x43, 0x45, 0x47)
MONOCASE = 0x20


def text(row, col, data, attr=0x20):
    """Orders which write EBCDIC data at row/col (which start at 1), with an
    attribute in the column before."""
    return bytes([SBA, row, col - 1, attr]) + bytes(data)


def field(row, col, length, ffw1=ALPHA_SHIFT, ffw2=0x00, attr=0x24):
    """Orders which make an input field start at row/col, its attribute
    being in the column before."""
    return bytes([SBA, row, col - 1, SF, ffw1, ffw2, attr,
                  length >> 8, length & 0xFF])


def screen(orders, cursor):
    """The data of a record which clears the screen, writes to it, puts the
    cursor somewhere and asks for the fields which get modified."""
    return (bytes([ESC, CMD_CLEAR_UNIT, ESC, CMD_WRITE_TO_DISPLAY, 0x00, 0x18])
            + bytes(orders) + bytes([IC, cursor[0], cursor[1]])
            + bytes([ESC, CMD_READ_MDT_FIELDS, 0x00, 0x00]))


class Reply:
    """What the emulator sends when a key such as Enter is pressed: the
    position of the cursor, the key, and the data of the modified fields
    by position."""

    def __init__(self, record):
        self.row, self.col, self.aid = record[0], record[1], record[2]
        self.fields = {}
        i = 3
        while i < len(record):
            if record[i] != SBA:
                raise ValueError('unexpected byte %02X at %d of %s'
                                 % (record[i], i, record.hex()))
            end = record.find(bytes([SBA]), i + 3)
            if end < 0:
                end = len(record)
            self.fields[(record[i + 1], record[i + 2])] = record[i + 3:end]
            i = end

    def __repr__(self):
        return 'Reply(cursor=%d,%d aid=%02X fields=%r)' % (
            self.row, self.col, self.aid, self.fields)


class FakeHost:
    """Just enough of a 5250 telnet server to send screens to an emulator
    and to get what it sends: the data of its records in self.records, but
    for the codes of the errors of the keyboard, which are in
    self.errors."""

    def __init__(self):
        self.listener = socket.socket()
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(('127.0.0.1', 0))
        self.listener.listen(1)
        self.port = self.listener.getsockname()[1]
        self.conn = None
        self.pending = b''
        self.terminal_type = None
        self.records = []
        self.errors = []

    def accept(self):
        self.conn, _ = self.listener.accept()
        self.conn.setblocking(False)
        self.conn.sendall(bytes([IAC, DO, NEW_ENVIRON, IAC, DO, TERMINAL_TYPE,
                                 IAC, SB, TERMINAL_TYPE, 1, IAC, SE,
                                 IAC, DO, END_OF_RECORD,
                                 IAC, WILL, END_OF_RECORD,
                                 IAC, DO, TRANSMIT_BINARY,
                                 IAC, WILL, TRANSMIT_BINARY]))

    def receive(self):
        """Read what the emulator sent, and split it in telnet commands,
        which are skipped but for the terminal type, and records."""
        try:
            data = self.conn.recv(65536)
        except (BlockingIOError, ConnectionResetError):
            return
        data = self.pending + data
        record = bytearray()
        # Whatever follows the last record is kept for the next time,
        # unless it is nothing but telnet commands.
        i = done = 0
        while i < len(data):
            if data[i] != IAC:
                record.append(data[i])
                i += 1
                continue
            if i + 1 >= len(data):
                break
            command = data[i + 1]
            if command == IAC:
                record.append(IAC)
                i += 2
                continue
            if command in (DO, DONT, WILL, WONT):
                if i + 2 >= len(data):
                    break
                i += 3
            elif command == SB:
                end = data.find(bytes([IAC, SE]), i)
                if end < 0:
                    break
                if data[i + 2:i + 4] == bytes([TERMINAL_TYPE, 0]):
                    self.terminal_type = data[i + 4:end].decode('ascii')
                i = end + 2
            elif command == EOR:
                # The first ten bytes are the header of the record, with
                # its flags: the one of the Help key tells that this is
                # about the key which was just refused.
                if record[7] & 0x01 and len(record) == 12:
                    self.errors.append(record[10] << 8 | record[11])
                else:
                    self.records.append(bytes(record[10:]))
                record = bytearray()
                i += 2
            else:
                i += 2
            if not record:
                done = i
        self.pending = data[done:]

    def send(self, data):
        """Send a record, which gets its header here."""
        length = len(data) + 10
        record = bytes([length >> 8, length & 0xFF, 0x12, 0xA0, 0x00, 0x00,
                        0x04, 0x00, 0x00, OPCODE_PUT_GET]) + bytes(data)
        self.conn.sendall(record.replace(bytes([IAC]), bytes([IAC, IAC]))
                          + bytes([IAC, EOR]))

    def close(self):
        for sock in (self.conn, self.listener):
            if sock is not None:
                sock.close()


def char_width(char):
    """How many cells of a terminal a character takes."""
    if unicodedata.category(char) in ('Mn', 'Me', 'Cf') and char != '\xad':
        return 0
    if unicodedata.east_asian_width(char) in ('W', 'F'):
        return 2
    return 1


class Screen:
    """A terminal emulator, which knows the control sequences ncurses sends
    to an xterm.  The ones it does not know are skipped and kept in
    self.unknown, as they are the first thing to look at when a screen is
    not the expected one."""

    SGR = {1: 'bold', 4: 'underline', 5: 'blink', 7: 'reverse'}

    def __init__(self, rows, cols, encoding):
        self.rows, self.cols = rows, cols
        self.encoding = encoding
        self.cells = [[' '] * cols for _ in range(rows)]
        self.attrs = [[frozenset()] * cols for _ in range(rows)]
        self.y = self.x = 0
        self.wrap_pending = False
        self.sgr = set()
        self.saved = (0, 0)
        self.top, self.bottom = 0, rows - 1
        self.last = ' '
        self.bells = 0
        self.unknown = []
        self.raw = b''
        self.undecoded = b''
        self.state = 'text'
        self.sequence = ''

    def feed(self, data):
        """Take in what was written to the terminal."""
        self.raw += data
        data = self.undecoded + data
        self.undecoded = b''
        if self.encoding == 'utf-8':
            # a character may not be all there yet
            for back in range(1, min(4, len(data)) + 1):
                byte = data[-back]
                if byte & 0xC0 == 0x80:
                    continue
                if byte >= 0xC0:
                    need = 2 if byte < 0xE0 else 3 if byte < 0xF0 else 4
                    if back < need:
                        self.undecoded = data[-back:]
                        data = data[:-back]
                break
        for char in data.decode(self.encoding, errors='replace'):
            self.char(char)

    def char(self, char):
        if self.state == 'text':
            if char == '\x1b':
                self.state = 'escape'
            elif char == '\r':
                self.x, self.wrap_pending = 0, False
            elif char == '\n':
                self.linefeed()
            elif char == '\b':
                self.x, self.wrap_pending = max(0, self.x - 1), False
            elif char == '\t':
                self.x = min(self.cols - 1, (self.x // 8 + 1) * 8)
            elif char == '\a':
                self.bells += 1
            elif ord(char) < 0x20 or 0x7F <= ord(char) < 0xA0:
                self.unknown.append('control character %r' % char)
            else:
                self.put(char)
        elif self.state == 'escape':
            self.state = 'text'
            if char == '[':
                self.state, self.sequence = 'csi', ''
            elif char == ']':
                self.state = 'osc'
            elif char in '()':
                self.state = 'charset'
            elif char == '7':
                self.saved = (self.y, self.x)
            elif char == '8':
                self.y, self.x = self.saved
            elif char == 'M':
                self.y = max(0, self.y - 1)
            elif char not in '=>':
                self.unknown.append('ESC ' + char)
        elif self.state == 'charset':
            self.state = 'text'
            if char != 'B':
                self.unknown.append('character set ' + char)
        elif self.state == 'osc':
            if char == '\a':
                self.state = 'text'
            elif char == '\x1b':
                self.state = 'osc-escape'
        elif self.state == 'osc-escape':
            self.state = 'text'
        elif self.state == 'csi':
            if '\x40' <= char <= '\x7e':
                self.state = 'text'
                self.csi(self.sequence, char)
            else:
                self.sequence += char

    def linefeed(self):
        if self.y == self.bottom:
            self.delete_line(self.top)
        elif self.y < self.rows - 1:
            self.y += 1
        self.wrap_pending = False

    def delete_line(self, y):
        del self.cells[y]
        del self.attrs[y]
        self.cells.insert(self.bottom, [' '] * self.cols)
        self.attrs.insert(self.bottom, [frozenset()] * self.cols)

    def insert_line(self, y):
        del self.cells[self.bottom]
        del self.attrs[self.bottom]
        self.cells.insert(y, [' '] * self.cols)
        self.attrs.insert(y, [frozenset()] * self.cols)

    def put(self, char):
        width = char_width(char)
        if width == 0:
            # it combines with the character before the cursor
            x = self.x if self.wrap_pending else self.x - 1
            if x >= 0:
                self.cells[self.y][x] += char
            return
        if self.wrap_pending:
            self.x = 0
            self.linefeed()
        self.cells[self.y][self.x] = char
        self.attrs[self.y][self.x] = frozenset(self.sgr)
        self.last = char
        if width == 2 and self.x + 1 < self.cols:
            self.cells[self.y][self.x + 1] = ''
            self.attrs[self.y][self.x + 1] = frozenset(self.sgr)
        self.x += width
        if self.x >= self.cols:
            self.x = self.cols - 1
            self.wrap_pending = True

    def erase(self, y, start, end):
        for x in range(start, end):
            self.cells[y][x] = ' '
            self.attrs[y][x] = frozenset()

    def csi(self, sequence, final):
        if sequence[:1] in ('?', '>') or final in 'hlt':
            return  # modes and window operations
        try:
            params = [int(p or 0) for p in sequence.split(';')]
        except ValueError:
            self.unknown.append('CSI %s %s' % (sequence, final))
            return

        def param(i, default=1):
            return params[i] if i < len(params) and params[i] else default

        if final in 'HfABCDGdJKXP@LM':
            self.wrap_pending = False
        if final in 'Hf':
            self.y = min(self.rows, param(0)) - 1
            self.x = min(self.cols, param(1)) - 1
        elif final == 'A':
            self.y = max(0, self.y - param(0))
        elif final == 'B':
            self.y = min(self.rows - 1, self.y + param(0))
        elif final == 'C':
            self.x = min(self.cols - 1, self.x + param(0))
        elif final == 'D':
            self.x = max(0, self.x - param(0))
        elif final == 'G':
            self.x = min(self.cols, param(0)) - 1
        elif final == 'd':
            self.y = min(self.rows, param(0)) - 1
        elif final == 'J':
            if params[0] == 0:
                self.erase(self.y, self.x, self.cols)
                for y in range(self.y + 1, self.rows):
                    self.erase(y, 0, self.cols)
            elif params[0] == 1:
                for y in range(self.y):
                    self.erase(y, 0, self.cols)
                self.erase(self.y, 0, self.x + 1)
            else:
                for y in range(self.rows):
                    self.erase(y, 0, self.cols)
        elif final == 'K':
            if params[0] == 0:
                self.erase(self.y, self.x, self.cols)
            elif params[0] == 1:
                self.erase(self.y, 0, self.x + 1)
            else:
                self.erase(self.y, 0, self.cols)
        elif final == 'X':
            self.erase(self.y, self.x, min(self.cols, self.x + param(0)))
        elif final == 'b':
            for _ in range(param(0)):
                self.put(self.last)
        elif final == 'P':
            for row, blank in ((self.cells[self.y], ' '),
                               (self.attrs[self.y], frozenset())):
                del row[self.x:self.x + param(0)]
                row.extend([blank] * (self.cols - len(row)))
        elif final == '@':
            for row, blank in ((self.cells[self.y], ' '),
                               (self.attrs[self.y], frozenset())):
                row[self.x:self.x] = [blank] * param(0)
                del row[self.cols:]
        elif final == 'L':
            for _ in range(param(0)):
                self.insert_line(self.y)
        elif final == 'M':
            for _ in range(param(0)):
                self.delete_line(self.y)
        elif final == 'r':
            self.top = param(0) - 1
            self.bottom = param(1, self.rows) - 1
            self.y = self.x = 0
        elif final == 'm':
            for p in params:
                if p == 0:
                    self.sgr.clear()
                elif p in self.SGR:
                    self.sgr.add(self.SGR[p])
                elif p - 20 in self.SGR or p == 22:
                    self.sgr.discard(self.SGR[1 if p == 22 else p - 20])
                elif 30 <= p <= 39:
                    self.sgr = {a for a in self.sgr if not a.startswith('fg')}
                    if p < 38:
                        self.sgr.add('fg%d' % (p - 30))
                elif not 40 <= p <= 49:
                    self.unknown.append('SGR %d' % p)
        else:
            self.unknown.append('CSI %s %s' % (sequence, final))

    def text(self, row, col=1, length=None):
        """The characters displayed on a row, from a column on (both start
        at 1)."""
        cells = self.cells[row - 1][col - 1:]
        return ''.join(cells if length is None else cells[:length])

    def cell_attrs(self, row, col):
        return self.attrs[row - 1][col - 1]

    def dump(self):
        lines = ['%2d|%s|' % (y + 1, ''.join(self.cells[y]))
                 for y in range(self.rows)]
        if self.unknown:
            lines.append('unknown sequences: %s' % self.unknown[:10])
        return '\n'.join(lines)


class Session:
    """A tn5250 which runs in a pseudo terminal and is connected to a
    FakeHost, unless it is told to do something else with `host'."""

    def __init__(self, binary, args=(), env=None, encoding='utf-8',
                 host=None, cwd=None, rows=25, cols=80):
        self.rows, self.cols = rows, cols
        self.screen = Screen(rows, cols, encoding)
        self.exit_status = None
        self.host = FakeHost() if host is None else None
        if host is None:
            host = '127.0.0.1:%d' % self.host.port
        argv = [binary] + list(args) + [host]
        environ = {name: value for name, value in os.environ.items()
                   if name not in ('LANG', 'LANGUAGE', 'LOCPATH', 'LINES',
                                   'COLUMNS') and not name.startswith('LC_')}
        environ['TERM'] = 'xterm'
        environ.update(env or {})
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            try:
                size = struct.pack('HHHH', rows, cols, 0, 0)
                fcntl.ioctl(0, termios.TIOCSWINSZ, size)
                if cwd is not None:
                    os.chdir(cwd)
                os.execve(binary, argv, environ)
            finally:
                os._exit(127)
        if self.host is not None:
            try:
                self.wait_for(self.accept, 'the connection of tn5250')
                self.wait_for(lambda: self.host.terminal_type is not None,
                              'the terminal type of tn5250')
            except AssertionError:
                self.close()
                raise

    def accept(self):
        readable, _, _ = select.select([self.host.listener], [], [], 0)
        if readable:
            self.host.accept()
        return bool(readable)

    def pump(self, timeout):
        """Take in what tn5250 writes to the terminal and to the host,
        waiting for it for a while."""
        files = []
        if self.fd is not None:
            files.append(self.fd)
        if self.host is not None and self.host.conn is not None:
            files.append(self.host.conn)
        if not files:
            time.sleep(timeout)
            return
        readable, _, _ = select.select(files, [], [], timeout)
        if self.fd in readable:
            try:
                data = os.read(self.fd, 65536)
            except OSError:
                data = b''
            if data:
                self.screen.feed(data)
            else:
                os.close(self.fd)
                self.fd = None
        if self.host is not None and self.host.conn in readable:
            self.host.receive()

    def wait_for(self, condition, what, timeout=None):
        """Wait until condition() is true, and fail if this takes too
        long."""
        deadline = time.monotonic() + (TIMEOUT if timeout is None else timeout)
        while not condition():
            if time.monotonic() > deadline:
                raise AssertionError('timed out waiting for %s\n%s\n%s' % (
                    what, self.screen.dump(), self.status()))
            self.pump(0.02)

    def status(self):
        if self.alive():
            return 'tn5250 is running'
        return 'tn5250 has ended with status %r' % self.exit_status

    # -- the screen
    def indicators(self):
        """The line below the screen of the 5250."""
        return self.screen.text(self.rows)

    def cursor(self):
        """The position of the cursor according to the indicators: row and
        column, which start at 1."""
        position = self.indicators()[72:79]
        if position[3:4] != '/' or not (position[:3] + position[4:]).isdigit():
            return None
        return int(position[4:]), int(position[:3])

    def inhibited(self):
        return self.indicators()[9:10] == 'X'

    def wait_cursor(self, row, col):
        """Wait for the cursor to be somewhere with the keyboard ready,
        which tells that what was sent or typed before has been dealt
        with."""
        self.wait_for(lambda: self.cursor() == (row, col)
                      and not self.inhibited(),
                      'the cursor to be at row %d, column %d' % (row, col))

    def show(self, orders, cursor):
        """Have the host send a screen, and wait for it to be displayed."""
        self.host.send(screen(orders, cursor))
        self.wait_cursor(*cursor)

    # -- the keyboard
    def type(self, data):
        """Type characters, or bytes."""
        if isinstance(data, str):
            data = data.encode(self.screen.encoding)
        while data:
            data = data[os.write(self.fd, data):]

    def reply(self, key=b'\r'):
        """Press a key which makes the emulator send the fields, Enter
        unless told otherwise, and return what the host gets."""
        del self.host.records[:]
        self.type(key)
        self.wait_for(lambda: self.host.records, 'a record from tn5250')
        return Reply(self.host.records.pop(0))

    # -- the end
    def alive(self):
        if self.exit_status is None:
            pid, status = os.waitpid(self.pid, os.WNOHANG)
            if pid:
                self.exit_status = status
        return self.exit_status is None

    def wait_exit(self, timeout=None):
        """Wait for tn5250 to end, and return what it wrote since."""
        start = len(self.screen.raw)
        self.wait_for(lambda: not self.alive(), 'the end of tn5250', timeout)
        while self.fd is not None:
            self.pump(0.05)
        return self.screen.raw[start:]

    def close(self):
        """Quit tn5250, the hard way if it takes it, and return its exit
        status as os.waitpid does."""
        if self.alive() and self.fd is not None:
            try:
                os.write(self.fd, b'\x11')  # Ctrl+Q
                self.wait_exit(5)
            except (OSError, AssertionError):
                pass
        if self.alive():
            os.kill(self.pid, signal.SIGKILL)
            _, self.exit_status = os.waitpid(self.pid, 0)
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None
        if self.host is not None:
            self.host.close()
            self.host = None
        return self.exit_status


class CharMaps:
    """The character maps of lib5250, as `charmaptest --dump' prints them:
    for each map, the Unicode character and the local character of every
    EBCDIC code, and the EBCDIC code of every local character."""

    def __init__(self, charmaptest):
        self.unicode, self.local, self.remote = {}, {}, {}
        tables = {'U': self.unicode, 'L': self.local, 'R': self.remote}
        output = subprocess.check_output([charmaptest, '--dump'])
        for line in output.decode('ascii').splitlines():
            kind, name, values = line.split(None, 2)
            tables[kind][name] = [int(value, 16) for value in values.split()]
        self.names = sorted(self.unicode, key=int)

    def encode(self, name, string):
        """The EBCDIC codes of the characters of a string."""
        return bytes(self.unicode[name].index(ord(char), 0x40)
                     for char in string)

    def typable(self, name):
        """The EBCDIC codes a keyboard can enter, and their characters."""
        table = self.unicode[name]
        codes = [code for code in range(0x40, 0xFF) if table[code] != 0xFFFD]
        return bytes(codes), ''.join(chr(table[code]) for code in codes)


def locale_codeset(name, locpath=None):
    """The character encoding of a locale, or None if there is no such
    locale."""
    env = dict(os.environ, LC_ALL=name)
    env.pop('LOCPATH', None)
    if locpath is not None:
        env['LOCPATH'] = locpath
    program = ('import locale; locale.setlocale(locale.LC_ALL, ""); '
               'print(locale.nl_langinfo(locale.CODESET))')
    try:
        output = subprocess.check_output([sys.executable, '-c', program],
                                         env=env, stderr=subprocess.DEVNULL)
    except (subprocess.CalledProcessError, OSError):
        return None
    return output.decode('ascii', 'replace').strip().upper().replace('-', '')


def find_locale(codeset, names):
    """The environment which selects a locale with some character encoding,
    looking for it under a few names: None if there is none."""
    wanted = codeset.upper().replace('-', '')
    for name in names:
        if locale_codeset(name) == wanted:
            return {'LC_ALL': name}
    return None


def make_locale(charmap, directory):
    """Make a locale with the character encoding of a charmap, such as
    ISO-8859-1, where the C library can do so and has what it takes.
    Return the environment which selects it, or None."""
    localedef = shutil.which('localedef')
    if localedef is None:
        return None
    name = 'en_US.' + charmap
    subprocess.call([localedef, '-f', charmap, '-i', 'en_US',
                     os.path.join(directory, name)],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if locale_codeset(name, directory) != charmap.upper().replace('-', ''):
        return None
    return {'LC_ALL': name, 'LOCPATH': directory}


def has_terminfo(term):
    """Whether curses knows a terminal type.  Guess that it does when there
    is no telling."""
    infocmp = shutil.which('infocmp')
    if infocmp is None:
        return True
    return subprocess.call([infocmp, term], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL) == 0


def temporary_directory():
    return tempfile.TemporaryDirectory(prefix='tn5250-test-')
