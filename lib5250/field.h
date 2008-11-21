#ifndef FIELD_H
#define FIELD_H

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

#ifdef __cplusplus
extern "C" {
#endif

   struct _Tn5250Field;
   struct _Tn5250DBuffer;

/****s* lib5250/Tn5250Field
 * NAME
 *    Tn5250Field
 * SYNOPSIS
 *    int w = tn5250_dbuffer_width(dbuffer);
 *    Tn5250Field *field = tn5250_field_new(w);
 *    field->start_row = 2;
 *    field->start_col = 2;
 *    field->length = 10;
 *    field->FFW = TN5250_FIELD_NUM_ONLY | TN5250_FIELD_DUP_ENABLE;
 *    tn5250_dbuffer_add_field (dbuffer, field);
 * DESCRIPTION
 *    The Tn5250Field object manages an input field on the display.  It
 *    does not hold the actual data from the field; rather, that is
 *    contained by the display buffer.
 * SOURCE
 */
struct _Tn5250Field {
   struct _Tn5250Field /*@dependent@*/ /*@null@*/ *next;
   struct _Tn5250Field /*@dependent@*/ /*@null@*/ *prev;
   int id;
   int entry_id;
   int resequence;
   short magstripe;
   short lightpen;
   short magandlight;
   short lightandattn;
   short ideographiconly;
   short ideographicdatatype;
   short ideographiceither;
   short ideographicopen;
   int transparency;
   short forwardedge;
   short continuous;
   short continued_first;
   short continued_middle;
   short continued_last;
   short wordwrap;
   int nextfieldprogressionid;
   unsigned char highlightentryattr;
   unsigned char pointeraid;
   short selfcheckmod11;
   short selfcheckmod10;
   struct _Tn5250DBuffer /*@dependent@*/ *table;

   int w;	/* Display width, needed for some calcs */

   Tn5250Uint16 FFW;

  /* The problem with this single FCW is that there can be multiple FCWs
   * per field, so just one doesn't cut it.  We could make a linked list
   * of FCWs defined for each field, but that may be overkill.  For now,
   * keep this one and add a flag above for each capability we support.
   *
   * Tn5250Uint16 FCW;
   */
   unsigned char attribute;
   int start_row;
   int start_col;
   int length;

   void *script_slot;
};

typedef struct _Tn5250Field Tn5250Field;
/*******/

/* Field Formats (Byte one) */
#define TN5250_FIELD_BYPASS		0x2000
#define TN5250_FIELD_DUP_ENABLE		0x1000
#define TN5250_FIELD_MODIFIED		0x0800
#define TN5250_FIELD_ALPHA_SHIFT	0x0000	/* 000 */
#define TN5250_FIELD_ALPHA_ONLY		0x0100	/* 100 */
#define TN5250_FIELD_NUM_SHIFT		0x0200	/* 010 */
#define TN5250_FIELD_NUM_ONLY		0x0300	/* 011 */
#define TN5250_FIELD_KATA_SHIFT		0x0400	/* 001 */
#define TN5250_FIELD_DIGIT_ONLY		0x0500	/* 101 */
#define TN5250_FIELD_MAG_READER		0x0600	/* 110 */
#define TN5250_FIELD_SIGNED_NUM		0x0700	/* 111 */
#define TN5250_FIELD_FIELD_MASK		0x0700	/* 111 */

/* Field Formats (Byte two) */
#define TN5250_FIELD_AUTO_ENTER		0x0080
#define TN5250_FIELD_FER		0x0040
#define TN5250_FIELD_MONOCASE		0x0020
#define TN5250_FIELD_RESERVED		0x0001
#define TN5250_FIELD_MANDATORY		0x0008
#define TN5250_FIELD_NO_ADJUST		0x0000
#define TN5250_FIELD_MF_RESERVED_1	0x0001
#define TN5250_FIELD_MF_RESERVED_2	0x0002
#define TN5250_FIELD_MF_RESERVED_3	0x0003
#define TN5250_FIELD_MF_RESERVED_4	0x0004
#define TN5250_FIELD_RIGHT_ZERO		0x0005
#define TN5250_FIELD_RIGHT_BLANK	0x0006
#define TN5250_FIELD_MANDATORY_FILL	0x0007
#define TN5250_FIELD_MAND_FILL_MASK	0x0007

/* Manipulate fields */
   extern Tn5250Field /*@only@*/ /*@null@*/ *tn5250_field_new(int w);
   extern Tn5250Field /*@only@*/ /*@null@*/ *tn5250_field_copy(Tn5250Field * This);
   extern void tn5250_field_destroy(Tn5250Field /*@only@*/ * This);

   extern int tn5250_field_hit_test(Tn5250Field * This, int y, int x);
   extern int tn5250_field_start_pos(Tn5250Field * This);
   extern int tn5250_field_end_pos(Tn5250Field * This);
   extern int tn5250_field_end_row(Tn5250Field * This);
   extern int tn5250_field_end_col(Tn5250Field * This);
   extern const char *tn5250_field_description(Tn5250Field * This);
   extern const char *tn5250_field_adjust_description(Tn5250Field * This);

   extern int tn5250_field_count_left(Tn5250Field * This, int y, int x);
   extern int tn5250_field_count_right(Tn5250Field * This, int y, int x);
   extern void tn5250_field_set_mdt(Tn5250Field *This);
   extern int tn5250_field_valid_char(Tn5250Field *This, int ch, int *src);

#define tn5250_field_mdt(This) \
	(((This)->FFW & TN5250_FIELD_MODIFIED) != 0)
#define tn5250_field_clear_mdt(This) \
	(void)((This)->FFW &= ~TN5250_FIELD_MODIFIED)
#define tn5250_field_is_bypass(This) \
	(((This)->FFW & TN5250_FIELD_BYPASS) != 0)
#define tn5250_field_is_dup_enable(This) \
	(((This)->FFW & TN5250_FIELD_DUP_ENABLE) != 0)
#define tn5250_field_is_auto_enter(This) \
	(((This)->FFW & TN5250_FIELD_AUTO_ENTER) != 0)
#define tn5250_field_is_fer(This) \
	(((This)->FFW & TN5250_FIELD_FER) != 0)
#define tn5250_field_is_monocase(This) \
	(((This)->FFW & TN5250_FIELD_MONOCASE) != 0)
#define tn5250_field_is_mandatory(This) \
	(((This)->FFW & TN5250_FIELD_MANDATORY) != 0)

/* Mandatory fill type macros */
#define tn5250_field_mand_fill_type(This) \
	((This)->FFW & TN5250_FIELD_MAND_FILL_MASK)
#define tn5250_field_is_no_adjust(This) \
	(tn5250_field_mand_fill_type (This) == TN5250_FIELD_NO_ADJUST)
#define tn5250_field_is_right_zero(This) \
	(tn5250_field_mand_fill_type (This) == TN5250_FIELD_RIGHT_ZERO)
#define tn5250_field_is_right_blank(This) \
	(tn5250_field_mand_fill_type (This) == TN5250_FIELD_RIGHT_BLANK)
#define tn5250_field_is_mand_fill(This) \
	(tn5250_field_mand_fill_type (This) != 0)

#define tn5250_field_type(This) \
	((This)->FFW & TN5250_FIELD_FIELD_MASK)
#define tn5250_field_is_alpha_shift(This) \
	(tn5250_field_type (This) == TN5250_FIELD_ALPHA_SHIFT)
#define tn5250_field_is_alpha_only(This) \
	(tn5250_field_type (This) == TN5250_FIELD_ALPHA_ONLY)
#define tn5250_field_is_num_shift(This) \
	(tn5250_field_type (This) == TN5250_FIELD_NUM_SHIFT)
#define tn5250_field_is_num_only(This) \
	(tn5250_field_type (This) == TN5250_FIELD_NUM_ONLY)
#define tn5250_field_is_kata_shift(This) \
	(tn5250_field_type (This) == TN5250_FIELD_KATA_SHIFT)
#define tn5250_field_is_digit_only(This) \
	(tn5250_field_type (This) == TN5250_FIELD_DIGIT_ONLY)
#define tn5250_field_is_mag_reader(This) \
	(tn5250_field_type (This) == TN5250_FIELD_MAG_READER)
#define tn5250_field_is_signed_num(This) \
	(tn5250_field_type (This) == TN5250_FIELD_SIGNED_NUM)


#define TN5250_FIELD_CONTROL_HILIGHTED_ENTRY    0x8900
#define TN5250_FIELD_CONTROL_CONTINUED          0x8600

#define tn5250_field_control(This) \
        ((This)->FCW)
#define tn5250_field_is_higlighted_entry(This) \
        ( (tn5250_field_control (This) & 0xFF00) == TN5250_FIELD_CONTROL_HILIGHTED_ENTRY)
#define tn5250_field_is_continued(This) \
        ((This)->continuous)
#define tn5250_field_is_continued_first(This) \
        ((This)->continued_first)
#define tn5250_field_is_continued_middle(This) \
        ((This)->continued_middle)
#define tn5250_field_is_continued_last(This) \
        ((This)->continued_last)
#define tn5250_field_is_wordwrap(This) \
        ((This)->wordwrap)


#define tn5250_field_attribute(This) \
	((This)->attribute)
#define tn5250_field_start_row(This) \
	((This)->start_row)
#define tn5250_field_start_col(This) \
	((This)->start_col)
#define tn5250_field_length(This) \
	((This)->length)


#ifdef NDEBUG
#define tn5250_field_dump(This) (void)0
#else
   extern void tn5250_field_dump(Tn5250Field * This);
#endif


/* Manipulate field lists */
   extern Tn5250Field /*@null@*/ *tn5250_field_list_destroy(Tn5250Field /*@null@*/ /*@only@*/ * list);
   extern Tn5250Field *tn5250_field_list_add(Tn5250Field /*@null@*/ * list, Tn5250Field /*@dependent@*/ * node);
   extern Tn5250Field /*@null@*/ * tn5250_field_list_remove(Tn5250Field /*@null@*/ * list, Tn5250Field * node);
   extern Tn5250Field /*@null@*/ /*@observer@*/ * tn5250_field_list_find_by_id(Tn5250Field /*@null@*/ * list, int id);
   extern Tn5250Field /*@null@*/ *tn5250_field_list_copy(Tn5250Field /*@null@*/ * list);

#ifdef __cplusplus
}

#endif
#endif				/* FIELD_H */

/* vi:set cindent sts=3 sw=3: */
