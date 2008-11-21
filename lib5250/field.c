/* TN5250
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

#include "tn5250-private.h"


/****f* lib5250/tn5250_field_new
 * NAME
 *    tn5250_field_new
 * SYNOPSIS
 *    ret = tn5250_field_new (w);
 * INPUTS
 *    int                  w          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Field *
tn5250_field_new (int w)
{
  Tn5250Field *This = tn5250_new (Tn5250Field, 1);
  if (This == NULL)
    {
      return NULL;
    }
  memset (This, 0, sizeof (Tn5250Field));
  This->id = -1;
  This->w = w;
  return This;
}


/****f* lib5250/tn5250_field_copy
 * NAME
 *    tn5250_field_copy
 * SYNOPSIS
 *    ret = tn5250_field_copy (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Field *
tn5250_field_copy (Tn5250Field * This)
{
  Tn5250Field *fld = tn5250_new (Tn5250Field, 1);
  if (fld == NULL)
    {
      return NULL;
    }
  memcpy (fld, This, sizeof (Tn5250Field));
  fld->next = NULL;
  fld->prev = NULL;
  fld->script_slot = NULL;
  return fld;
}


/****f* lib5250/tn5250_field_destroy
 * NAME
 *    tn5250_field_destroy
 * SYNOPSIS
 *    tn5250_field_destroy (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_field_destroy (Tn5250Field * This)
{
  free (This);
}


#ifndef NDEBUG
/****f* lib5250/tn5250_field_dump
 * NAME
 *    tn5250_field_dump
 * SYNOPSIS
 *    tn5250_field_dump (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_field_dump (Tn5250Field * This)
{
  Tn5250Uint16 ffw = This->FFW;

  TN5250_LOG (("tn5250_field_dump: ffw flags = "));
  if ((ffw & TN5250_FIELD_BYPASS) != 0)
    {
      TN5250_LOG (("bypass "));
    }
  if ((ffw & TN5250_FIELD_DUP_ENABLE) != 0)
    {
      TN5250_LOG (("dup-enable "));
    }
  if ((ffw & TN5250_FIELD_MODIFIED) != 0)
    {
      TN5250_LOG (("mdt "));
    }
  if ((ffw & TN5250_FIELD_AUTO_ENTER) != 0)
    {
      TN5250_LOG (("auto-enter"));
    }
  if ((ffw & TN5250_FIELD_FER) != 0)
    {
      TN5250_LOG (("fer "));
    }
  if ((ffw & TN5250_FIELD_MONOCASE) != 0)
    {
      TN5250_LOG (("monocase "));
    }
  if ((ffw & TN5250_FIELD_MANDATORY) != 0)
    {
      TN5250_LOG (("mandatory "));
    }

  TN5250_LOG (("\ntn5250_field_dump: fcw flags = "));
  if (This->resequence != 0)
    {
      TN5250_LOG (("Entry field resequencing: %d ", This->resequence));
    }
  if (This->magstripe)
    {
      TN5250_LOG (("Magnetic stripe reader entry field "));
    }
  if (This->lightpen)
    {
      TN5250_LOG (("Selector light pen or cursor select field "));
    }
  if (This->magandlight)
    {
      TN5250_LOG (("Magnetic stripe reader and selector light pen entry field "));
    }
  if (This->lightandattn)
    {
      TN5250_LOG (("Selector light pen and selectable attention entry field "));
    }
  if (This->ideographiconly)
    {
      TN5250_LOG (("Ideographic-only entry field "));
    }
  if (This->ideographicdatatype)
    {
      TN5250_LOG (("Ideographic data type entry field "));
    }
  if (This->ideographiceither)
    {
      TN5250_LOG (("Ideographic-either entry field "));
    }
  if (This->ideographicopen)
    {
      TN5250_LOG (("Ideographic-open entry field "));
    }
  if (This->transparency != 0)
    {
      TN5250_LOG (("Transparency entry field: %d ", This->transparency));
    }
  if (This->forwardedge)
    {
      TN5250_LOG (("Forward edge trigger entry field "));
    }
  if (This->continuous)
    {
      TN5250_LOG (("continuous "));
    }
  if (tn5250_field_is_continued_first (This))
    {
      TN5250_LOG (("(first) "));
    }
  if (tn5250_field_is_continued_middle (This))
    {
      TN5250_LOG (("(middle) "));
    }
  if (tn5250_field_is_continued_last (This))
    {
      TN5250_LOG (("(last) "));
    }
  if (This->wordwrap)
    {
      TN5250_LOG (("wordwrap "));
    }
  if (This->nextfieldprogressionid != 0)
    {
      TN5250_LOG (("cursor progression: %d ", This->nextfieldprogressionid));
    }
  if ((int) This->highlightentryattr != 0)
    {
      TN5250_LOG (("Highlighted entry field: %x ", This->highlightentryattr));
    }
  if ((int) This->pointeraid != 0)
    {
      TN5250_LOG (("Pointer device selection entry field: %x ", This->pointeraid));
    }
  if (This->selfcheckmod11)
    {
      TN5250_LOG (("Self-check modulus 11 entry field "));
    }
  if (This->selfcheckmod10)
    {
      TN5250_LOG (("Self-check modulus 10 entry field "));
    }

  TN5250_LOG (("\ntn5250_field_dump: type = %s\n",
	       tn5250_field_description (This)));
  TN5250_LOG (("tn5250_field_dump: adjust = %s\ntn5250_field_dump: data = ",
	       tn5250_field_adjust_description (This)));
  TN5250_LOG (("\n"));
}
#endif


/****f* lib5250/tn5250_field_hit_test
 * NAME
 *    tn5250_field_hit_test
 * SYNOPSIS
 *    ret = tn5250_field_hit_test (This, y, x);
 * INPUTS
 *    Tn5250Field *        This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    Determine if the screen position at row ``y'', column ``x'' is contained
 *    within this field.  (A hit-test, in other words.)
 *****/
int
tn5250_field_hit_test (Tn5250Field * This, int y, int x)
{
  int pos = (y * This->w) + x;
  return (pos >= tn5250_field_start_pos (This)
	  && pos <= tn5250_field_end_pos (This));
}


/****f* lib5250/tn5250_field_start_pos
 * NAME
 *    tn5250_field_start_pos
 * SYNOPSIS
 *    ret = tn5250_field_start_pos (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Figure out the starting address of this field.
 *****/
int
tn5250_field_start_pos (Tn5250Field * This)
{
  return This->start_row * This->w + This->start_col;
}


/****f* lib5250/tn5250_field_end_pos
 * NAME
 *    tn5250_field_end_pos
 * SYNOPSIS
 *    ret = tn5250_field_end_pos (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Figure out the ending address of this field.
 *****/
int
tn5250_field_end_pos (Tn5250Field * This)
{
  return tn5250_field_start_pos (This) + tn5250_field_length (This) - 1;
}


/****f* lib5250/tn5250_field_end_row
 * NAME
 *    tn5250_field_end_row
 * SYNOPSIS
 *    ret = tn5250_field_end_row (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Figure out the ending row of this field.
 *****/
int
tn5250_field_end_row (Tn5250Field * This)
{
  return tn5250_field_end_pos (This) / This->w;
}


/****f* lib5250/tn5250_field_end_col
 * NAME
 *    tn5250_field_end_col
 * SYNOPSIS
 *    ret = tn5250_field_end_col (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Figure out the ending column of this field.
 *****/
int
tn5250_field_end_col (Tn5250Field * This)
{
  return tn5250_field_end_pos (This) % This->w;
}


/****f* lib5250/tn5250_field_description
 * NAME
 *    tn5250_field_description
 * SYNOPSIS
 *    ret = tn5250_field_description (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Get a description of this field.
 *****/
const char *
tn5250_field_description (Tn5250Field * This)
{
  switch (This->FFW & TN5250_FIELD_FIELD_MASK)
    {
    case TN5250_FIELD_ALPHA_SHIFT:
      return "Alpha Shift";
    case TN5250_FIELD_DUP_ENABLE:
      return "Dup Enabled";
    case TN5250_FIELD_ALPHA_ONLY:
      return "Alpha Only";
    case TN5250_FIELD_NUM_SHIFT:
      return "Numeric Shift";
    case TN5250_FIELD_NUM_ONLY:
      return "Numeric Only";
    case TN5250_FIELD_KATA_SHIFT:
      return "Katakana";
    case TN5250_FIELD_DIGIT_ONLY:
      return "Digits Only";
    case TN5250_FIELD_MAG_READER:
      return "Mag Reader I/O Field";
    case TN5250_FIELD_SIGNED_NUM:
      return "Signed Numeric";
    default:
      return "(?)";
    }
}


/****f* lib5250/tn5250_field_adjust_description
 * NAME
 *    tn5250_field_adjust_description
 * SYNOPSIS
 *    ret = tn5250_field_adjust_description (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Get a description of the mandatory fill mode for this field.
 *****/
const char *
tn5250_field_adjust_description (Tn5250Field * This)
{
  switch (This->FFW & TN5250_FIELD_MAND_FILL_MASK)
    {
    case TN5250_FIELD_NO_ADJUST:
      return "No Adjust";
    case TN5250_FIELD_MF_RESERVED_1:
      return "Reserved 1";
    case TN5250_FIELD_MF_RESERVED_2:
      return "Reserved 2";
    case TN5250_FIELD_MF_RESERVED_3:
      return "Reserved 3";
    case TN5250_FIELD_MF_RESERVED_4:
      return "Reserved 4";
    case TN5250_FIELD_RIGHT_ZERO:
      return "Right Adjust, Zero Fill";
    case TN5250_FIELD_RIGHT_BLANK:
      return "Right Adjust, Blank Fill";
    case TN5250_FIELD_MANDATORY_FILL:
      return "Mandatory Fill";
    default:
      return "";
    }
}


/****f* lib5250/tn5250_field_count_left
 * NAME
 *    tn5250_field_count_left
 * SYNOPSIS
 *    ret = tn5250_field_count_left (This, y, x);
 * INPUTS
 *    Tn5250Field *        This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    Return the number of characters in the this field which
 *    are to the left of the specified cursor position.  Used
 *    as an index to insert data when the user types.
 *****/
int
tn5250_field_count_left (Tn5250Field * This, int y, int x)
{
  int pos;

  pos = (y * This->w + x);
  pos -= tn5250_field_start_pos (This);

  TN5250_ASSERT (tn5250_field_hit_test (This, y, x));
  TN5250_ASSERT (pos >= 0);
  TN5250_ASSERT (pos < tn5250_field_length (This));

  return pos;
}


/****f* lib5250/tn5250_field_count_right
 * NAME
 *    tn5250_field_count_right
 * SYNOPSIS
 *    ret = tn5250_field_count_right (This, y, x);
 * INPUTS
 *    Tn5250Field *        This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    This returns the number of characters in the specified field
 *    which are to the right of the specified cursor position.
 *****/
int
tn5250_field_count_right (Tn5250Field * This, int y, int x)
{
  TN5250_ASSERT (tn5250_field_hit_test (This, y, x));
  return tn5250_field_end_pos (This) - (y * This->w + x);
}


/****f* lib5250/tn5250_field_valid_char
 * NAME
 *    tn5250_field_valid_char
 * SYNOPSIS
 *    ret = tn5250_field_valid_char (field, ch);
 * INPUTS
 *    Tn5250Field *        field      - 
 *    int                  ch         - 
 *    int         *        src        -
 * DESCRIPTION
 *    Determine if the supplied character is a valid data character
 *    for this field, and return system reference code for errors (SRC)
 *****/
int
tn5250_field_valid_char (Tn5250Field * field, int ch, int *src)
{
  TN5250_LOG (("HandleKey: fieldtype = %d; char = '%c'.\n",
	       tn5250_field_type (field), ch));

  *src = TN5250_KBDSRC_NONE;

  switch (tn5250_field_type (field))
    {
    case TN5250_FIELD_ALPHA_SHIFT:
      return 1;

    case TN5250_FIELD_ALPHA_ONLY:
      if (!(isalpha (ch) || ch == ',' || ch == '.' || ch == '-' || ch == ' '))
	{
	  *src = TN5250_KBDSRC_ALPHAONLY;
	  return 0;
	}
      return 1;

    case TN5250_FIELD_NUM_SHIFT:
      return 1;

    case TN5250_FIELD_NUM_ONLY:
      if (!(isdigit (ch) || ch == ',' || ch == '.' || ch == '-' || ch == ' '))
	{
	  *src = TN5250_KBDSRC_NUMONLY;
	  return 0;
	}
      return 1;

    case TN5250_FIELD_KATA_SHIFT:
      TN5250_LOG (("KATAKANA not implemented.\n"));
      return 1;

    case TN5250_FIELD_DIGIT_ONLY:
      if (!isdigit (ch))
	{
	  *src = TN5250_KBDSRC_ONLY09;
	  return 0;
	}
      return 1;

    case TN5250_FIELD_MAG_READER:
      *src = TN5250_KBDSRC_DATA_DISALLOWED;
      return 0;

    case TN5250_FIELD_SIGNED_NUM:
      if (!isdigit (ch))
	{
	  *src = TN5250_KBDSRC_ONLY09;
	  return 0;
	}
      return 1;
    }
  return 0;
}


/****f* lib5250/tn5250_field_set_mdt
 * NAME
 *    tn5250_field_set_mdt
 * SYNOPSIS
 *    tn5250_field_set_mdt (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Set the MDT flag for this field and for the table which owns it.
 *****/
void
tn5250_field_set_mdt (Tn5250Field * This)
{
  TN5250_ASSERT (This->table != NULL);

  /* Taken from tn5250j
   * get the first field of a continued edit field if it is continued
   */
  if ((This->continuous) && !tn5250_field_is_continued_first (This))
    {
      Tn5250Field *iter;

      for (iter = This->prev;
	   (iter->continuous && !tn5250_field_is_continued_first (iter));
	   iter = iter->prev)
	{
	  TN5250_ASSERT (iter->continuous);
	}

      tn5250_field_set_mdt (iter);
      tn5250_dbuffer_set_mdt (iter->table);
    }
  else
    {
      This->FFW |= TN5250_FIELD_MODIFIED;
      tn5250_dbuffer_set_mdt (This->table);
    }
  return;
}


/****f* lib5250/tn5250_field_list_destroy
 * NAME
 *    tn5250_field_list_destroy
 * SYNOPSIS
 *    ret = tn5250_field_list_destroy (list);
 * INPUTS
 *    Tn5250Field *        list       - 
 * DESCRIPTION
 *    Destroy all fields in a field list.
 *****/
Tn5250Field *
tn5250_field_list_destroy (Tn5250Field * list)
{
  Tn5250Field *iter, *next;

  if ((iter = list) != NULL)
    {
      /*@-usereleased@ */
      do
	{
	  next = iter->next;
	  tn5250_field_destroy (iter);
	  iter = next;
	}
      while (iter != list);
      /*@=usereleased@ */
    }
  return NULL;
}


/****f* lib5250/tn5250_field_list_add
 * NAME
 *    tn5250_field_list_add
 * SYNOPSIS
 *    ret = tn5250_field_list_add (list, node);
 * INPUTS
 *    Tn5250Field *        list       - 
 *    Tn5250Field *        node       - 
 * DESCRIPTION
 *    Add a field to the end of a list of fields.
 *****/
Tn5250Field *
tn5250_field_list_add (Tn5250Field * list, Tn5250Field * node)
{
  node->prev = node->next = NULL;

  if (list == NULL)
    {
      node->next = node->prev = node;
      return node;
    }
  node->next = list;
  node->prev = list->prev;
  node->prev->next = node;
  node->next->prev = node;
  return list;
}


/****f* lib5250/tn5250_field_list_remove
 * NAME
 *    tn5250_field_list_remove
 * SYNOPSIS
 *    ret = tn5250_field_list_remove (list, node);
 * INPUTS
 *    Tn5250Field *        list       - 
 *    Tn5250Field *        node       - 
 * DESCRIPTION
 *    Remove a field from a list of fields.
 *****/
Tn5250Field *
tn5250_field_list_remove (Tn5250Field * list, Tn5250Field * node)
{
  if (list == NULL)
    {
      return NULL;
    }
  if (list->next == list && list == node)
    {
      node->next = node->prev = NULL;
      return NULL;
    }
  if (list == node)
    {
      list = list->next;
    }

  node->next->prev = node->prev;
  node->prev->next = node->next;
  node->prev = node->next = NULL;
  return list;
}


/****f* lib5250/tn5250_field_list_find_by_id
 * NAME
 *    tn5250_field_list_find_by_id
 * SYNOPSIS
 *    ret = tn5250_field_list_find_by_id (list, id);
 * INPUTS
 *    Tn5250Field *        list       - 
 *    int                  id         - 
 * DESCRIPTION
 *    Find a field by its numeric id.
 *****/
Tn5250Field *
tn5250_field_list_find_by_id (Tn5250Field * list, int id)
{
  Tn5250Field *iter;

  if ((iter = list) != NULL)
    {
      do
	{
	  if (iter->id == id)
	    return iter;
	  iter = iter->next;
	}
      while (iter != list);
    }
  return NULL;
}


/****f* lib5250/tn5250_field_list_copy
 * NAME
 *    tn5250_field_list_copy
 * SYNOPSIS
 *    ret = tn5250_field_list_copy (This);
 * INPUTS
 *    Tn5250Field *        This       - 
 * DESCRIPTION
 *    Copy all fields in a list to another list.
 *****/
Tn5250Field *
tn5250_field_list_copy (Tn5250Field * This)
{
  Tn5250Field *new_list = NULL, *iter, *new_field;
  if ((iter = This) != NULL)
    {
      do
	{
	  new_field = tn5250_field_copy (iter);
	  if (new_field != NULL)
	    {
	      new_list = tn5250_field_list_add (new_list, new_field);
	    }
	  iter = iter->next;
	}
      while (iter != This);
    }
  return new_list;
}
