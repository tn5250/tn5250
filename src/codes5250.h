#ifndef CODES5250_H
#define CODES5250_H

#ifdef __cplusplus
extern "C" {
#endif

/* Misc */
#define ESC 0x4

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

#define CMD_READ_MDT_FIELDS_ALT			0x82 /* FIXME: */
#define CMD_READ_MDT_FIELDS_IMMEDIATE_ALT	0x83 /* FIXME: */

/* Orders - those tagged FIXME are not implemented: */
#define SOH	0x01		/* Start of header */
#define RA	0x02		/* Repeat to address */
#define EA	0x03		/* Erase to Address on 5494 */ /* FIXME: */
#define TD	0x10		/* Transparent Data on 5494 */ /* FIXME: */
#define SBA	0x11		/* Set buffer address */
#define WEA	0x12		/* Write Extended Attribute on 5494 */ /* FIXME: */
#define IC	0x13		/* Insert cursor */
#define MC	0x14		/* Move Cursor on 5494 */ /* FIXME: */
#define WDSF	0x15		/* Write to Display Structured Field on 5494 */ /* FIXME: */
#define SF	0x1D		/* Start of field */

#ifdef __cplusplus
}

#endif
#endif				/* CODES_5250_H */
