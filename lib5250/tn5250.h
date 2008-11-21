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
#ifndef TN5250_H
#define TN5250_H

#ifdef __cplusplus
extern "C" {
#endif

	/* We need this */
#include <stdio.h>

#include <tn5250/buffer.h>
#include <tn5250/utility.h>
#include <tn5250/codes5250.h>
#include <tn5250/dbuffer.h>
#include <tn5250/field.h>
#include <tn5250/display.h>
#include <tn5250/macro.h>
#include <tn5250/menu.h>
#include <tn5250/record.h>
#include <tn5250/stream.h>
#include <tn5250/scrollbar.h>
#include <tn5250/window.h>

#include <tn5250/terminal.h>
#include <tn5250/session.h>
#include <tn5250/printsession.h>
#include <tn5250/debug.h>

#include <tn5250/conf.h>

#ifdef __cplusplus
}
#endif

#endif				/* TN5250_H */
