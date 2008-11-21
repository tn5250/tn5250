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

#ifdef NDEBUG
#define ASSERT_VALID(This)
#else
#define ASSERT_VALID(This) \
   { \
      TN5250_ASSERT ((This) != NULL); \
      TN5250_ASSERT ((This)->cy >= 0); \
      TN5250_ASSERT ((This)->cx >= 0); \
      TN5250_ASSERT ((This)->cy < (This)->h); \
      TN5250_ASSERT ((This)->cx < (This)->w); \
   }
#endif


/****f* lib5250/tn5250_dbuffer_new
 * NAME
 *    tn5250_dbuffer_new
 * SYNOPSIS
 *    ret = tn5250_dbuffer_new (width, height);
 * INPUTS
 *    int                  width      - 
 *    int                  height     - 
 * DESCRIPTION
 *    Allocates a new display buffer.
 *****/
Tn5250DBuffer *
tn5250_dbuffer_new (int width, int height)
{
  Tn5250DBuffer *This = tn5250_new (Tn5250DBuffer, 1);

  if (This == NULL)
    {
      return NULL;
    }

  This->w = width;
  This->h = height;
  This->cx = This->cy = 0;
  This->tcx = This->tcy = 0;
  This->next = This->prev = NULL;

  This->field_count = 0;
  This->entry_field_count = 0;
  This->window_count = 0;
  This->scrollbar_count = 0;
  This->menubar_count = 0;
  This->field_list = NULL;
  This->window_list = NULL;
  This->scrollbar_list = NULL;
  This->menubar_list = NULL;
  This->master_mdt = 0;
  This->header_data = NULL;
  This->header_length = 0;

  This->script_slot = NULL;

  This->data = tn5250_new (unsigned char, width * height);
  if (This->data == NULL)
    {
      free (This);
      return NULL;
    }

  tn5250_dbuffer_clear (This);
  return This;
}


/****f* lib5250/tn5250_dbuffer_copy
 * NAME
 *    tn5250_dbuffer_copy
 * SYNOPSIS
 *    ret = tn5250_dbuffer_copy (dsp);
 * INPUTS
 *    Tn5250DBuffer *      dsp        - 
 * DESCRIPTION
 *    Allocates a new display buffer and copies the contents of the old
 *    one.
 *****/
Tn5250DBuffer *
tn5250_dbuffer_copy (Tn5250DBuffer * dsp)
{
  Tn5250DBuffer *This = tn5250_new (Tn5250DBuffer, 1);

  if (This == NULL)
    {
      return NULL;
    }

  ASSERT_VALID (dsp);

  This->w = dsp->w;
  This->h = dsp->h;
  This->cx = dsp->cx;
  This->cy = dsp->cy;
  This->tcx = dsp->tcx;
  This->tcy = dsp->tcy;
  This->data = tn5250_new (unsigned char, dsp->w * dsp->h);
  if (This->data == NULL)
    {
      free (This);
      return NULL;
    }
  memcpy (This->data, dsp->data, dsp->w * dsp->h);

  This->field_list = tn5250_field_list_copy (dsp->field_list);
  This->window_list = tn5250_window_list_copy (dsp->window_list);
  This->header_length = dsp->header_length;
  if (dsp->header_data != NULL)
    {
      This->header_data = (unsigned char *) malloc (This->header_length);
      memcpy (This->header_data, dsp->header_data, dsp->header_length);
    }
  else
    {
      This->header_data = NULL;
    }

  ASSERT_VALID (This);
  return This;
}


/****f* lib5250/tn5250_dbuffer_destroy
 * NAME
 *    tn5250_dbuffer_destroy
 * SYNOPSIS
 *    tn5250_dbuffer_destroy (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    Free a display buffer and destroy all sub-structures.
 *****/
void
tn5250_dbuffer_destroy (Tn5250DBuffer * This)
{
  free (This->data);
  if (This->header_data != NULL)
    {
      free (This->header_data);
    }
  (void) tn5250_field_list_destroy (This->field_list);
  (void) tn5250_window_list_destroy (This->window_list);
  free (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_set_header_data
 * NAME
 *    tn5250_dbuffer_set_header_data
 * SYNOPSIS
 *    tn5250_dbuffer_set_header_data (This, data, len);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    unsigned char *      data       - 
 *    int                  len        - 
 * DESCRIPTION
 *    Set the format table header data.
 *****/
void
tn5250_dbuffer_set_header_data (Tn5250DBuffer * This, unsigned char *data,
				int len)
{
  if (This->header_data != NULL)
    {
      free (This->header_data);
    }
  This->header_length = len;
  if (data != NULL)
    {
      TN5250_ASSERT (len > 0);
      This->header_data = (unsigned char *) malloc (len);
      memcpy (This->header_data, data, len);
    }
  return;
}


/****f* lib5250/tn5250_dbuffer_send_data_for_aid_key
 * NAME
 *    tn5250_dbuffer_send_data_for_aid_key
 * SYNOPSIS
 *    ret = tn5250_dbuffer_send_data_for_aid_key (This, k);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  k          - 
 * DESCRIPTION
 *    Determine, according to the format table header, if we should send
 *    data for this aid key.
 *****/
int
tn5250_dbuffer_send_data_for_aid_key (Tn5250DBuffer * This, int k)
{
  int byte, bit, result;

  if (This->header_data == NULL || This->header_length <= 6)
    {
      result = 1;
      TN5250_LOG (("tn5250_dbuffer_send_data_for_aid_key: no format table header or key mask.\n"));
      goto send_data_done;
    }

#ifndef NDEBUG
  TN5250_LOG (("tn5250_dbuffer_send_data_for_aid_key: format table header = \n"));
  for (byte = 0; byte < This->header_length; byte++)
    {
      TN5250_LOG (("0x%02X ", This->header_data[byte]));
    }
  TN5250_LOG (("\n"));
#endif

  switch (k)
    {
    case TN5250_SESSION_AID_F1:
      byte = 6;
      bit = 7;
      break;
    case TN5250_SESSION_AID_F2:
      byte = 6;
      bit = 6;
      break;
    case TN5250_SESSION_AID_F3:
      byte = 6;
      bit = 5;
      break;
    case TN5250_SESSION_AID_F4:
      byte = 6;
      bit = 4;
      break;
    case TN5250_SESSION_AID_F5:
      byte = 6;
      bit = 3;
      break;
    case TN5250_SESSION_AID_F6:
      byte = 6;
      bit = 2;
      break;
    case TN5250_SESSION_AID_F7:
      byte = 6;
      bit = 1;
      break;
    case TN5250_SESSION_AID_F8:
      byte = 6;
      bit = 0;
      break;
    case TN5250_SESSION_AID_F9:
      byte = 5;
      bit = 7;
      break;
    case TN5250_SESSION_AID_F10:
      byte = 5;
      bit = 6;
      break;
    case TN5250_SESSION_AID_F11:
      byte = 5;
      bit = 5;
      break;
    case TN5250_SESSION_AID_F12:
      byte = 5;
      bit = 4;
      break;
    case TN5250_SESSION_AID_F13:
      byte = 5;
      bit = 3;
      break;
    case TN5250_SESSION_AID_F14:
      byte = 5;
      bit = 2;
      break;
    case TN5250_SESSION_AID_F15:
      byte = 5;
      bit = 1;
      break;
    case TN5250_SESSION_AID_F16:
      byte = 5;
      bit = 0;
      break;
    case TN5250_SESSION_AID_F17:
      byte = 4;
      bit = 7;
      break;
    case TN5250_SESSION_AID_F18:
      byte = 4;
      bit = 6;
      break;
    case TN5250_SESSION_AID_F19:
      byte = 4;
      bit = 5;
      break;
    case TN5250_SESSION_AID_F20:
      byte = 4;
      bit = 4;
      break;
    case TN5250_SESSION_AID_F21:
      byte = 4;
      bit = 3;
      break;
    case TN5250_SESSION_AID_F22:
      byte = 4;
      bit = 2;
      break;
    case TN5250_SESSION_AID_F23:
      byte = 4;
      bit = 1;
      break;
    case TN5250_SESSION_AID_F24:
      byte = 4;
      bit = 0;
      break;
    default:
      result = 1;
      goto send_data_done;
    }

  result = ((This->header_data[byte] & (0x80 >> bit)) == 0);
send_data_done:
  TN5250_LOG (("tn5250_dbuffer_send_data_for_aid_key() = %d\n", result));
  return result;
}


/****f* lib5250/tn5250_dbuffer_field_data
 * NAME
 *    tn5250_dbuffer_field_data
 * SYNOPSIS
 *    ret = tn5250_dbuffer_field_data (This, field);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    Tn5250Field *        field      - 
 * DESCRIPTION
 *    Return a pointer into the display buffer data where the specified
 *    field begins.
 *****/
unsigned char *
tn5250_dbuffer_field_data (Tn5250DBuffer * This, Tn5250Field * field)
{
  return &This->data[field->start_row * This->w + field->start_col];
}


/****f* lib5250/tn5250_dbuffer_set_size
 * NAME
 *    tn5250_dbuffer_set_size
 * SYNOPSIS
 *    tn5250_dbuffer_set_size (This, rows, cols);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  rows       - 
 *    int                  cols       - 
 * DESCRIPTION
 *    Resize the display (say, to 132 columns ;)
 *****/
void
tn5250_dbuffer_set_size (Tn5250DBuffer * This, int rows, int cols)
{
  This->w = cols;
  This->h = rows;

  free (This->data);
  This->data = tn5250_new (unsigned char, rows * cols);
  TN5250_ASSERT (This->data != NULL);

  tn5250_dbuffer_clear (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_cursor_set
 * NAME
 *    tn5250_dbuffer_cursor_set
 * SYNOPSIS
 *    tn5250_dbuffer_cursor_set (This, y, x);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_cursor_set (Tn5250DBuffer * This, int y, int x)
{
  This->cy = y;
  This->cx = x;

  TN5250_LOG (("This->cy = %d, This->cx = %d\n", This->cy, This->cx));

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_clear
 * NAME
 *    tn5250_dbuffer_clear
 * SYNOPSIS
 *    tn5250_dbuffer_clear (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_clear (Tn5250DBuffer * This)
{
  memset (This->data, 0, This->w * This->h);
  This->cx = This->cy = 0;
  tn5250_dbuffer_clear_table (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_add_field
 * NAME
 *    tn5250_dbuffer_add_field
 * SYNOPSIS
 *    tn5250_dbuffer_add_field (This, field);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    Tn5250Field *        field      - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_add_field (Tn5250DBuffer * This, Tn5250Field * field)
{
  field->id = This->field_count++;
  field->table = This;
  This->field_list = tn5250_field_list_add (This->field_list, field);

  if ((!tn5250_field_is_continued_middle (field))
      && (!tn5250_field_is_continued_last (field)))
    {
      This->entry_field_count++;
    }
  field->entry_id = This->entry_field_count;

  TN5250_LOG (("adding field: field->id: %d, field->entry_id: %d\n",
	       field->id, field->entry_id));
  return;
}


/****f* lib5250/tn5250_dbuffer_clear_table
 * NAME
 *    tn5250_dbuffer_clear_table
 * SYNOPSIS
 *    tn5250_dbuffer_clear_table (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_clear_table (Tn5250DBuffer * This)
{
  TN5250_LOG (("tn5250_dbuffer_clear_table() entered.\n"));
  This->field_list = tn5250_field_list_destroy (This->field_list);
  /* Comment this for now since the table is cleared just after we have
   * received a Create Window Structured Field command.  We don't really
   * want to blow away our newly created window.
   * FIXME
   */
  /*
     This->window_list = tn5250_window_list_destroy (This->window_list);
     This->scrollbar_list = tn5250_scrollbar_list_destroy (This->scrollbar_list);
   */
  This->field_count = 0;
  This->entry_field_count = 0;
  /*
     This->window_count = 0;
     This->scrollbar_count = 0;
   */
  This->master_mdt = 0;
  This->header_length = 0;
  if (This->header_data != NULL)
    {
      free (This->header_data);
      This->header_data = NULL;
    }
  return;
}


/****f* lib5250/tn5250_dbuffer_field_yx
 * NAME
 *    tn5250_dbuffer_field_yx
 * SYNOPSIS
 *    ret = tn5250_dbuffer_field_yx (This, y, x);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Field *
tn5250_dbuffer_field_yx (Tn5250DBuffer * This, int y, int x)
{
  Tn5250Field *iter;
  if ((iter = This->field_list) != NULL)
    {
      do
	{
	  if (tn5250_field_hit_test (iter, y, x))
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != This->field_list);
    }
  return NULL;
}


/****f* lib5250/tn5250_dbuffer_first_non_bypass
 * NAME
 *    tn5250_dbuffer_first_non_bypass
 * SYNOPSIS
 *    field = tn5250_dbuffer_first_non_bypass (dbuffer);
 * INPUTS
 *    Tn5250DBuffer *      dbuffer    - The display buffer to use.
 * DESCRIPTION
 *    Returns a pointer to the first non-bypass field in the format table,
 *    or NULL if there are no non-bypass fields.
 *****/
Tn5250Field *
tn5250_dbuffer_first_non_bypass (Tn5250DBuffer * This)
{
  Tn5250Field *iter;
  if ((iter = This->field_list) != NULL)
    {
      do
	{
	  if (!tn5250_field_is_bypass (iter))
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != This->field_list);
    }
  return NULL;
}


/****f* lib5250/tn5250_dbuffer_right
 * NAME
 *    tn5250_dbuffer_right
 * SYNOPSIS
 *    tn5250_dbuffer_right (This, n);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  n          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_right (Tn5250DBuffer * This, int n)
{
  if (This->menubar_count > 0)
    {
      Tn5250Menubar *menubar =
	tn5250_menubar_hit_test (This->menubar_list, This->cx, This->cy);

      if (menubar != NULL)
	{
	  tn5250_menubar_select_next (menubar, &This->cx, &This->cy);
	  ASSERT_VALID (This);
	  return;
	}
    }

  This->cx += n;
  This->cy += (This->cx / This->w);
  This->cx = (This->cx % This->w);
  This->cy = (This->cy % This->h);

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_left
 * NAME
 *    tn5250_dbuffer_left
 * SYNOPSIS
 *    tn5250_dbuffer_left (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_left (Tn5250DBuffer * This)
{
  if (This->menubar_count > 0)
    {
      Tn5250Menubar *menubar =
	tn5250_menubar_hit_test (This->menubar_list, This->cx, This->cy);

      if (menubar != NULL)
	{
	  tn5250_menubar_select_prev (menubar, &This->cx, &This->cy);
	  ASSERT_VALID (This);
	  return;
	}
    }

  This->cx--;
  if (This->cx == -1)
    {
      This->cx = This->w - 1;
      This->cy--;
      if (This->cy == -1)
	{
	  This->cy = This->h - 1;
	}
    }

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_up
 * NAME
 *    tn5250_dbuffer_up
 * SYNOPSIS
 *    tn5250_dbuffer_up (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_up (Tn5250DBuffer * This)
{
  if (This->menubar_count > 0)
    {
      Tn5250Menubar *menubar =
	tn5250_menubar_hit_test (This->menubar_list, This->cx, This->cy);

      if (menubar != NULL)
	{
	  tn5250_menubar_select_prev (menubar, &This->cx, &This->cy);
	  ASSERT_VALID (This);
	  return;
	}
    }

  if (--This->cy == -1)
    This->cy = This->h - 1;

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_down
 * NAME
 *    tn5250_dbuffer_down
 * SYNOPSIS
 *    tn5250_dbuffer_down (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_down (Tn5250DBuffer * This)
{
  if (This->menubar_count > 0)
    {
      Tn5250Menubar *menubar =
	tn5250_menubar_hit_test (This->menubar_list, This->cx, This->cy);

      if (menubar != NULL)
	{
	  tn5250_menubar_select_next (menubar, &This->cx, &This->cy);
	  ASSERT_VALID (This);
	  return;
	}
    }

  if (++This->cy == This->h)
    {
      This->cy = 0;
    }

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_goto_ic
 * NAME
 *    tn5250_dbuffer_goto_ic
 * SYNOPSIS
 *    tn5250_dbuffer_goto_ic (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_goto_ic (Tn5250DBuffer * This)
{
  ASSERT_VALID (This);

  This->cy = This->tcy;
  This->cx = This->tcx;

  ASSERT_VALID (This);
  return;
}

/****f* lib5250/tn5250_dbuffer_addch
 * NAME
 *    tn5250_dbuffer_addch
 * SYNOPSIS
 *    tn5250_dbuffer_addch (This, c);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    unsigned char        c          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_addch (Tn5250DBuffer * This, unsigned char c)
{
  ASSERT_VALID (This);

  This->data[(This->cy * This->w) + This->cx] = c;
  tn5250_dbuffer_right (This, 1);

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_del
 * NAME
 *    tn5250_dbuffer_del
 * SYNOPSIS
 *    tn5250_dbuffer_del (This, fieldid, shiftcount);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  shiftcount - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_del (Tn5250DBuffer * This, int fieldid, int shiftcount)
{
  Tn5250Field *iter, *field;
  int x = This->cx, y = This->cy, fwdx, fwdy, i;

  field = tn5250_field_list_find_by_id (This->field_list, fieldid);
  iter = field;

  while (tn5250_field_is_continued (iter))
    {
      if (tn5250_field_is_continued_last (iter))
	{
	  break;
	}
      iter = iter->next;
      shiftcount =
	shiftcount + tn5250_field_count_right (iter,
					       tn5250_field_start_row (iter),
					       tn5250_field_start_col (iter));
    }

  iter = field;

  for (i = 0; i < shiftcount; i++)
    {
      fwdx = x + 1;
      fwdy = y;
      if (fwdx == This->w)
	{
	  fwdx = 0;
	  fwdy++;
	}

      if (tn5250_field_is_continued (iter)
	  && (!tn5250_field_is_continued_last (iter))
	  && (fwdx > tn5250_field_end_col (iter)))
	{
	  iter = iter->next;
	  fwdx = tn5250_field_start_col (iter);
	  fwdy = tn5250_field_start_row (iter);
	  i--;
	}

      This->data[y * This->w + x] = This->data[fwdy * This->w + fwdx];
      x = fwdx;
      y = fwdy;
    }
  This->data[y * This->w + x] = 0x00;

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_del_this_field_only
 * NAME
 *    tn5250_dbuffer_del_this_field_only
 * SYNOPSIS
 *    tn5250_dbuffer_del_this_field_only (This, shiftcount);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  shiftcount - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_del_this_field_only (Tn5250DBuffer * This, int shiftcount)
{
  /* This function is actually the original tn5250_dbuffer_del() function
   * without the continuous field support.  Generally, tn5250_dbuffer_del()
   * should be used and not this function.  This function should only be
   * when you need to delete a character from a single field within a
   * continuous field group.  The only this should ever be necessary is
   * when deleting a character from a wordwrap field.
   */
  int x = This->cx, y = This->cy, fwdx, fwdy, i;

  for (i = 0; i < shiftcount; i++)
    {
      fwdx = x + 1;
      fwdy = y;
      if (fwdx == This->w)
	{
	  fwdx = 0;
	  fwdy++;
	}
      This->data[y * This->w + x] = This->data[fwdy * This->w + fwdx];
      x = fwdx;
      y = fwdy;
    }
  This->data[y * This->w + x] = TN5250_DISPLAY_WORD_WRAP_SPACE;

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_ins
 * NAME
 *    tn5250_dbuffer_ins
 * SYNOPSIS
 *    tn5250_dbuffer_ins (This, fieldid, c, shiftcount);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  fieldid    -
 *    unsigned char        c          - 
 *    int                  shiftcount - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_ins (Tn5250DBuffer * This, int fieldid,
		    unsigned char c, int shiftcount)
{
  Tn5250Field *iter, *field;
  int x = This->cx, y = This->cy, i;
  unsigned char c2;

  field = tn5250_field_list_find_by_id (This->field_list, fieldid);
  iter = field;

  while (tn5250_field_is_continued (iter))
    {
      if (tn5250_field_is_continued_last (iter))
	{
	  break;
	}
      iter = iter->next;
      shiftcount =
	shiftcount + tn5250_field_count_right (iter,
					       tn5250_field_start_row (iter),
					       tn5250_field_start_col (iter));
    }

  iter = field;

  for (i = 0; i <= shiftcount; i++)
    {
      c2 = This->data[y * This->w + x];
      This->data[y * This->w + x] = c;
      c = c2;
      if (++x == This->w)
	{
	  x = 0;
	  y++;
	}

      if (tn5250_field_is_continued (iter)
	  && (!tn5250_field_is_continued_last (iter))
	  && (x > tn5250_field_end_col (iter)))
	{
	  iter = iter->next;
	  x = tn5250_field_start_col (iter);
	  y = tn5250_field_start_row (iter);
	  i--;
	}
    }

  tn5250_dbuffer_right (This, 1);

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_set_ic
 * NAME
 *    tn5250_dbuffer_set_ic
 * SYNOPSIS
 *    tn5250_dbuffer_set_ic (This, y, x);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_set_ic (Tn5250DBuffer * This, int y, int x)
{
  This->tcx = x;
  This->tcy = y;

  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_roll
 * NAME
 *    tn5250_dbuffer_roll
 * SYNOPSIS
 *    tn5250_dbuffer_roll (This, top, bot, lines);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  top        - 
 *    int                  bot        - 
 *    int                  lines      - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_roll (Tn5250DBuffer * This, int top, int bot, int lines)
{
  int n, c;

  ASSERT_VALID (This);

  if (lines == 0)
    {
      return;
    }

  if (lines < 0)
    {
      /* Move text up */
      for (n = top; n <= bot; n++)
	{
	  if (n + lines >= top)
	    {
	      for (c = 0; c < This->w; c++)
		{
		  This->data[(n + lines) * This->w + c] =
		    This->data[n * This->w + c];
		}
	    }
	}
    }
  else
    {
      for (n = bot; n >= top; n--)
	{
	  if (n + lines <= bot)
	    {
	      for (c = 0; c < This->w; c++)
		{
		  This->data[(n + lines) * This->w + c] =
		    This->data[n * This->w + c];
		}
	    }
	}
    }
  ASSERT_VALID (This);
  return;
}


/****f* lib5250/tn5250_dbuffer_char_at
 * NAME
 *    tn5250_dbuffer_char_at
 * SYNOPSIS
 *    ret = tn5250_dbuffer_char_at (This, y, x);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
unsigned char
tn5250_dbuffer_char_at (Tn5250DBuffer * This, int y, int x)
{
  ASSERT_VALID (This);
  TN5250_ASSERT (y >= 0);
  TN5250_ASSERT (x >= 0);
  TN5250_ASSERT (y < This->h);
  TN5250_ASSERT (x < This->w);
  return This->data[y * This->w + x];
}


/****f* lib5250/tn5250_dbuffer_msg_line
 * NAME
 *    tn5250_dbuffer_msg_line
 * SYNOPSIS
 *    ret = tn5250_dbuffer_msg_line (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    Determine which line we should use as the operator error line.
 *****/
int
tn5250_dbuffer_msg_line (Tn5250DBuffer * This)
{
  int l;
  l = 1000;
  if (This->header_data && (This->header_length >= 4))
    {
      l = This->header_data[3] - 1;
    }
  if ((l > tn5250_dbuffer_height (This) - 1) || (l < 0))
    {
      l = tn5250_dbuffer_height (This) - 1;
    }
  return l;
}


/****f* lib5250/tn5250_dbuffer_prevword
 * NAME
 *    tn5250_dbuffer_prevword
 * SYNOPSIS
 *    tn5250_dbuffer_prevword (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_prevword (Tn5250DBuffer * This)
{
  int state = 0;
  int maxiter;

  TN5250_LOG (("dbuffer_prevword: entered.\n"));

  maxiter = (This->w * This->h);
  TN5250_ASSERT (maxiter > 0);

  while (--maxiter)
    {
      tn5250_dbuffer_left (This);
      switch (state)
	{
	case 0:
	  if (This->data[This->cy * This->w + This->cx] <= 0x40)
	    {
	      state++;
	    }
	  break;
	case 1:
	  if (This->data[This->cy * This->w + This->cx] > 0x40)
	    {
	      state++;
	    }
	  break;
	case 2:
	  if (This->data[This->cy * This->w + This->cx] <= 0x40)
	    {
	      tn5250_dbuffer_right (This, 1);
	      return;
	    }
	  break;
	}
    }
  return;
}


/****f* lib5250/tn5250_dbuffer_nextword
 * NAME
 *    tn5250_dbuffer_nextword
 * SYNOPSIS
 *    tn5250_dbuffer_nextword (This);
 * INPUTS
 *    Tn5250DBuffer *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_nextword (Tn5250DBuffer * This)
{
  int foundblank = 0;
  int maxiter;

  TN5250_LOG (("dbuffer_nextword: entered.\n"));

  maxiter = (This->w * This->h);
  TN5250_ASSERT (maxiter > 0);

  while (--maxiter)
    {
      tn5250_dbuffer_right (This, 1);
      if (This->data[This->cy * This->w + This->cx] <= 0x40)
	{
	  foundblank++;
	}
      if ((foundblank) && (This->data[This->cy * This->w + This->cx] > 0x40))
	{
	  break;
	}
    }

  ASSERT_VALID (This);
  return;
}


/***** lib5250/tn5250_dbuffer_add_window
 * NAME
 *    tn5250_dbuffer_add_window
 * SYNOPSIS
 *    tn5250_dbuffer_add_window (This, window);
 * INPUTS
 *    Tn5250DBuffer *      This       -
 *    Tn5250Window *       window     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_add_window (Tn5250DBuffer * This, Tn5250Window * window)
{
  window->id = This->window_count++;
  window->table = This;
  This->window_list = tn5250_window_list_add (This->window_list, window);

  TN5250_LOG (("adding window: window->id: %d\n", window->id));
  return;
}


/***** lib5250/tn5250_dbuffer_add_scrollbar
 * NAME
 *    tn5250_dbuffer_add_scrollbar
 * SYNOPSIS
 *    tn5250_dbuffer_add_scrollbar (This, scrollbar);
 * INPUTS
 *    Tn5250DBuffer *      This       -
 *    Tn5250Scrollbar *    scrollbar     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_add_scrollbar (Tn5250DBuffer * This,
			      Tn5250Scrollbar * scrollbar)
{
  scrollbar->id = This->scrollbar_count++;
  scrollbar->table = This;
  This->scrollbar_list =
    tn5250_scrollbar_list_add (This->scrollbar_list, scrollbar);

  TN5250_LOG (("adding scrollbar: scrollbar->id: %d\n", scrollbar->id));
  return;
}


/***** lib5250/tn5250_dbuffer_add_menubar
 * NAME
 *    tn5250_dbuffer_add_menubar
 * SYNOPSIS
 *    tn5250_dbuffer_add_menubar (This, menubar);
 * INPUTS
 *    Tn5250DBuffer *      This       -
 *    Tn5250Menubar *      menubar    -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_dbuffer_add_menubar (Tn5250DBuffer * This, Tn5250Menubar * menubar)
{
  menubar->id = This->menubar_count++;
  menubar->table = This;
  This->menubar_list = tn5250_menubar_list_add (This->menubar_list, menubar);

  TN5250_LOG (("adding selection field: menubar->id: %d\n", menubar->id));
  return;
}
