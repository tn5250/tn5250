/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997 Michael Madore
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 * As a special exception, the Free Software Foundation gives permission
 * for additional uses of the text contained in its release of TN5250.
 * 
 * The exception is that, if you link the TN5250 library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the TN5250 library code into it.
 * 
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 * 
 * If you write modifications of your own for TN5250, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice. */
#ifndef TN5250_H
#define TN5250_H

#ifdef __cplusplus
extern "C" {
#endif

#include <tn5250/config.h>

	/* We need this */
#include <stdio.h>

#include <tn5250/buffer.h>
#include <tn5250/utility.h>
#include <tn5250/codes5250.h>
#include <tn5250/dbuffer.h>
#include <tn5250/field.h>
#include <tn5250/record.h>
#include <tn5250/stream.h>

#include <tn5250/terminal.h>
#include <tn5250/session.h>
#include <tn5250/printsession.h>
#ifdef USE_CURSES
#include <tn5250/cursesterm.h>
#endif
#ifdef USE_SLANG
#include <tn5250/slangterm.h>
#endif
#include <tn5250/debug.h>

#include <tn5250/display.h>
#include <tn5250/conf.h>

#ifdef __cplusplus
}
#endif

#endif				/* TN5250_H */
