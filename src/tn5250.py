#!/usr/bin/python
# vi:set sts=4 sw=4 ai:
#
# tn5250.py - Python version of the tn5250 program.
#

import sys
import os
from _tn5250 import *

def syntax ():
    print "Here's where the syntax message would go."
    sys.exit(1)

class Error:
    pass

config = tn5250_config_new ()
if tn5250_config_load_default (config) == -1:
    tn5250_config_unref (config)
    sys.exit(1)

a = tn5250_argv_new (len(sys.argv))
for i in range(len(sys.argv)):
    tn5250_argv_set (a, i, sys.argv[i])
if tn5250_config_parse_argv (config, len(sys.argv), a) == -1:
    tn5250_config_unref (config)
    syntax ()
tn5250_argv_free (a)

if tn5250_config_exists (config, "help"):
    syntax ()
if tn5250_config_exists (config, "version"):
    # version not implemented here.
    syntax ()
if not tn5250_config_exists (config, "host"):
    syntax ()

try:
    stream = tn5250_stream_open (tn5250_config_get (config, "host"))
    if stream == None:
	raise Error

    if tn5250_stream_config (stream, config) == -1:
	raise Error

    display = tn5250_display_new ()
    print display
    if tn5250_display_config (display, config) == -1:
	raise Error

    term = tn5250_curses_terminal_new ()
    if tn5250_config_exists (config, "underscores"):
	tn5250_curses_terminal_use_underscores (term,
	     tn5250_config_get_bool (config, "underscores")
	     )
    if tn5250_terminal_config (term, config) == -1:
	raise Error

    try:
	tn5250_terminal_init (term)
	tn5250_display_set_terminal (display, term)

	sess = tn5250_session_new ()
	tn5250_display_set_session (display, sess)
   
	tn5250_terminal_set_conn_fd (term, 
	    tn5250_stream_socket_handle (stream))
	tn5250_session_set_stream (sess, stream)
	if tn5250_session_config (sess, config) == -1:
	    raise Error

	tn5250_session_main_loop(sess)
    finally:
	tn5250_terminal_term (term)
except Error, e:
    syntax ()
