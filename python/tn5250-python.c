/* LINUX5250
 * Copyright (C) 2001-2008 Jay 'Eraserhead' Felice
 *
 * This file is part of TN5250.
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This module provides Python language bindings for the 5250 library.
 */

#include "tn5250-private.h"
#include <Python.h>

#include <stdlib.h>
#include <errno.h>

typedef struct {
    PyObject_HEAD
    Tn5250Config *conf;
} Tn5250_Config;

typedef struct {
    PyObject_HEAD
    Tn5250Terminal *term;
} Tn5250_Terminal;

typedef struct {
    PyObject_HEAD
    Tn5250Display *display;
} Tn5250_Display;

typedef struct {
    PyObject_HEAD
    Tn5250DBuffer *dbuffer;
} Tn5250_DisplayBuffer;

typedef struct {
    PyObject_HEAD
    Tn5250Field *field;
} Tn5250_Field;

typedef struct {
    PyObject_HEAD
    Tn5250PrintSession *psess;
} Tn5250_PrintSession;

typedef struct {
    PyObject_HEAD
    Tn5250Record *rec;
} Tn5250_Record;

staticforward PyTypeObject Tn5250_ConfigType;
staticforward PyTypeObject Tn5250_TerminalType;
staticforward PyTypeObject Tn5250_DisplayType;
staticforward PyTypeObject Tn5250_DisplayBufferType;
staticforward PyTypeObject Tn5250_FieldType;
staticforward PyTypeObject Tn5250_PrintSessionType;
staticforward PyTypeObject Tn5250_RecordType;

#define Tn5250_ConfigCheck(v)	    ((v)->ob_type == &Tn5250_ConfigType)
#define Tn5250_TerminalCheck(v)	    ((v)->ob_type == &Tn5250_TerminalType)
#define Tn5250_DisplayCheck(v)	    ((v)->ob_type == &Tn5250_DisplayType)
#define Tn5250_DisplayBufferCheck(v)((v)->ob_type == &Tn5250_DisplayBufferType)
#define Tn5250_FieldCheck(v)	    ((v)->ob_type == &Tn5250_FieldType)
#define Tn5250_PrintSessionCheck(v) ((v)->ob_type == &Tn5250_PrintSessionType)
#define Tn5250_RecordCheck(v)	    ((v)->ob_type == &Tn5250_RecordType)

/****************************************************************************/
/* Config class								    */
/****************************************************************************/

static PyObject*
tn5250_Config (func_self, args)
    PyObject *func_self;
    PyObject *args;
{
    Tn5250_Config *self;
    if (!PyArg_ParseTuple(args, ":Config"))
	return NULL;
    self = PyObject_NEW(Tn5250_Config, &Tn5250_ConfigType);
    if (self == NULL)
	return NULL;

    self->conf = tn5250_config_new ();
    return (PyObject*)self;
}

static void
tn5250_Config_dealloc (self)
    Tn5250_Config *self;
{
    if (self->conf != NULL) {
	tn5250_config_unref (self->conf);
	self->conf = NULL;
    }
    /* PyObject_Del(self); */
}

static PyObject*
tn5250_Config_load (obj, args)
    PyObject *obj;
    PyObject *args;
{
    char *filename;
    Tn5250_Config* self = (Tn5250_Config*)obj;
    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError, "self is not a Config object.");
    if (!PyArg_ParseTuple(args, "s:Config.load", &filename))
	return NULL;
    if (tn5250_config_load (self->conf, filename) == -1)
	return PyErr_Format(PyExc_IOError, "%s: %s", filename,
		strerror(errno));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Config_load_default (obj, args)
    PyObject *obj;
    PyObject *args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError, "self is not a Config object.");
    if (!PyArg_ParseTuple(args, ":Config.load_default"))
	return NULL;
    if (tn5250_config_load_default (self->conf) == -1)
	return PyErr_Format(PyExc_IOError, "%s", strerror(errno));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Config_parse_argv (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    PyObject* list;
    char **argv;
    int i, argc, ret;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError, "Config.parse_argv: self is not a Config object.");
    if (PyTuple_GET_SIZE(args) != 1)
	return PyErr_Format(PyExc_TypeError, "Config.parse_argv: expected 2 arguments, got 1.");
    list = PyTuple_GetItem(args, 0);
    if (! PyList_Check(list))
	return PyErr_Format(PyExc_TypeError, "Config.parse_argv: args is not a list.");

    /* Convert to a C-style argv list. */
    argc = PyList_GET_SIZE(list);
    argv = (char**)malloc (sizeof (char*)*argc);
    if (argv == NULL)
	return PyErr_NoMemory();

    for (i = 0; i < argc; i++) {
	PyObject* str;
	char *data;
	str = PyList_GetItem (list, i);
	Py_INCREF(str);
	data = PyString_AsString (str);
	argv[i] = (char*)malloc (strlen(data)+1);

	if (argv[i] == NULL) {
	    int j;
	    for (j = 0; j < i; j++)
		free(argv[j]);
	    free(argv);
	    return PyErr_NoMemory ();
	}

	strcpy (argv[i], data);
	Py_DECREF(str);
    }

    ret = tn5250_config_parse_argv (self->conf, argc, argv);

    for (i = 0; i < argc; i++)
	free (argv[i]);
    free(argv);

    if (ret == -1)
	return PyErr_Format(PyExc_IOError, "%s", strerror(errno));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Config_get (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    char *name;
    const char *ret;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError,"Config.get: not a Config object.");
    if (!PyArg_ParseTuple(args, "s:Config.get", &name))
	return NULL;
    ret = tn5250_config_get (self->conf, name);
    if (ret == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    return PyString_FromString (ret);
}

static PyObject*
tn5250_Config_get_bool (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    char *name;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError,"Config.get_bool: not a Config object.");
    if (!PyArg_ParseTuple(args, "s:Config.get_bool", &name))
	return NULL;
    if (tn5250_config_get_bool (self->conf, name))
	return PyInt_FromLong(1);
    else
	return PyInt_FromLong(0);
}

static PyObject*
tn5250_Config_get_int (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    char *name;
    int ret;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError,"Config.get_int: not a Config object.");
    if (!PyArg_ParseTuple(args, "s:Config.get_int", &name))
	return NULL;
    ret = tn5250_config_get_int (self->conf, name);
    return PyInt_FromLong((long)ret);
}

static PyObject*
tn5250_Config_set (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    char *name, *value;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError,"Config.set: not a Config object.");
    if (!PyArg_ParseTuple(args, "ss:Config.set", &name, &value))
	return NULL;
    /* XXX: We should get rid of list values or something. */
    tn5250_config_set (self->conf, name, CONFIG_STRING, value);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Config_unset (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    char *name;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError,"Config.unset: not a Config object.");
    if (!PyArg_ParseTuple(args, "s:Config.unset", &name))
	return NULL;
    tn5250_config_unset (self->conf, name);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Config_promote (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Config* self = (Tn5250_Config*)obj;
    char *name;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return PyErr_Format(PyExc_TypeError,"Config.promote: not a Config object.");
    if (!PyArg_ParseTuple(args, "s:Config.get_bool", &name))
	return NULL;
    tn5250_config_promote (self->conf, name);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef Tn5250_ConfigMethods[] = {
    { "load",		tn5250_Config_load,		METH_VARARGS },
    { "load_default",	tn5250_Config_load_default,	METH_VARARGS },
    { "parse_argv",	tn5250_Config_parse_argv,	METH_VARARGS },
    { "get",		tn5250_Config_get,		METH_VARARGS },
    { "get_bool",	tn5250_Config_get_bool,		METH_VARARGS },
    { "get_int",	tn5250_Config_get_int,		METH_VARARGS },
    { "set",		tn5250_Config_set,		METH_VARARGS },
    { "unset",		tn5250_Config_unset,		METH_VARARGS },
    { "promote",        tn5250_Config_promote,          METH_VARARGS },
    {NULL, NULL}
};

static PyObject*
tn5250_Config_getattr (self, name)
    Tn5250_Config* self;
    char *name;
{
    return Py_FindMethod (Tn5250_ConfigMethods, (PyObject*)self, name);
}

statichere PyTypeObject Tn5250_ConfigType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "Config",				/* tp_name */
    sizeof (Tn5250_Config),			/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_Config_dealloc,	/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_Config_getattr,	/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* Terminal class 							    */
/****************************************************************************/

static PyObject*
tn5250_Terminal (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* term;
    if (!PyArg_ParseTuple(args, ":tn5250.Terminal"))
	return NULL;
    term = PyObject_NEW(Tn5250_Terminal, &Tn5250_TerminalType);
    if (term == NULL)
	return PyErr_NoMemory ();

    term->term = NULL;
    return (PyObject*)term;
}

static void
tn5250_Terminal_dealloc (self)
    Tn5250_Terminal* self;
{
    /* XXX: Okay, we might have a reference counting problem here - what if
     * the 5250 library is using this? */
    if (self->term != NULL)
	tn5250_terminal_destroy(self->term);
}

static PyObject*
tn5250_Terminal_init (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.init: not a Terminal object.");
    if (!PyArg_ParseTuple(args, ":Terminal.init"))
	return NULL;
    if (self->term != NULL)
	tn5250_terminal_init(self->term);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_term (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.term: not a Terminal object.");
    if (!PyArg_ParseTuple(args, ":Terminal.term"))
	return NULL;
    if (self->term != NULL)
	tn5250_terminal_term(self->term);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_width (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.width: not a Terminal object.");
    if (!PyArg_ParseTuple(args, ":Terminal.width"))
	return NULL;
    if (self->term != NULL)
	return PyInt_FromLong((long)tn5250_terminal_width (self->term));
    else {
	Py_INCREF(Py_None);
	return Py_None;
    }
}

static PyObject*
tn5250_Terminal_height (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.height: not a Terminal object.");
    if (!PyArg_ParseTuple(args, ":Terminal.height"))
	return NULL;
    if (self->term != NULL)
	return PyInt_FromLong((long)tn5250_terminal_height (self->term));
    else {
	Py_INCREF(Py_None);
	return Py_None;
    }
}

static PyObject*
tn5250_Terminal_flags (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.flags: not a Terminal object.");
    if (!PyArg_ParseTuple(args, ":Terminal.flags"))
	return NULL;
    if (self->term != NULL)
	return PyInt_FromLong((long)tn5250_terminal_flags (self->term));
    else {
	Py_INCREF(Py_None);
	return Py_None;
    }
}

static PyObject*
tn5250_Terminal_update (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.update: not a Terminal object.");

    /* XXX: Implement. */

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_update_indicators (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.update_indicators: not a Terminal object.");

    /* XXX: Implement. */

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_waitevent (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.waitevent: not a Terminal object.");
    /* XXX: Implement. */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_getkey (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.getkey: not a Terminal object.");
    /* XXX: Implement. */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_beep (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    if (self == NULL || !Tn5250_TerminalCheck (self))
	return PyErr_Format(PyExc_TypeError,"Terminal.beep: not a Terminal object.");
    if (self->term != NULL)
	tn5250_terminal_beep (self->term);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Terminal_config (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* self = (Tn5250_Terminal*)obj;
    Tn5250_Config* config;
    PyObject* config_obj;
    if (self == NULL || !Tn5250_TerminalCheck(obj))
	return PyErr_Format(PyExc_TypeError,"not a Terminal object.");
    if (!PyArg_ParseTuple(args, "O!:Terminal.config", &Tn5250_ConfigType, &config))
	return NULL;
    if (tn5250_terminal_config (self->term, config->conf) == -1) {
	/* XXX: Exception. */
	return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef Tn5250_TerminalMethods[] = {
    { "init",		    tn5250_Terminal_init,		METH_VARARGS },
    { "term",		    tn5250_Terminal_term,		METH_VARARGS },
    { "width",		    tn5250_Terminal_width,		METH_VARARGS },
    { "height",		    tn5250_Terminal_height,		METH_VARARGS },
    { "flags",		    tn5250_Terminal_flags,		METH_VARARGS },
    { "update",		    tn5250_Terminal_update,		METH_VARARGS },
    { "update_indicators",  tn5250_Terminal_update_indicators,	METH_VARARGS },
    { "waitevent",	    tn5250_Terminal_waitevent,		METH_VARARGS },
    { "getkey",		    tn5250_Terminal_getkey,		METH_VARARGS },
    { "beep",		    tn5250_Terminal_beep,		METH_VARARGS },
    { "config",		    tn5250_Terminal_config,		METH_VARARGS },
    { NULL, NULL}
};

static PyObject*
tn5250_Terminal_getattr (self, name)
    Tn5250_Terminal* self;
    char *name;
{
    return Py_FindMethod (Tn5250_TerminalMethods, (PyObject*)self, name);
}

statichere PyTypeObject Tn5250_TerminalType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "Terminal",				/* tp_name */
    sizeof (Tn5250_Terminal),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_Terminal_dealloc,/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_Terminal_getattr,/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* CursesTerminal class 						    */
/****************************************************************************/

#ifdef USE_CURSES
static PyObject*
tn5250_CursesTerminal (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* term = (Tn5250_Terminal*)tn5250_Terminal (obj, args);
    if (term == NULL)
	return NULL;

    term->term = tn5250_curses_terminal_new ();
    return (PyObject*)term;
}
#endif /* USE_CURSES */

/****************************************************************************/
/* DebugTerminal class (?)						    */
/****************************************************************************/

#ifndef NDEBUG
static PyObject*
tn5250_DebugTerminal (obj, args)
    PyObject* obj;
    PyObject* args;
{
    /* XXX: Not implmented. */
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

/****************************************************************************/
/* SlangTerminal class							    */
/****************************************************************************/

#ifdef USE_SLANG
static PyObject*
tn5250_SlangTerminal (obj, args)
    PyObject* obj;
    PyObject* args;
{
    Tn5250_Terminal* term = (Tn5250_Terminal*)tn5250_Terminal (obj, args);
    if (term == NULL)
	return NULL;

    term->term = tn5250_slang_terminal_new ();
    return (PyObject*)term;
}
#endif /* USE_SLANG */

/****************************************************************************/
/* GTK+ Terminal class							    */
/****************************************************************************/

/****************************************************************************/
/* DisplayBuffer class                                                      */
/****************************************************************************/

static PyObject*
Tn5250_DisplayBuffer_wrap (dbuffer)
    Tn5250DBuffer* dbuffer;
{
    Tn5250_DisplayBuffer* self;

    if (dbuffer->script_slot != NULL)
	return (PyObject*)dbuffer->script_slot;
    
    self = PyObject_NEW(Tn5250_DisplayBuffer, &Tn5250_DisplayBufferType);
    if (self == NULL) {
	// XXX: Unreference the display buffer.
	return NULL;
    }
    self->dbuffer = dbuffer;
    dbuffer->script_slot = self;
    return (PyObject*)self;
}


static PyObject*
tn5250_DisplayBuffer (func_self, args)
    PyObject *func_self;
    PyObject *args;
{
    Tn5250DBuffer* dbuf;
    int cols = 80, rows = 24;
    if (!PyArg_ParseTuple(args, "|dd:DisplayBuffer", &rows, &cols))
	return NULL;
    dbuf = tn5250_dbuffer_new (rows, cols);
    if (dbuf == NULL) {
	/* Strerror */
	return NULL;
    }
    return Tn5250_DisplayBuffer_wrap(dbuf);
}

static void
tn5250_DisplayBuffer_dealloc (self)
    Tn5250_DisplayBuffer* self;
{
    /* XXX: Okay, we might have a reference counting problem here - what if
     * the 5250 library is using this? */
    if (self->dbuffer != NULL)
	tn5250_dbuffer_destroy(self->dbuffer);
}

static PyObject*
tn5250_DisplayBuffer_copy (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    Tn5250_DisplayBuffer* newdbuf;

    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.copy"))
	return NULL;

    newdbuf = PyObject_NEW(Tn5250_DisplayBuffer, &Tn5250_DisplayBufferType);
    if (newdbuf == NULL)
	return NULL;

    newdbuf->dbuffer = tn5250_dbuffer_copy (self->dbuffer);
    if (newdbuf->dbuffer == NULL) {
	Py_DECREF(newdbuf);
	return PyErr_NoMemory ();
    }
    return (PyObject*)newdbuf;
}

static PyObject*
tn5250_DisplayBuffer_set_size (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int cols = 80, rows = 24;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "|dd:DisplayBuffer.set_size", &rows, &cols))
	return NULL;

    if (self->dbuffer != NULL)
	tn5250_dbuffer_set_size (self->dbuffer, rows, cols);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_cursor_set (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int y = 0, x = 0;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "|dd:DisplayBuffer.cursor_set", &y, &x))
	return NULL;

    if (self->dbuffer != NULL)
	tn5250_dbuffer_cursor_set (self->dbuffer, y, x);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_clear (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.clear"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_clear (self->dbuffer);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_right (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int count = 1;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "|d:DisplayBuffer.right", &count))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_right (self->dbuffer, count);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_left (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.left"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_left(self->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_up (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.up"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_up(self->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_down (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.down"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_down(self->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_goto_ic (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.goto_ic"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_goto_ic(self->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_addch (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    unsigned char ch;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "c:DisplayBuffer.addch", &ch))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_addch (self->dbuffer, ch);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_del (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int shiftcount;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "d:DisplayBuffer.del", &shiftcount))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_del (self->dbuffer, shiftcount);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_ins (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int shiftcount;
    unsigned char c;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "cd:DisplayBuffer.ins", &c, &shiftcount))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_ins (self->dbuffer, c, shiftcount);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_set_ic (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int x, y;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "dd:DisplayBuffer.set_ic", &x, &y))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_set_ic (self->dbuffer, x, y);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_roll (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int top, bot, lines;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "ddd:DisplayBuffer.roll", &top, &bot, &lines))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_roll (self->dbuffer, top, bot, lines);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_char_at (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int y, x;
    unsigned char c;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "dd:DisplayBuffer.char_at", &y, &x))
	return NULL;
    c = 0;
    if (self->dbuffer != NULL)
	c = tn5250_dbuffer_char_at (self->dbuffer, y, x);
    return Py_BuildValue("c", c);
}

static PyObject*
tn5250_DisplayBuffer_width (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int wid;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.width"))
	return NULL;
    wid = 0;
    if (self->dbuffer != NULL)
	wid = tn5250_dbuffer_width (self->dbuffer);
    return Py_BuildValue("d", wid);
}

static PyObject*
tn5250_DisplayBuffer_height (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int wid;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.height"))
	return NULL;
    wid = 0;
    if (self->dbuffer != NULL)
	wid = tn5250_dbuffer_height (self->dbuffer);
    return Py_BuildValue("d", wid);
}

static PyObject*
tn5250_DisplayBuffer_cursor_x (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int wid;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.cursor_x"))
	return NULL;
    wid = 0;
    if (self->dbuffer != NULL)
	wid = tn5250_dbuffer_cursor_x (self->dbuffer);
    return Py_BuildValue("d", wid);
}

static PyObject*
tn5250_DisplayBuffer_cursor_y (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int wid;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.cursor_y"))
	return NULL;
    wid = 0;
    if (self->dbuffer != NULL)
	wid = tn5250_dbuffer_cursor_y (self->dbuffer);
    return Py_BuildValue("d", wid);
}

static PyObject*
tn5250_DisplayBuffer_add_field (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    /* XXX: Implement. */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_clear_table (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.clear_table"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_clear_table(self->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_field_yx (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    /* XXX: Implement */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_set_header_data (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    unsigned char *ptr;
    int len;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "s#:DisplayBuffer.set_header_data", &ptr, &len))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_set_header_data (self->dbuffer, ptr, len);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_send_data_for_aid_key (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int k = 0;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, "d:DisplayBuffer.send_data_for_aid_key", &k))
	return NULL;
    if (self->dbuffer != NULL) {
	k = tn5250_dbuffer_send_data_for_aid_key (self->dbuffer, k);
    }
    return PyInt_FromLong((long)k);
}

static PyObject*
tn5250_DisplayBuffer_field_data (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.field_data"))
	return NULL;
    if (self->dbuffer != NULL) {
// XXX: Implement
//	ptr = tn5250_dbuffer_field_data (self->dbuffer);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_msg_line (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int ml;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.msg_line"))
        return NULL;
    if (self->dbuffer != NULL)
	ml = tn5250_dbuffer_msg_line(self->dbuffer);
    return PyInt_FromLong((long)ml);
}

static PyObject*
tn5250_DisplayBuffer_first_non_bypass (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.first_non_bypass"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_DisplayBuffer_field_count (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int c;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.field_count"))
        return NULL;
    if (self->dbuffer != NULL)
	c = tn5250_dbuffer_field_count (self->dbuffer);
    return PyInt_FromLong((long)c);
}

static PyObject*
tn5250_DisplayBuffer_mdt (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    int mdt;
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.mdt"))
        return NULL;
    if (self->dbuffer != NULL)
	mdt = tn5250_dbuffer_mdt(self->dbuffer);
    return PyInt_FromLong((long)mdt);
}

static PyObject*
tn5250_DisplayBuffer_set_mdt (self, args)
    Tn5250_DisplayBuffer* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.set_mdt"))
        return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_set_mdt (self->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef Tn5250_DisplayBufferMethods[] = {
    { "copy",		tn5250_DisplayBuffer_copy,	    METH_VARARGS },
    { "set_size",	tn5250_DisplayBuffer_set_size,	    METH_VARARGS },
    { "cursor_set",	tn5250_DisplayBuffer_cursor_set,    METH_VARARGS },
    { "clear",		tn5250_DisplayBuffer_clear,	    METH_VARARGS },
    { "right",		tn5250_DisplayBuffer_right,	    METH_VARARGS },
    { "left",		tn5250_DisplayBuffer_left,	    METH_VARARGS },
    { "up",		tn5250_DisplayBuffer_up,	    METH_VARARGS },
    { "down",		tn5250_DisplayBuffer_down,	    METH_VARARGS },
    { "goto_ic",	tn5250_DisplayBuffer_goto_ic,	    METH_VARARGS },
    { "addch",		tn5250_DisplayBuffer_addch,	    METH_VARARGS },
    { "del",		tn5250_DisplayBuffer_del,	    METH_VARARGS },
    { "ins",		tn5250_DisplayBuffer_ins,	    METH_VARARGS },
    { "set_ic",		tn5250_DisplayBuffer_set_ic,	    METH_VARARGS },
    { "roll",		tn5250_DisplayBuffer_roll,	    METH_VARARGS },
    { "char_at",	tn5250_DisplayBuffer_char_at,	    METH_VARARGS },
    { "width",		tn5250_DisplayBuffer_width,	    METH_VARARGS },
    { "height",		tn5250_DisplayBuffer_height,	    METH_VARARGS },
    { "cursor_x",	tn5250_DisplayBuffer_cursor_x,	    METH_VARARGS },
    { "cursor_y",	tn5250_DisplayBuffer_cursor_y,	    METH_VARARGS },
    { "add_field",	tn5250_DisplayBuffer_add_field,	    METH_VARARGS },
    { "clear_table",	tn5250_DisplayBuffer_clear_table,   METH_VARARGS },
    { "field_yx",	tn5250_DisplayBuffer_field_yx,	    METH_VARARGS },
    { "set_header_data",tn5250_DisplayBuffer_set_header_data,METH_VARARGS },
    { "send_data_for_aid_key",tn5250_DisplayBuffer_send_data_for_aid_key,METH_VARARGS },
    { "field_data",	tn5250_DisplayBuffer_field_data,    METH_VARARGS },
    { "msg_line",	tn5250_DisplayBuffer_msg_line,	    METH_VARARGS },
    { "first_non_bypass",tn5250_DisplayBuffer_first_non_bypass,METH_VARARGS },
    { "field_count",	tn5250_DisplayBuffer_field_count,   METH_VARARGS },
    { "mdt",		tn5250_DisplayBuffer_mdt,	    METH_VARARGS },
    { "set_mdt",	tn5250_DisplayBuffer_set_mdt,	    METH_VARARGS },
    { NULL, NULL }
};

static PyObject*
tn5250_DisplayBuffer_getattr (self, name)
    Tn5250_DisplayBuffer* self;
    char *name;
{
    return Py_FindMethod (Tn5250_DisplayBufferMethods, (PyObject*)self, name);
}

statichere PyTypeObject Tn5250_DisplayBufferType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "DisplayBuffer",			/* tp_name */
    sizeof (Tn5250_DisplayBuffer),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_DisplayBuffer_dealloc,/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_DisplayBuffer_getattr,/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* Display class                                                            */
/****************************************************************************/

static PyObject*
tn5250_Display (func_self, args)
    PyObject* func_self;
    PyObject* args;
{
    Tn5250_Display *self;
    int cols = 80, rows = 24;
    if (!PyArg_ParseTuple(args, ":Display"))
	return NULL;
    self = PyObject_NEW(Tn5250_Display, &Tn5250_DisplayType);
    if (self == NULL)
	return NULL;

    self->display = tn5250_display_new ();
    return (PyObject*)self;
}

static void
tn5250_Display_dealloc (self)
    Tn5250_Display* self;
{
    // XXX: Reference counting problems.
    if (self->display != NULL)
	tn5250_display_destroy (self->display);
}

static PyObject*
tn5250_Display_config (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    Tn5250_Config* config;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "O!:Display.config", &Tn5250_ConfigType, &config))
        return NULL;
    if (tn5250_display_config (self->display, config->conf) == -1) {
	/* XXX: Exception. */
	return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_session (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.set_session"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_push_dbuffer (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    Tn5250DBuffer* dbuffer;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.push_dbuffer"))
        return NULL;
    dbuffer = tn5250_display_push_dbuffer(self->display);
    if (dbuffer == NULL) {
	// XXX: Strerror message
	return NULL;
    }
    return Tn5250_DisplayBuffer_wrap(dbuffer);
}

static PyObject*
tn5250_Display_restore_dbuffer (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    Tn5250_DisplayBuffer* dbuf;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "O!:Display.restore_dbuffer", &Tn5250_DisplayBufferType, &dbuf))
        return NULL;
    if (self->display != NULL && dbuf->dbuffer != NULL)
	tn5250_display_restore_dbuffer (self->display, dbuf->dbuffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_terminal (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    Tn5250_Terminal *term;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "O!:Display.set_terminal", &Tn5250_TerminalType, &term))
        return NULL;
    if (self->display != NULL && term->term != NULL)
	tn5250_display_set_terminal (self->display, term->term);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_update (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.update"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_update (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_waitevent (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int ret;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.waitevent"))
        return NULL;
    ret = 0;
    if (self->display != NULL)
	ret = tn5250_display_waitevent (self->display);
    return PyInt_FromLong((long)ret);
}

static PyObject*
tn5250_Display_getkey (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int key;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.getkey"))
        return NULL;
    key = -1;
    if (self->display != NULL)
	key = tn5250_display_getkey (self->display);
    return PyInt_FromLong((long)key);
}

static PyObject*
tn5250_Display_field_at (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "dd:Display.field_at", &y, &x))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_current_field (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.current_field"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_next_field (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.next_field"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_prev_field (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.prev_field"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_cursor_field (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.set_cursor_field"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_cursor_home (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.set_cursor_home"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_cursor_home (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_cursor_next_field (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.set_cursor_next_field"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_cursor_next_field (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_cursor_prev_field (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.set_cursor_prev_field"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_cursor_prev_field (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_shift_right (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.shift_right"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_field_adjust (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.field_adjust"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_interactive_addch (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    unsigned char ch;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "c:Display.interactive_addch", &ch))
        return NULL;
    if (self->display != NULL)
	tn5250_display_interactive_addch (self->display, ch);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_beep (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.beep"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_beep (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_do_aidkey (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int aidcode;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "d:Display.do_aidkey", &aidcode))
        return NULL;
    if (self->display != NULL)
	tn5250_display_do_aidkey (self->display, aidcode);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_indicator_set (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int inds;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "d:Display.indicator_set", &inds))
        return NULL;
    if (self->display != NULL)
	tn5250_display_indicator_set (self->display, inds);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_indicator_clear (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int inds;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "d:Display.indicator_clear", &inds))
        return NULL;
    if (self->display != NULL)
	tn5250_display_indicator_clear (self->display, inds);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_clear_unit (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.clear_unit"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_clear_unit (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_clear_unit_alternate (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.clear_unit_alternate"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_clear_unit_alternate (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_clear_format_table (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.clear_format_table"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_clear_format_table (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_pending_insert (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "dd:Display.set_pending_insert", &y, &x))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_pending_insert (self->display, y, x);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_make_wtd_data (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    Tn5250Buffer buf;
    Tn5250_DisplayBuffer *dbuf = NULL;
    PyObject* ret;

    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "|O!:Display.make_wtd_data", &Tn5250_DisplayBufferType, &dbuf))
        return NULL;
    tn5250_buffer_init (&buf);
    
    tn5250_display_make_wtd_data (self->display, &buf, dbuf ? dbuf->dbuffer : NULL);
    ret = Py_BuildValue("s#", tn5250_buffer_data(&buf), tn5250_buffer_length(&buf));
    tn5250_buffer_free(&buf);
    return ret;
}

static PyObject*
tn5250_Display_save_msg_line (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.save_msg_line"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_save_msg_line (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_char_map (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    char *mapname;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "s:Display.set_char_map", &mapname))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_char_map (self->display, mapname);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_do_keys (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.do_keys"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_do_keys (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_do_key (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int key;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "d:Display.do_key", &key))
        return NULL;
    if (self->display != NULL)
	tn5250_display_do_key (self->display, key);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_backspace (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_backspace"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_backspace (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_up (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_up"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_up (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_down (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_down"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_down (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_left (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_left"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_left (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_right (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_right"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_right (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_field_exit (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_field_exit"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_field_exit (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_field_minus (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_field_minus"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_field_minus (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_field_plus (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_field_plus"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_field_plus (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_dup (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_dup"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_dup (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_insert (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_insert"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_insert (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_tab (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_tab"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_tab (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_backtab (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_backtab"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_backtab (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_end (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_end"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_end (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_home (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_home"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_home (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_kf_delete (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.kf_delete"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_kf_delete (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_dbuffer (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    Tn5250DBuffer *dbuf;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.dbuffer"))
        return NULL;
    dbuf = tn5250_display_dbuffer(self->display);
    return Tn5250_DisplayBuffer_wrap (dbuf);
}

static PyObject*
tn5250_Display_indicators (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int inds;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.indicators"))
        return NULL;
    inds = 0;
    if (self->display != NULL)
	inds = tn5250_display_indicators (self->display);
    return PyInt_FromLong((long)inds);
}

static PyObject*
tn5250_Display_inhibited (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int inh;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.inhibited"))
        return NULL;
    inh = 0;
    if (self->display != NULL)
	inh = tn5250_display_inhibited (self->display);
    return PyInt_FromLong((long)inh);
}

static PyObject*
tn5250_Display_inhibit (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.inhibit"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_inhibit (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_uninhibit (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.uninhibit"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_uninhibit (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_cursor_x (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int x;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.cursor_x"))
        return NULL;
    x = 0;
    if (self->display != NULL)
	x = tn5250_display_cursor_x (self->display);
    return PyInt_FromLong((long)x);
}

static PyObject*
tn5250_Display_cursor_y (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int y;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.cursor_y"))
        return NULL;
    y = 0;
    if (self->display != NULL)
	y = tn5250_display_cursor_y (self->display);
    return PyInt_FromLong ((long)y);
}

static PyObject*
tn5250_Display_set_cursor (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "dd:Display.set_cursor", &y, &x))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_cursor (self->display, y, x);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_width (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int w;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.width"))
        return NULL;
    if (self->display != NULL)
	w = tn5250_display_width (self->display);
    return PyInt_FromLong((long)w);
}

static PyObject*
tn5250_Display_height (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int h;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.height"))
        return NULL;
    if (self->display != NULL)
	h = tn5250_display_height (self->display);
    return PyInt_FromLong((long)h);
}

static PyObject*
tn5250_Display_char_at (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    unsigned char ch;
    int y, x;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "dd:Display.char_at", &y, &x))
        return NULL;
    ch = 0;
    if (self->display != NULL)
	ch = tn5250_display_char_at (self->display, y, x);
    return Py_BuildValue("c", ch);
}

static PyObject*
tn5250_Display_addch (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    unsigned char ch;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "c:Display.addch", &ch))
        return NULL;
    if (self->display != NULL)
	tn5250_display_addch (self->display, ch);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_roll (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int top, bottom, lines;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "ddd:Display.roll", &top, &bottom, &lines))
        return NULL;
    if (self->display != NULL)
	tn5250_display_roll (self->display, top, bottom, lines);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_ic (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "dd:Display.set_ic", &y, &x))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_ic (self->display, y, x);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_set_header_data (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    unsigned char *ptr;
    int len;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, "s#:Display.set_header_data", &ptr, &len))
        return NULL;
    if (self->display != NULL)
	tn5250_display_set_header_data (self->display, ptr, len);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_clear_pending_insert (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.clear_pending_insert"))
        return NULL;
    if (self->display != NULL)
	tn5250_display_clear_pending_insert (self->display);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_pending_insert (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int pins;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.pending_insert"))
        return NULL;
    pins = 0;
    if (self->display != NULL)
	pins = tn5250_display_pending_insert(self->display);
    return PyInt_FromLong((long)pins);
}

static PyObject*
tn5250_Display_field_data (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.field_data"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Display_msg_line (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    int ml;
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.msg_line"))
        return NULL;
    ml = 0;
    if (self->display != NULL)
	ml = tn5250_display_msg_line (self->display);
    return PyInt_FromLong((long)ml);
}

static PyObject*
tn5250_Display_char_map (self, args)
    Tn5250_Display* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_DisplayCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Display.");
    if (!PyArg_ParseTuple(args, ":Display.char_map"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef Tn5250_DisplayMethods[] = {
    { "config",		    tn5250_Display_config,		METH_VARARGS },
    { "set_session",	    tn5250_Display_set_session,		METH_VARARGS },
    { "push_dbuffer",	    tn5250_Display_push_dbuffer,	METH_VARARGS },
    { "restore_dbuffer",    tn5250_Display_restore_dbuffer,	METH_VARARGS },
    { "set_terminal",	    tn5250_Display_set_terminal,	METH_VARARGS },
    { "update",		    tn5250_Display_update,		METH_VARARGS },
    { "waitevent",	    tn5250_Display_waitevent,		METH_VARARGS },
    { "getkey",		    tn5250_Display_getkey,		METH_VARARGS },
    { "field_at",	    tn5250_Display_field_at,		METH_VARARGS },
    { "current_field",	    tn5250_Display_current_field,	METH_VARARGS },
    { "next_field",	    tn5250_Display_next_field,		METH_VARARGS },
    { "prev_field",	    tn5250_Display_prev_field,		METH_VARARGS },
    { "set_cursor_field",   tn5250_Display_set_cursor_field,	METH_VARARGS },
    { "set_cursor_home",    tn5250_Display_set_cursor_home,	METH_VARARGS },
    { "set_cursor_next_field",tn5250_Display_set_cursor_next_field,METH_VARARGS },
    { "set_cursor_prev_field",tn5250_Display_set_cursor_prev_field,METH_VARARGS },
    { "shift_right",	    tn5250_Display_shift_right,		METH_VARARGS },
    { "field_adjust",	    tn5250_Display_field_adjust,	METH_VARARGS },
    { "interactive_addch",  tn5250_Display_interactive_addch,	METH_VARARGS },
    { "beep",		    tn5250_Display_beep,		METH_VARARGS },
    { "do_aidkey",	    tn5250_Display_do_aidkey,		METH_VARARGS },
    { "indicator_set",	    tn5250_Display_indicator_set,	METH_VARARGS },
    { "indicator_clear",    tn5250_Display_indicator_clear,	METH_VARARGS },
    { "clear_unit",	    tn5250_Display_clear_unit,		METH_VARARGS },
    { "clear_unit_alternate",tn5250_Display_clear_unit_alternate,METH_VARARGS },
    { "clear_format_table", tn5250_Display_clear_format_table,	METH_VARARGS },
    { "set_pending_insert", tn5250_Display_set_pending_insert,	METH_VARARGS },
    { "make_wtd_data",	    tn5250_Display_make_wtd_data,	METH_VARARGS },
    { "save_msg_line",	    tn5250_Display_save_msg_line,	METH_VARARGS },
    { "set_char_map",	    tn5250_Display_set_char_map,	METH_VARARGS },
    { "do_keys",	    tn5250_Display_do_keys,		METH_VARARGS },
    { "do_key",		    tn5250_Display_do_key,		METH_VARARGS },
    { "kf_backspace",	    tn5250_Display_kf_backspace,	METH_VARARGS },
    { "kf_up",		    tn5250_Display_kf_up,		METH_VARARGS },
    { "kf_down",	    tn5250_Display_kf_down,		METH_VARARGS },
    { "kf_left",	    tn5250_Display_kf_left,		METH_VARARGS },
    { "kf_right",	    tn5250_Display_kf_right,		METH_VARARGS },
    { "kf_field_exit",	    tn5250_Display_kf_field_exit,	METH_VARARGS },
    { "kf_field_minus",	    tn5250_Display_kf_field_minus,	METH_VARARGS },
    { "kf_field_plus",	    tn5250_Display_kf_field_plus,	METH_VARARGS },
    { "kf_dup",		    tn5250_Display_kf_dup,		METH_VARARGS },
    { "kf_insert",	    tn5250_Display_kf_insert,		METH_VARARGS },
    { "kf_tab",		    tn5250_Display_kf_tab,		METH_VARARGS },
    { "kf_backtab",	    tn5250_Display_kf_backtab,		METH_VARARGS },
    { "kf_end",		    tn5250_Display_kf_end,		METH_VARARGS },
    { "kf_home",	    tn5250_Display_kf_home,		METH_VARARGS },
    { "kf_delete",	    tn5250_Display_kf_delete,		METH_VARARGS },
    { "dbuffer",	    tn5250_Display_dbuffer,		METH_VARARGS },
    { "indicators",	    tn5250_Display_indicators,		METH_VARARGS },
    { "inhibited",	    tn5250_Display_inhibited,		METH_VARARGS },
    { "inhibit",	    tn5250_Display_inhibit,		METH_VARARGS },
    { "uninhibit",	    tn5250_Display_uninhibit,		METH_VARARGS },
    { "cursor_x",	    tn5250_Display_cursor_x,		METH_VARARGS },
    { "cursor_y",	    tn5250_Display_cursor_y,		METH_VARARGS },
    { "set_cursor",	    tn5250_Display_set_cursor,		METH_VARARGS },
    { "width",		    tn5250_Display_width,		METH_VARARGS },
    { "height",		    tn5250_Display_height,		METH_VARARGS },
    { "char_at",	    tn5250_Display_char_at,		METH_VARARGS },
    { "addch",		    tn5250_Display_addch,		METH_VARARGS },
    { "roll",		    tn5250_Display_roll,		METH_VARARGS },
    { "set_ic",		    tn5250_Display_set_ic,		METH_VARARGS },
    { "set_header_data",    tn5250_Display_set_header_data,	METH_VARARGS },
    { "clear_pending_insert",tn5250_Display_clear_pending_insert,METH_VARARGS },
    { "pending_insert",	    tn5250_Display_pending_insert,	METH_VARARGS },
    { "field_data",	    tn5250_Display_field_data,		METH_VARARGS },
    { "msg_line",	    tn5250_Display_msg_line,		METH_VARARGS },
    { "char_map",	    tn5250_Display_char_map,		METH_VARARGS },
    { NULL, NULL }
};

static PyObject*
tn5250_Display_getattr (self, name)
    Tn5250_Display* self;
    char *name;
{
    return Py_FindMethod (Tn5250_DisplayMethods, (PyObject*)self, name);
}

statichere PyTypeObject Tn5250_DisplayType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "Display",			/* tp_name */
    sizeof (Tn5250_Display),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_Display_dealloc,/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_Display_getattr,/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* Field class                                                              */
/****************************************************************************/

static void
tn5250_Field_dealloc (self)
    Tn5250_Field* self;
{
    // XXX: Reference counting problems.
    if (self->field != NULL)
	tn5250_field_destroy (self->field);
}

static PyObject*
Tn5250_Field_wrap (fld)
    Tn5250Field *fld;
{
    Tn5250_Field *field;

    if (fld->script_slot)
	return (PyObject*)fld->script_slot;

    field = PyObject_NEW(Tn5250_Field, &Tn5250_FieldType);
    field->field = fld;
    fld->script_slot = field;
    return (PyObject*)field;
}

static PyObject*
tn5250_Field (func_self, args)
    PyObject* func_self;
    PyObject* args;
{
    int w;
    Tn5250Field *fld;
    if (!PyArg_ParseTuple(args, "d:Field", &w))
	return NULL;

    fld = tn5250_field_new (w);
    if (fld == NULL) {
	// XXX: Strerror
	return NULL;
    }

    return Tn5250_Field_wrap (fld);
}

static PyObject*
tn5250_Field_copy (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    // XXX: Maybe copy methods should follow python's protocol (deep/shallow
    // copies).
    Tn5250Field* fld;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.copy"))
        return NULL;
    fld = tn5250_field_copy (self->field);
    return Tn5250_Field_wrap (fld);
}

static PyObject*
tn5250_Field_hit_test (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, "dd:Field.hit_test", &y, &x))
        return NULL;
    if (tn5250_field_hit_test (self->field, y, x)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_start_pos (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int pos = 0;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.start_pos"))
        return NULL;
    if (self->field != NULL)
	pos = tn5250_field_start_pos (self->field);
    return PyInt_FromLong((long)pos);
}

static PyObject*
tn5250_Field_end_pos (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int pos = 0;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.end_pos"))
        return NULL;
    if (self->field != NULL)
	pos = tn5250_field_end_pos (self->field);
    return PyInt_FromLong((long)pos);
}

static PyObject*
tn5250_Field_end_row (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int row = 0;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.end_row"))
        return NULL;
    if (self->field != NULL)
	row = tn5250_field_end_row (self->field);
    return PyInt_FromLong((long)row);
}

static PyObject*
tn5250_Field_end_col (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int col = 0;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.end_col"))
        return NULL;
    if (self->field != NULL)
	col = tn5250_field_end_col (self->field);
    return PyInt_FromLong((long)col);
}

static PyObject*
tn5250_Field_description (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.description"))
        return NULL;
    if (self->field != NULL)
	return Py_BuildValue ("s", tn5250_field_description (self->field));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_adjust_description (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.adjust_description"))
        return NULL;
    if (self->field != NULL)
	return Py_BuildValue ("s", tn5250_field_adjust_description (self->field));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_count_left (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, "dd:Field.count_left", &y, &x))
        return NULL;
    if (self->field != NULL)
	return PyInt_FromLong((long)tn5250_field_count_left (self->field, y, x));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_count_right (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    int y, x;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, "dd:Field.count_right", &y, &x))
        return NULL;
    if (self->field != NULL)
	return PyInt_FromLong((long)tn5250_field_count_right (self->field, y, x));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_set_mdt (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.set_mdt"))
        return NULL;
    if (self->field != NULL)
	tn5250_field_set_mdt (self->field);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_valid_char (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    char ch;
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, "c:Field.valid_char", &ch))
        return NULL;
    if (self->field != NULL && tn5250_field_valid_char (self->field, ch)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_mdt (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.mdt"))
        return NULL;
    if (self->field && tn5250_field_mdt(self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_clear_mdt (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.clear_mdt"))
        return NULL;
    if (self->field != NULL)
	tn5250_field_clear_mdt (self->field);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_is_bypass (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_bypass"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_bypass(self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_dup_enable (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_dup_enable"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_dup_enable (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_auto_enter (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_auto_enter"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_auto_enter (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_fer (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_fer"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_fer (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_monocase (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_monocase"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_fer(self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_mandatory (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_mandatory"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_mandatory(self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_mand_fill_type (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.mand_fill_type"))
        return NULL;
    if (self->field != NULL)
	return PyInt_FromLong(tn5250_field_mand_fill_type (self->field));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Field_is_no_adjust (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_no_adjust"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_no_adjust (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_right_zero (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_right_zero"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_right_zero (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_right_blank (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_right_blank"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_right_zero (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_is_mand_fill (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.is_mand_fill"))
        return NULL;
    if (self->field != NULL && tn5250_field_is_mand_fill (self->field)) {
	Py_INCREF(Py_True);
	return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject*
tn5250_Field_type (self, args)
    Tn5250_Field* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_FieldCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Field.");
    if (!PyArg_ParseTuple(args, ":Field.type"))
        return NULL;
    if (self->field != NULL)
	return PyInt_FromLong(tn5250_field_type(self->field));
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef Tn5250_FieldMethods[] = {
    { "copy",	tn5250_Field_copy,	METH_VARARGS },
    { "hit_test",	tn5250_Field_hit_test,	METH_VARARGS },
    { "start_pos",	tn5250_Field_start_pos,	METH_VARARGS },
    { "end_pos",	tn5250_Field_end_pos,	METH_VARARGS },
    { "end_row",	tn5250_Field_end_row,	METH_VARARGS },
    { "end_col",	tn5250_Field_end_col,	METH_VARARGS },
    { "description",	tn5250_Field_description,	METH_VARARGS },
    { "adjust_description",	tn5250_Field_adjust_description,	METH_VARARGS },
    { "count_left",	tn5250_Field_count_left,	METH_VARARGS },
    { "count_right",	tn5250_Field_count_right,	METH_VARARGS },
    { "set_mdt",	tn5250_Field_set_mdt,	METH_VARARGS },
    { "valid_char",	tn5250_Field_valid_char,	METH_VARARGS },
    { "mdt",		tn5250_Field_mdt,	METH_VARARGS },
    { "clear_mdt",	tn5250_Field_clear_mdt,	METH_VARARGS },
    { "is_bypass",	tn5250_Field_is_bypass,	METH_VARARGS },
    { "is_dup_enable",	tn5250_Field_is_dup_enable,	METH_VARARGS },
    { "is_auto_enter",	tn5250_Field_is_auto_enter,	METH_VARARGS },
    { "is_fer",		tn5250_Field_is_fer,	METH_VARARGS },
    { "is_monocase",	tn5250_Field_is_monocase,	METH_VARARGS },
    { "is_mandatory",	tn5250_Field_is_mandatory,	METH_VARARGS },
    { "mand_fill_type",	tn5250_Field_mand_fill_type,	METH_VARARGS },
    { "is_no_adjust",	tn5250_Field_is_no_adjust,	METH_VARARGS },
    { "is_right_zero",	tn5250_Field_is_right_zero,	METH_VARARGS },
    { "is_right_blank",	tn5250_Field_is_right_blank,	METH_VARARGS },
    { "is_mand_fill",	tn5250_Field_is_mand_fill,	METH_VARARGS },
    { "type",		tn5250_Field_type,	METH_VARARGS },
    { NULL, NULL }
};

static PyObject*
tn5250_Field_getattr (self, name)
    Tn5250_Field* self;
    char *name;
{
    // XXX: set/get mdt as boolean flag.
    /* Hopefully, we aren't loosing the high bit in the case off the FFW and
     * FCW. (nah, because FFW and FCW are 16 bit, long is 32 bit...) */
    if (!strcmp (name, "FFW"))
	return PyInt_FromLong((long)self->field->FFW);
    if (!strcmp (name, "FCW"))
	return PyInt_FromLong((long)self->field->FCW);
    if (!strcmp (name, "attribute"))
	return Py_BuildValue("c", self->field->attribute);
    if (!strcmp (name, "start_row"))
	return PyInt_FromLong((long)self->field->start_row);
    if (!strcmp (name, "start_col"))
	return PyInt_FromLong((long)self->field->start_col);
    if (!strcmp (name, "length"))
	return PyInt_FromLong((long)self->field->length);
    if (!strcmp (name, "display_width"))
	return PyInt_FromLong((long)self->field->w);
    return Py_FindMethod (Tn5250_FieldMethods, (PyObject*)self, name);
}

static int
tn5250_Field_setattr_helper (ptr, siz, obj)
    void *ptr;
    int siz;
    PyObject* obj;
{
    PyObject* intobj;
    if (!PyNumber_Check(obj)) {
	PyErr_Format(PyExc_TypeError, "value must be a number.");
	return 1;
    }
    intobj = PyNumber_Int (obj);
    if (siz == sizeof(unsigned char)) {
	*((unsigned char *)ptr) = PyInt_AsLong(intobj);
    } else if (siz == sizeof (int)) {
	*((int*)ptr) = PyInt_AsLong(intobj);
    } else if (siz == sizeof (unsigned short)) {
	*((unsigned short *)ptr) = PyInt_AsLong(intobj);
    }
    Py_DECREF(intobj);
    return 0;
}

static int
tn5250_Field_setattr (self, name, value)
    Tn5250_Field* self;
    char *name;
    PyObject* value;
{
    if (!strcmp (name, "FFW"))
	return tn5250_Field_setattr_helper (&self->field->FFW, sizeof (self->field->FFW), value);
    if (!strcmp (name, "FCW"))
	return tn5250_Field_setattr_helper (&self->field->FCW, sizeof (self->field->FCW), value);
    if (!strcmp (name, "attribute"))
	return tn5250_Field_setattr_helper (&self->field->attribute, sizeof (self->field->attribute), value);
    if (!strcmp (name, "start_row"))
	return tn5250_Field_setattr_helper (&self->field->start_row, sizeof (self->field->start_row), value);
    if (!strcmp (name, "start_col"))
	return tn5250_Field_setattr_helper (&self->field->start_col, sizeof (self->field->start_col), value);
    if (!strcmp (name, "length"))
	return tn5250_Field_setattr_helper (&self->field->length, sizeof (self->field->length), value);
    if (!strcmp (name, "display_width"))
	return tn5250_Field_setattr_helper (&self->field->w, sizeof (self->field->w), value);
    return 1;
}

statichere PyTypeObject Tn5250_FieldType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "Field",			/* tp_name */
    sizeof (Tn5250_Field),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_Field_dealloc,/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_Field_getattr,/* tp_getattr */
    (setattrfunc)tn5250_Field_setattr,/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* PrintSession class                                                       */
/****************************************************************************/

static PyObject*
Tn5250_PrintSession_wrap (psess)
    Tn5250PrintSession* psess;
{
    Tn5250_PrintSession* self;

    if (psess != NULL && psess->script_slot != NULL)
	return (PyObject*)psess->script_slot;

    self = PyObject_NEW(Tn5250_PrintSession, &Tn5250_PrintSessionType);
    if (self == NULL) {
	// XXX: decref the psess
	return NULL;
    }

    self->psess = psess;
    psess->script_slot = self;
    return (PyObject*)self;
}

static PyObject*
tn5250_PrintSession (func_self, args)
    PyObject* func_self;
    PyObject* args;
{
    Tn5250PrintSession* psess;

    psess = tn5250_print_session_new ();
    if (psess == NULL) {
	// XXX: Strerror
	return NULL;
    }

    return Tn5250_PrintSession_wrap(psess);
}

static void
tn5250_PrintSession_dealloc (self)
    Tn5250_PrintSession* self;
{
    // XXX: Reference counting problems.
    if (self->psess != NULL)
	tn5250_print_session_destroy (self->psess);
}

static PyObject*
tn5250_PrintSession_set_fd (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    SOCKET_TYPE fd;
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, "d:PrintSession.set_fd", &fd))
        return NULL;
    if (self->psess != NULL)
	tn5250_print_session_set_fd (self->psess, fd);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_PrintSession_get_response_code (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, ":PrintSession.get_response_code"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_PrintSession_set_stream (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, ":PrintSession.set_stream"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_PrintSession_set_output_command (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, ":PrintSession.set_output_command"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_PrintSession_set_char_map (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, ":PrintSession.set_char_map"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_PrintSession_main_loop (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, ":PrintSession.main_loop"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_PrintSession_stream (self, args)
    Tn5250_PrintSession* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_PrintSessionCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a PrintSession.");
    if (!PyArg_ParseTuple(args, ":PrintSession.stream"))
        return NULL;
    // XXX: Implement
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef Tn5250_PrintSessionMethods[] = {
    { "set_fd",		tn5250_PrintSession_set_fd,	    METH_VARARGS },
    { "get_response_code",tn5250_PrintSession_get_response_code,METH_VARARGS },
    { "set_stream",	tn5250_PrintSession_set_stream,	    METH_VARARGS },
    { "set_output_command",tn5250_PrintSession_set_output_command,METH_VARARGS },
    { "set_char_map",	tn5250_PrintSession_set_char_map,   METH_VARARGS },
    { "main_loop",	tn5250_PrintSession_main_loop,	    METH_VARARGS },
    { "stream",		tn5250_PrintSession_stream,	    METH_VARARGS },
    { NULL, NULL }
};

static PyObject*
tn5250_PrintSession_getattr (self, name)
    Tn5250_PrintSession* self;
    char *name;
{
    return Py_FindMethod (Tn5250_PrintSessionMethods, (PyObject*)self, name);
}

statichere PyTypeObject Tn5250_PrintSessionType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "PrintSession",			/* tp_name */
    sizeof (Tn5250_PrintSession),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_PrintSession_dealloc,/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_PrintSession_getattr,/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* Record class                                                             */
/****************************************************************************/

static PyObject*
tn5250_Record (func_self, args)
    PyObject *func_self;
    PyObject *args;
{
    Tn5250_Record *self;
    if (!PyArg_ParseTuple(args, ":Record"))
	return NULL;
    self = PyObject_NEW(Tn5250_Record, &Tn5250_RecordType);
    if (self == NULL)
	return NULL;

    self->rec = tn5250_record_new ();
    return (PyObject*)self;
}

static PyObject*
tn5250_Record_get_byte (self, args)
    Tn5250_Record* self;
    PyObject *args;
{
    int b;
    if (self == NULL || !Tn5250_RecordCheck (self))
        return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.get_byte"))
        return NULL;
    b = tn5250_record_get_byte (self->rec);
    return PyInt_FromLong((long)b);
}

static PyObject*
tn5250_Record_unget_byte (self, args)
    Tn5250_Record* self;
    PyObject *args;
{
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.unget_byte"))
	return NULL;
    tn5250_record_unget_byte (self->rec);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_is_chain_end (self, args)
    Tn5250_Record* self;
    PyObject *args;
{
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.is_chain_end"))
	return NULL;
    if (tn5250_record_is_chain_end (self->rec))
	return PyInt_FromLong(1);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_skip_to_end (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.skip_to_end"))
	return NULL;
    tn5250_record_skip_to_end (self->rec);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_length (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    int l;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.length"))
	return NULL;
    l = tn5250_record_length (self->rec);
    return PyInt_FromLong(l);
}

static PyObject*
tn5250_Record_append_byte (self, args)
    Tn5250_Record *self;
    PyObject* args;
{
    int c;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, "d:Record.append_byte", &c))
	return NULL;
    tn5250_record_append_byte (self->rec, c);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_data (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    char *data;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.data"))
	return NULL;
    return PyString_FromStringAndSize (tn5250_record_data (self->rec),
				       tn5250_record_length (self->rec));
}

static PyObject*
tn5250_Record_set_cur_pos (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    int newpos;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, "d:Record.set_cur_pos", &newpos))
	return NULL;
    tn5250_record_set_cur_pos (self->rec, newpos);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_opcode (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    int op;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.opcode"))
	return NULL;
    op = tn5250_record_opcode(self->rec);
    return PyInt_FromLong((long)op);
}

static PyObject*
tn5250_Record_flow_type (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    int ft;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.flow_type"))
	return NULL;
    ft = tn5250_record_flow_type(self->rec);
    return PyInt_FromLong((long)ft);
}

static PyObject*
tn5250_Record_flags (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    int f;
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.flags"))
	return NULL;
    f = tn5250_record_flags(self->rec);
    return PyInt_FromLong((long)f);
}

static PyObject*
tn5250_Record_sys_request (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.sys_request"))
	return NULL;
    if (tn5250_record_sys_request (self->rec))
	return PyInt_FromLong(1);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_attention (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.attention"))
	return NULL;
    if (tn5250_record_attention (self->rec))
	return PyInt_FromLong(1);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
tn5250_Record_dump (self, args)
    Tn5250_Record* self;
    PyObject* args;
{
    if (self == NULL || !Tn5250_RecordCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a Record.");
    if (!PyArg_ParseTuple(args, ":Record.dump"))
	return NULL;
    tn5250_record_dump (self->rec);
    Py_INCREF(Py_None);
    return Py_None;
}

static void
tn5250_Record_dealloc (self)
    Tn5250_Record* self;
{
    // XXX: Reference counting problems.
    if (self->rec != NULL)
	tn5250_record_destroy (self->rec);
}

static PyMethodDef Tn5250_RecordMethods[] = {
    { "get_byte",	tn5250_Record_get_byte,		METH_VARARGS },
    { "unget_byte",	tn5250_Record_unget_byte,	METH_VARARGS },
    { "is_chain_end",	tn5250_Record_is_chain_end,	METH_VARARGS },
    { "skip_to_end",	tn5250_Record_skip_to_end,	METH_VARARGS },
    { "length",		tn5250_Record_length,		METH_VARARGS },
    { "append_byte",	tn5250_Record_append_byte,	METH_VARARGS },
    { "data",		tn5250_Record_data,		METH_VARARGS },
    { "set_cur_pos",	tn5250_Record_set_cur_pos,	METH_VARARGS },
    { "opcode",		tn5250_Record_opcode,		METH_VARARGS },
    { "flow_type",	tn5250_Record_flow_type,	METH_VARARGS },
    { "flags",		tn5250_Record_flags,		METH_VARARGS },
    { "sys_request",	tn5250_Record_sys_request,	METH_VARARGS },
    { "attention",	tn5250_Record_attention,	METH_VARARGS },
    { "dump",		tn5250_Record_dump,		METH_VARARGS },
    { NULL, NULL }
};

static PyObject*
tn5250_Record_getattr (self, name)
    Tn5250_Record* self;
    char *name;
{
    return Py_FindMethod (Tn5250_RecordMethods, (PyObject*)self, name);
}

statichere PyTypeObject Tn5250_RecordType = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "Record",				/* tp_name */
    sizeof (Tn5250_Record),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)tn5250_Record_dealloc,	/* tp_dealloc */
    0,					/* tp_print */
    (getattrfunc)tn5250_Record_getattr,	/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
};

/****************************************************************************/
/* Session class                                                            */
/****************************************************************************/

/****************************************************************************/
/* Stream class                                                             */
/****************************************************************************/

/****************************************************************************/
/* CharMap class                                                            */
/****************************************************************************/

/****************************************************************************/
/* WTDContext class                                                         */
/****************************************************************************/

/****************************************************************************/
/* Initialization...                                                        */
/****************************************************************************/

static PyMethodDef Tn5250Methods[] = {
    { "Config",		tn5250_Config,		METH_VARARGS },
    { "Terminal",	tn5250_Terminal,	METH_VARARGS },
#ifdef USE_CURSES
    { "CursesTerminal",	tn5250_CursesTerminal,	METH_VARARGS },
#endif /* USE_CURSES */
#ifndef NDEBUG
    { "DebugTerminal",	tn5250_DebugTerminal,	METH_VARARGS },
#endif /* NDEBUG */
#ifdef USE_SLANG
    { "SlangTerminal",	tn5250_SlangTerminal,	METH_VARARGS },
#endif /* USE_SLANG */
    { "DisplayBuffer",	tn5250_DisplayBuffer,	METH_VARARGS },
    { "Display",	tn5250_Display,		METH_VARARGS },
    { "Field",		tn5250_Field,		METH_VARARGS },
    { "PrintSession",	tn5250_PrintSession,	METH_VARARGS },
    { "Record",		tn5250_Record,		METH_VARARGS },
    { NULL, NULL }
};

void
inittn5250()
{
    PyObject *m, *d;

    Tn5250_ConfigType.ob_type = &PyType_Type;

    m = Py_InitModule("tn5250", Tn5250Methods);
}

/* vi:set sts=4 sw=4 cindent: */
