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
#ifndef SLANGTERM_H
#define SLANGTERM_H

#ifdef __cplusplus
extern "C" {
#endif

#if USE_SLANG
#if defined(HAVE_SLANG_H)
#include <slang.h>
#elif defined(HAVE_SLANG_SLANG_H)
#include <slang/slang.h>
#endif
#include "slangterm.h"
#endif

#ifdef USE_SLANG
   extern Tn5250Terminal /*@only@*/ *tn5250_slang_terminal_new(void);
   extern int tn5250_slang_terminal_use_underscores(Tn5250Terminal *, int f);
#endif

#ifdef __cplusplus
}

#endif
#endif				/* SLANGTERM_H */
