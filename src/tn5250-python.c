/* LINUX5250
 * Copyright (C) 2001 Jay 'Eraserhead' Felice
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

staticforward PyTypeObject Tn5250_ConfigType;
staticforward PyTypeObject Tn5250_TerminalType;
staticforward PyTypeObject Tn5250_DisplayType;
staticforward PyTypeObject Tn5250_DisplayBufferType;
staticforward PyTypeObject Tn5250_FieldType;

#define Tn5250_ConfigCheck(v)	    ((v)->ob_type == &Tn5250_ConfigType)
#define Tn5250_TerminalCheck(v)	    ((v)->ob_type == &Tn5250_TerminalType)
#define Tn5250_DisplayCheck(v)	    ((v)->ob_type == &Tn5250_DisplayType)
#define Tn5250_DisplayBufferCheck(v)((v)->ob_type == &Tn5250_DisplayBufferType)
#define Tn5250_FieldCheck(v)	    ((v)->ob_type == &Tn5250_FieldType)

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
    ret = tn5250_config_get_bool (self->conf, name);
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
	return PyErr_Format(PyExc_TypeError,"Terminal.config: not a Terminal object.");
    if (PyTuple_GET_SIZE(args) != 1)
	return PyErr_Format(PyExc_TypeError,"Terminal.config: need 2 arguments");
    config_obj = PyTuple_GetItem(args, 0);
    if (config_obj == NULL || !Tn5250_ConfigCheck(config_obj))
	return PyErr_Format(PyExc_TypeError,"Terminal.config: not a Config object.");
    config = (Tn5250_Config*)config_obj;
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
tn5250_DisplayBuffer (func_self, args)
    PyObject *func_self;
    PyObject *args;
{
    Tn5250_DisplayBuffer *self;
    int cols = 80, rows = 24;
    if (!PyArg_ParseTuple(args, "|dd:DisplayBuffer", &rows, &cols))
	return NULL;
    self = PyObject_NEW(Tn5250_DisplayBuffer, &Tn5250_DisplayBufferType);
    if (self == NULL)
	return NULL;

    self->dbuffer = tn5250_dbuffer_new (rows, cols);
    return (PyObject*)self;
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
    if (self == NULL || !Tn5250_DisplayBufferCheck (self))
	return PyErr_Format(PyExc_TypeError, "not a DisplayBuffer.");
    if (!PyArg_ParseTuple(args, ":DisplayBuffer.addch"))
	return NULL;
    if (self->dbuffer != NULL)
	tn5250_dbuffer_addch (self->dbuffer, ch);
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
    { NULL, NULL }
};

static PyObject*
tn5250_DisplayBuffer_getattr (self, name)
    Tn5250_Config* self;
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

/****************************************************************************/
/* Field class                                                              */
/****************************************************************************/

/****************************************************************************/
/* PrintSession class                                                       */
/****************************************************************************/

/****************************************************************************/
/* Record class                                                             */
/****************************************************************************/

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
