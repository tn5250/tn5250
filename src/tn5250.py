# vi:set sts=4 sw=4 ai:
#
# tn5250.py - Python tn5250 API.
#

import sys
import os
from _tn5250 import *

##############################################################################
# Utility functions

def _isnull (str):
    return str[:3] == '_0_' or str == 'NULL'

def _make_field (fld):
    # FIXME: Cache these objects.
    return Field (fld)

##############################################################################
# Error class

class Error:
    """Exception type for 5250 errors."""
    def __init__ (self, msg):
	self.msg = msg
    def __str__ (self):
	return self.msg

##############################################################################
# Config class

class Config:
    """Useful, generic configuration type.

       Stores string, boolean, integer, and list configuration values in a
       tree-like structure."""

    def __init__ (self, data = None):
	"""Create a new configuration object."""
	if data == None:
	    self.data = tn5250_config_new ()
	else:
	    self.data = data
    
    def __del__ (self):
	tn5250_config_unref (self.data)
    
    def load (self, filen):
	"""Load a configuration file."""
	if tn5250_config_load (self.data, filen) == -1:
	    raise Error, 'config load error'

    def load_default (self):
	"""Load the default configuration files.

	   Loads the system configuration file (/usr/local/etc/tn5250rc
	   usually) into the configuration object if present, then loads the
	   user's configuration file (~/.tn5250rc) into the object."""
	if tn5250_config_load_default (self.data) == -1:
	    raise Error, 'config load error'

    def parse_args (self, args):
	"""Parse an array of command-line arguments.
	
	   Parses an array of command-line arguments into the configuration
	   object."""
	argv = tn5250_argv_new (len (args))
	for i in range(len(args)):
	    tn5250_argv_set (argv, i, args[i])
	ret = tn5250_config_parse_argv (self.data, len(args), argv)
	tn5250_argv_free (argv)
	if ret == -1:
	    raise Error, 'syntax error with arguments'

    def has_key (self, name):
	"""Determine whether the specified configuration key exists."""
	return tn5250_config_exists (self.data, name)

    def get (self, name):
	"""Retreive the specified configuration key.

	   Retreives the specified configuration key (as a string), or raise
	   a KeyError if the key is not found."""
	if not tn5250_config_exists (self.data, name):
	    raise KeyError, 'key not found'
	return tn5250_config_get (self.data, name)
    def __getitem__ (self, name):
	return self.get (name)

    def get_bool (self, name):
	"""Retreive the specified configuration key as a boolean value.

	   Retreives the specified configuration key as a boolean value
	   (can be specified as on/off, true/false, or yes/no in the
	   configuration), or raise a KeyError if the key is not found."""
	if not tn5250_config_exists (self.data, name):
	    raise KeyError, 'key not found'
	return tn5250_config_get_bool (self.data, name)

    def get_int (self, name):
	"""Retreive the specified configuration key as an integer value.

	   Retrieves the specifeid configuration key as an integer value or
	   raises KeyError if the key is not found."""
	if not tn5250_config_exists (self.data, name):
	    raise KeyError, 'key not found'
	return tn5250_config_get_int (self.data, name)

    def set (self, name, val):
	"""Set the specified configuration key.

	   Sets the specified configuration key.  The `val' argument can be
	   an integer value, a string value, or a list."""
	if type(val) == type([]):
	    tn5250_config_set (self.data, name, 2, val)
	elif type(val) == type(''):
	    tn5250_config_set (self.data, name, 1, val)
	else:
	    tn5250_config_set (self.data, name, 1, str(val))
    def __setitem__ (self, name, val):
	self.set (name, val)

    def unset (self, name):
	"""Remove the specified key from the configuration object.

	   Removes the specifeid key from the configuration object.  Does NOT
	   raise a KeyError if the key does not exist."""
	tn5250_config_unset (self.data, name)
    def __delitem__ (self, name):
	self.unset (name)

    def promote (self, prefix):
	"""Promote configuration items.

	   Finds all configuration items which start with `prefix' followed by
	   a period, and removes the prefix and the period.  If a key with that
	   name already existed, it is overwritten.  This is how selecting from
	   several different configurations is implemented."""
	tn5250_config_promote (self.data, prefix)

	
##############################################################################
# Stream class

class Stream:
    # FIXME: Need an interface for creating a hosting stream.
    def __init__ (self, url):
	if url[0] == '_':
	    self.data = url
	else:
	    self.data = tn5250_stream_open (url)

    def __del__ (self):
	tn5250_stream_destroy (self.data)

    def config (self, cfg):
	if tn5250_stream_config (self.data, cfg.data) == -1:
	    raise Error, 'config error'

    def connect (self, url):
	if tn5250_stream_connect (self.data, url) == -1:
	    raise Error, 'connect error'

    def disconnect (self):
	tn5250_stream_disconnect (self.data)

    def handle_receive (self):
	tn5250_stream_handle_receive (self.data)

    def get_record (self):
	rec = tn5250_stream_get_record (self.data)
	if _isnull(rec):
	    return None
	return rec

    def send_packet (self, len, header, data):
	tn5250_stream_send (self.data, len, header, data)

    def setenv (self, name, val):
	tn5250_stream_setenv (self.data, name, value)
    def __setitem__ (self, name, value):
	self.setenv (name, value)

    def getenv (self, name):
	if not tn5250_stream_env_exists (self.data, name):
	    raise KeyError, 'stream environment not found'
	return tn5250_stream_getenv (self.data, name)
    def __getitem__ (self, name):
	return self.getenv (name)

    def unsetenv (self, name):
	tn5250_stream_unsetenv (self.data, name)
    def __delitem__ (self, name):
	self.unsetenv (name)

    def socket_handle (self):
	return tn5250_stream_socket_handle (self.data)
    def fd (self):
	return self.socket_handle ()


##############################################################################
# Session class

class Session:
    def __init__ (self, data = None):
	if data == None:
	    self.data = tn5250_session_new ()
	else:
	    self.data = data

    def __del__ (self):
	tn5250_session_destroy (self.data)

    def config (self, cfg):
	tn5250_session_config (self.data, cfg.data)

    def set_stream (self, strm):
	self._stream = strm
	tn5250_session_set_stream (self.data, strm.data)

    def get_stream (self):
	if hasattr(self,'_stream'):
	    return self._stream
	strm = Stream (self.data)
	if _isnull (strm):
	    self._stream = None
	else:
	    self._stream = Stream (strm)
	return self._stream

    def run (self):
	tn5250_session_main_loop (self.data)


##############################################################################
# Display class

class Display:
    def __init__ (self, data = None):
	if data == None:
	    self.data = tn5250_display_new ()
	else:
	    self.data = data

    def __del__ (self):
	tn5250_display_destroy (self.data)

    def config (self, cfg):
	if tn5250_display_config (self.data, cfg.data) == -1:
	    raise Error, 'error configuring display'

    def set_session (self, sess = None):
	if sess == None:
	    tn5250_display_set_session (self.data, 'NULL')
	else:
	    tn5250_display_set_session (self.data, sess.data)

    def push_dbuffer (self):
	return DisplayBuffer (tn5250_display_push_dbuffer (self.data))
    def restore_dbuffer (self, dbuf):
	return tn5250_display_restore_dbuffer (self.data, dbuf.data)

    def set_terminal (self, term):
	tn5250_display_set_terminal (self.data, term.data)

    def update (self):
	tn5250_display_update (self.data)
    def waitevent (self):
	return tn5250_display_waitevent (self.data)
    def getkey (self):
	return tn5250_display_getkey (self.data)

    def field_at (self, y, x):
	return _make_field (tn5250_display_field_at (self.data, y, x))
    def current_field (self):	
	return _make_field (tn5250_display_current_field (self.data))
    def next_field (self):
	return _make_field (tn5250_display_next_field (self.data))
    def prev_field (self):
	return _make_field (tn5250_display_prev_field (self.data))
    def set_cursor_field (self, fld):
	tn5250_display_set_cursor_field (self.data, fld.data)
    def set_cursor_home (self):
	tn5250_display_set_cursor_home (self.data)
    def set_cursor_next_field (self):
	tn5250_display_set_cursor_next_field (self.data)
    def set_cursor_prev_field (self):
	tn5250_display_set_cursor_prev_field (self.data)
    def shift_right (self, fld, fill):
	tn5250_display_shift_right (self.data, fld.data, fill)
    def field_adjust (self, fld):
	tn5250_display_field_adjust (self.data, fld.data)
    def interactive_addch (self, ch):
	tn5250_display_interactive_addch (self.data, ch)
    def beep (self):
	tn5250_display_beep (self.data)
    def do_aidkey (self, aidcode):
	tn5250_display_do_aidkey (self.data, aidcode)
    def indicator_set (self, inds):
	tn5250_display_indicator_set (self.data, inds)
    def indicator_clear (self, inds):
	tn5250_display_indicator_clear (self.data, inds)
    def clear_unit (self):
	tn5250_display_clear_unit (self.data)
    def clear_unit_alternate (self):
	tn5250_display_clear_unit_alternate (self.data)
    def clear_format_table (self):
	tn5250_display_clear_format_table (self.data)
    def set_pending_insert (self, y, x):
	tn5250_display_set_pending_insert (self.data, y, x)
    def make_wtd_data (self, dbuf):
	buf = tn5250_buffer_new ()
	tn5250_display_make_wtd_data (self.data, buf, dbuf.data)
	ret = tn5250_buffer_2string (buf)
	tn5250_buffer_destroy (buf)
	return ret
    def save_msg_line (self):
	tn5250_display_save_msg_line (self.data)
    def set_char_map (self, name):
	tn5250_display_set_char_map (self.data, name)
    def do_keys (self):
	tn5250_display_do_keys (self.data)
    def do_key (self, k):
	tn5250_display_do_key (self.data, k)
    def kf_backspace (self):
	tn5250_display_kf_backspace (self.data)
    def kf_up (self):
	tn5250_display_kf_up (self.data)
    def kf_down (self):
	tn5250_display_kf_down (self.data)
    def kf_left (self):
	tn5250_display_kf_left (self.data)
    def kf_right (self):
	tn5250_display_kf_right (self.data)
    def kf_field_exit (self):
	tn5250_display_kf_field_exit (self.data)
    def kf_field_minus (self):
	tn5250_display_kf_field_minus (self.data)
    def kf_field_plus (self):
	tn5250_display_kf_field_plus (self.data)
    def kf_dup (self):
	tn5250_display_kf_dup (self.data)
    def kf_insert (self):
	tn5250_display_kf_insert (self.data)
    def kf_tab (self):
	tn5250_display_kf_tab (self.data)
    def kf_backtab (self):
	tn5250_display_kf_backtab (self.data)
    def kf_end (self):
	tn5250_display_kf_end (self.data)
    def kf_home (self):
	tn5250_display_kf_home (self.data)
    def kf_delete (self):
	tn5250_display_kf_delete (self.data)
 

##############################################################################
# DisplayBuffer class

class DisplayBuffer:
    def __init__ (self, wid, ht = None):
	"""Create a new display buffer."""
	if ht == None:
	    self.data = wid
	else:
	    self.data = tn52520_dbuffer_new (wid, ht)

    def __del__ (self):
	if self.data != None:
	    tn5250_dbuffer_destroy (self.data)

    def copy (self):
	"""Return a copy of this display buffer."""
	return DisplayBuffer (tn5250_dbuffer_copy (self.data))

    def set_size (self, wid, ht):
	"""Resize the display buffer."""
	tn5250_dbuffer_set_size (self.data, wid, ht)

    def cursor_set (self, y, x):
	"""Set the positiion of the cursor within this display buffer."""
	tn5250_dbuffer_cursor_set (self.data, y, x)

    def clear (self):
	"""Clear the display buffer."""
	tn5250_dbuffer_clear (self.data)
	
    def right (self, n = 1):
	"""Move the cursor right `n' places.

	   Move the cursor right `n' places, wrapping to the next line if
	   we reach the end of the line or to the first position of the buffer
	   if we start at the end of the buffer."""
	tn5250_dbuffer_right (self.data)

    def left (self):
	"""Move the cursor left one position.

	   Move the cursor to the left one position, wrapping as necessary."""
	tn5250_dbuffer_left (self.data)

    def up (self):
	"""Move the cursor up one position, wrapping if necessary."""
	tn5250_dbuffer_up (self.data)

    def down (self):
	"""Move the cursor down one position, wrapping if necessary."""
	tn5250_dbuffer_down (self.data)

    def goto_ic (self):
	"""Move the cursor to the insert cursor position."""
	tn5250_dbuffer_goto_ic (self.data)

    def addch (self, ch):
	"""Add a character to the display buffer.

	   Add an EBCDIC character to the display buffer and move the cursor
	   as appropriate.  This does NOT handle cursor movement keys and 
	   such, see Display.interactive_addch (ch) for that."""
	tn5250_dbuffer_addch (self.data, ch)

    def del (self, shiftcount):
	"""Delete a character at the cursor.

	   Delete the character under the cursor by shifting `shiftcount'
	   characters to the left one place and inserting an EBCDIC NUL in
	   the created vacuum."""
	tn5250_dbuffer_del (self.data, shiftcount)

    def ins (self, c, shiftcount):
	"""Insert a character at the cursor.

	   Insert a character at the cursor, moving `shiftcount' characters
	   at the right of the cursor one position right."""
	tn5250_dbuffer_ins (self.data, c, shiftcount)

    def set_ic (self, y, x):
	"""Set the insert cursor position."""
	tn5250_dbuffer_set_ic (self.data, y, x)

    def roll (self, top, bot, lines):
	"""Roll some lines on the display up or down."""
	tn5250_dbuffer_roll (self.data, top, bot, lines)

    def char_at (self, y, x):
	"""Retreive the EBCDIC character at (x, y)."""
	return tn5250_dbuffer_char_at (self.data, y, x)


##############################################################################
# Terminal class

class Terminal:
    def __init__ (self, data = None):
	self.data = data
    def __del__ (self):
	if self.data != None:
	    tn5250_terminal_destroy (self.data)

    def init (self):
	tn5250_terminal_init (self.data)
    def term (self):
	tn5250_terminal_term (self.data)
    def width (self):
	return tn5250_terminal_width (self.data)
    def height (self):
	return tn5250_terminal_height (self.data)
    def flags (self):
	return tn5250_terminal_flags (self.data)
    def update (self, disp):
	tn5250_terminal_update (self.data, disp.data)
    def update_indicators (self, disp):
	tn5250_terminal_update_indicators (self.data, disp.data)
    def waitevent (self):
	return tn5250_terminal_waitevent (self.data)
    def getkey (self):
	return tn5250_terminal_getkey (self.data)
    def beep (self):
	tn5250_terminal_beep (self.data)
    def config (self, cfg):
	if tn5250_terminal_config (self.data, cfg.data) == -1:
	    raise Error, 'terminal config error'

##############################################################################
# CursesTerminal class

class CursesTerminal (Terminal):
    def __init__ (self, data = None):
	if data == None:
	    self.data = tn5250_curses_terminal_new ()
	else:
	    self.data = data

    def use_underscores (self, flag):
	tn5250_curses_terminal_use_underscores (self.data, flag)


##############################################################################
# SlangTerminal class

class SlangTerminal (Terminal):
    def __init__ (self, data = None):
	if data == None:
	    self.data = tn5250_slang_terminal_new ()
	else:
	    self.data = data

    def use_underscores (self, flag):
	tn5250_slang_terminal_use_underscores (self.data, flag)
