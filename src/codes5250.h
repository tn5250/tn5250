/* TN5250
 * Copyright (C) 2000 Michael Madore
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
#ifndef CODES5250_H
#define CODES5250_H

#ifdef __cplusplus
extern "C" {
#endif

/* Misc */
#define ESC 0x4
#define SOHLEN   7
#define ERR_ROW 25

/* Commands */
#define CMD_WRITE_TO_DISPLAY			0x11
#define CMD_CLEAR_UNIT				0x40
#define CMD_CLEAR_UNIT_ALTERNATE		0x20
#define CMD_CLEAR_FORMAT_TABLE			0x50
#define CMD_READ_MDT_FIELDS			0x52
#define CMD_READ_IMMEDIATE			0x72
#define CMD_READ_SCREEN_IMMEDIATE       	0x62
#define CMD_WRITE_STRUCTURED_FIELD		0xF3
#define CMD_SAVE_SCREEN				0x02
#define CMD_RESTORE_SCREEN			0x12
#define CMD_WRITE_ERROR_CODE            	0x21
#define CMD_READ_INPUT_FIELDS			0x42
#define CMD_ROLL				0x23
#define CMD_READ_MDT_FIELDS_ALT			0x82
#define CMD_READ_IMMEDIATE_ALT			0x83

/* Orders - those tagged FIXME are not implemented: */
#define SOH	0x01		/* Start of header */
#define RA	0x02		/* Repeat to address */
#define EA	0x03		/* Erase to Address on 5494 */ /* FIXME: */
#define TD	0x10		/* Transparent Data on 5494 */
#define SBA	0x11		/* Set buffer address */
#define WEA	0x12		/* Write Extended Attribute on 5494 */ /* FIXME: */
#define IC	0x13		/* Insert cursor */
#define MC	0x14		/* Move Cursor on 5494 */
#define WDSF	0x15		/* Write to Display Structured Field on 5494 */ /* FIXME: */
#define SF	0x1D		/* Start of field */

#ifdef __cplusplus
}

#endif
#endif				/* CODES_5250_H */
