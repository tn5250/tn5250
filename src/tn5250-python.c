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

/****************************************************************************/
/* Config class								    */
/****************************************************************************/

typedef struct {
    PyObject_HEAD
    Tn5250Config *conf;
} Tn5250_Config;

staticforward PyTypeObject Tn5250_ConfigType;

#define Tn5250_ConfigCheck(v)	    ((v)->ob_type == &Tn5250_ConfigType)

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
	return NULL;
    if (!PyArg_ParseTuple(args, "s:Config.load", &filename))
	return NULL;
    if (tn5250_config_load (self->conf, filename) == -1)
	return NULL; /* XXX: Throw an IOError. */
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
	return NULL;
    if (!PyArg_ParseTuple(args, ":Config.load_default"))
	return NULL;
    if (tn5250_config_load_default (self->conf) == -1)
	return NULL;
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
    int i, argc;

    if (self == NULL || !Tn5250_ConfigCheck (self))
	return NULL;
    if (!PyArg_ParseTuple(args, "L:Config.parse_argv", &list))
	return NULL;
    /* Convert to a C-style argv list. */
    argc = PySequence_Length(list);
    argv = (char**)malloc (sizeof (char*)*argc);
    if (argv == NULL)
	return NULL;
    for (i = 0; i < argc; i++) {
	PyObject* str;
	char *data;
	str = PySequence_GetItem (list, i);
	Py_INCREF(str);
	data = PyString_AsString (str);
	argv[i] = (char*)malloc (strlen(data)+1);
	strcpy (argv[i], data);
	Py_DECREF(str);
    }
    if (tn5250_config_parse_argv (self->conf, argc, argv) == -1)
	return NULL;
    for (i = 0; i < argc; i++)
	free (argv[i]);
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
	return NULL;
    if (!PyArg_ParseTuple(args, "s:Config.get", &name))
	return NULL;
    ret = tn5250_config_get (self->conf, name);
    if (ret == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    return PyString_FromString (ret);
}

static PyMethodDef Tn5250_ConfigMethods[] = {
    { "load",		tn5250_Config_load,		METH_VARARGS },
    { "load_default",	tn5250_Config_load_default,	METH_VARARGS },
    { "parse_argv",	tn5250_Config_parse_argv,	METH_VARARGS },
    { "get",		tn5250_Config_get,		METH_VARARGS },
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

/****************************************************************************/
/* CursesTerminal class 						    */
/****************************************************************************/

/****************************************************************************/
/* DebugTerminal class (?)						    */
/****************************************************************************/

/****************************************************************************/
/* DisplayBuffer class                                                      */
/****************************************************************************/

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
