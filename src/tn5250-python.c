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
    tn5250_config_unref (self->conf);
    /* PyObject_Del(self); */
}

static PyObject *
tn5250_Config_load (self, args)
    PyObject *self;
    PyObject *args;
{
    char *filename;
    if (self == NULL || !Tn5250_ConfigCheck (self))
	return NULL;
    if (!PyArg_ParseTuple(args, "s:Config.load", &filename))
	return NULL;
    if (tn5250_config_load (((Tn5250_Config*)self)->conf, filename) == -1)
	; /* XXX: Throw an IOError. */
    return Py_None;
}

static PyMethodDef Tn5250_ConfigMethods[] = {
    { "load",	    tn5250_Config_load,		METH_VARARGS },
    {NULL, NULL}
};

statichere PyTypeObject Tn5250_ConfigType = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"Config",				/* tp_name */
	sizeof (Tn5250_Config),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)tn5250_Config_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
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
