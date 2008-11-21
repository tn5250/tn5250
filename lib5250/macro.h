/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997-2008 Michael Madore
 * 
 * This file is part of TN5250.
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1, or (at your option)
 * any later version.
 * 
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 */
#ifndef MACRO_H
#define MACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#define	MACRO_BUFSIZE	2000		/* 9999 Max */

/****s* lib5250/Tn5250Macro
 * NAME
 *    Tn5250Macro
 * SYNOPSIS
 *
 * DESCRIPTION
 *
 * SOURCE
 */
struct _Tn5250Macro {
   char	RState ;		/* Macro record state */
   char	EState ;		/* Macro execution state */
   int	FctnKey ;
   int	*BuffM[24] ;
   int	TleBuff ;
   char	*fname ;		/* Macro file name */
};

typedef struct _Tn5250Macro Tn5250Macro;
/*******/

extern Tn5250Macro *tn5250_macro_init() ;
extern void tn5250_macro_exit(Tn5250Macro * This) ;
extern int tn5250_macro_attach (Tn5250Display *This, Tn5250Macro *Macro) ;
extern char tn5250_macro_rstate (Tn5250Display *This) ;
extern char tn5250_macro_startdef (Tn5250Display *This) ;
extern void tn5250_macro_enddef (Tn5250Display *This) ;
extern char tn5250_macro_recfunct (Tn5250Display *This, int key) ;
extern void tn5250_macro_reckey (Tn5250Display *This, int key) ;
extern char  * tn5250_macro_printstate (Tn5250Display *This) ;
extern char tn5250_macro_estate (Tn5250Display *This) ;
extern char tn5250_macro_startexec (Tn5250Display *This) ;
extern void tn5250_macro_endexec (Tn5250Display *This) ;
extern char tn5250_macro_execfunct (Tn5250Display *This, int key) ;
extern int tn5250_macro_getkey (Tn5250Display *This, char *Last) ;

#ifdef __cplusplus
}

#endif
#endif				/* MACRO_H */
