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

/******************************************************************
 * Operator Error Codes see 5494 User's Guide (GA27-3960-03) 2.3.4
 ******************************************************************/
 
#define ERR_DONT_KNOW		0x01
#define ERR_BYPASS_FIELD	0x04
#define ERR_NO_FIELD		0x05
#define ERR_INVALID_SYSREQ	0x06
#define ERR_MANDATORY_ENTRY	0x07
#define ERR_ALPHA_ONLY		0x08
#define ERR_NUMERIC_ONLY	0x09
#define ERR_DIGITS_ONLY		0x10
#define ERR_LAST_SIGNED		0x11
#define ERR_NO_ROOM		0x12
#define ERR_MANADATORY_FILL	0x14
#define ERR_CHECK_DIGIT		0x15
#define ERR_NOT_SIGNED		0x16
#define ERR_EXIT_NOT_VALID	0x18
#define ERR_DUP_NOT_ENABLED	0x19
#define ERR_NO_FIELD_EXIT	0x20
#define ERR_NO_INPUT		0x26
#define ERR_BAD_CHAR		0x27

#define MSG_DONT_KNOW		"Keyboard overrun."
#define MSG_BYPASS_FIELD	"Entry of data not allowed in this " \
		"input/output field."
#define MSG_NO_FIELD		"Cursor in protected area of display."
#define MSG_INVALID_SYSREQ	"Key pressed following System Request " \
		"key was not valid."
#define MSG_MANDATORY_ENTRY	"Mandatory data entry field. " \
		"Must have data entered."
#define MSG_ALPHA_ONLY		"Field requires alphabetic characters."
#define MSG_NUMERIC_ONLY	"Field requires numeric characters."
#define MSG_DIGITS_ONLY		"Only characters 0 through 9 allowed."
#define MSG_LAST_SIGNED		"Key for sign position of field not valid."
#define MSG_NO_ROOM		"No room to insert data."
#define MSG_MANADATORY_FILL	"Mandatory fill field. Must fill to exit."
#define MSG_CHECK_DIGIT		"Modulo 10 or 11 check digit error."
#define MSG_NOT_SIGNED		"Field Minus key not valid in field."
#define MSG_EXIT_NOT_VALID	"The key used to exit field not valid."
#define MSG_DUP_NOT_ENABLED	"Duplicate key or Field Mark key not " \
		"allowed in field."
#define MSG_NO_FIELD_EXIT	"Enter key not allowed in field."
#define MSG_NO_INPUT		"Field- entry not allowed."
#define MSG_BAD_CHAR		"Cannot use undefined key."
#define MSG_NO_HELP		"No help text is available."

#ifdef JAPAN
 #define ERR_DBCS_WRONG_TYPE	0x60
 #define ERR_SBCS_WRONG_TYPE	0x61
 #define MSG_DBCS_WRONG_TYPE	"Field requires alphanumeric characters."
 #define MSG_SBCS_WRONG_TYPE	"Field requires double-byte characters."
#endif

/*************************************************************************
 * More error codes - Data Stream Negative Responses (SC30-3533-04) 13.4 *
 *************************************************************************/
 
#define DSNR_RESEQ_ERR	  03
#define DSNR_INVCURSPOS	0x22
#define DSNR_RAB4WSA	0x23
#define DSNR_INVSFA	0x26
#define DSNR_FLDEOD	0x28
#define DSNR_FMTOVF	0x29
#define DSNR_WRTEOD	0x2A
#define DSNR_SOHLEN	0x2B
#define DSNR_ROLLPARM	0x2C
#define DSNR_NO_ESC	0x31
#define DSNR_INV_WECW	0x32
#define DSNR_UNKNOWN	-1
 
#define EMSG_RESEQ_ERR	"Format table resequencing error."
#define EMSG_INVCURSPOS	"Write to display order row/col address is not valid"
#define EMSG_RAB4WSA	"Repeat to Address less than the current WS address."
#define EMSG_INVSFA	"Start-of-field order address not valid"
#define EMSG_FLDEOD	"Field extends past the end of the display."
#define EMSG_FMTOVF	"Format table overflow."
#define EMSG_WRTEOD	"Attempted to write past the end of display."
#define EMSG_SOHLEN	"Start-of-header length not valid."
#define EMSG_ROLLPARM	"Invalid ROLL command parameter."
#define EMSG_NO_ESC	"No escape code was found where it was expected."
#define EMSG_INV_WECW	"Invalid row/col address on WEC TO WINDOW command."

/* Field Attributes - C.f. 5494 Functions Reference (SC30-3533-04),
   Section 15.6.12.3.
   Bits 0-2 always set to 001 to identifiy as an attribute byte. */
#define ATTR_5250_GREEN		0x20	/* Default */
#define ATTR_5250_WHITE		0x22
#define ATTR_5250_NONDISP	0x27	/* Nondisplay */
#define ATTR_5250_RED		0x28
#define ATTR_5250_TURQ		0x30
#define ATTR_5250_YELLOW	0x32
#define ATTR_5250_PINK		0x38
#define ATTR_5250_BLUE		0x3A

#define ATTR_5250_NORMAL	ATTR_5250_GREEN

#ifdef __cplusplus
}

#endif
#endif				/* CODES_5250_H */

