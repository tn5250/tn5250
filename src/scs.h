/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997,1998,1999 Michael Madore
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
#ifndef SCS
#define SCS

#define SCS_NOOP	0x00
#define SCS_TRANSPARENT	0x03
#define SCS_SW		0x2A
#define SCS_RFF		0x3A

#define SCS_AVPP	0xC4
#define SCS_AHPP	0xC0
#define SCS_CR		0x0D
#define SCS_NL		0x15
#define SCS_RNL		0x06
#define SCS_HT		0x05
#define SCS_FF		0x0C

#endif
