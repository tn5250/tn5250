/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2000 Jay 'Eraserhead' Felice
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

void
tn5250_terminal_init (Tn5250Terminal * This)
{
  (*((This)->init)) ((This));
}

void
tn5250_terminal_term (Tn5250Terminal * This)
{
  (*((This)->term)) ((This));
}

void
tn5250_terminal_destroy (Tn5250Terminal * This)
{
  (*((This)->destroy)) ((This));
}

int
tn5250_terminal_width (Tn5250Terminal * This)
{
  return (*((This)->width)) ((This));
}

int
tn5250_terminal_height (Tn5250Terminal * This)
{
  return (*((This)->height)) ((This));
}

int
tn5250_terminal_flags (Tn5250Terminal * This)
{
  return (*((This)->flags)) ((This));
}

void
tn5250_terminal_update (Tn5250Terminal * This, Tn5250Display * d)
{
  (*((This)->update)) ((This), (d));
}

void
tn5250_terminal_update_indicators (Tn5250Terminal * This, Tn5250Display * d)
{
  (*((This)->update_indicators)) ((This), (d));
}

int
tn5250_terminal_waitevent (Tn5250Terminal * This)
{
  return (*((This)->waitevent)) ((This));
}

int
tn5250_terminal_getkey (Tn5250Terminal * This)
{
  return (*((This)->getkey)) ((This));
}

void
tn5250_terminal_beep (Tn5250Terminal * This)
{
  (*((This)->beep)) ((This));
}

int
tn5250_terminal_enhanced (Tn5250Terminal * This)
{
  return (*((This)->enhanced)) ((This));
}

int
tn5250_terminal_config (Tn5250Terminal * This, Tn5250Config * conf)
{
  return ((This)->config == NULL ? 0 : (*((This)->config)) ((This), (conf)));
}

void
tn5250_terminal_create_window (Tn5250Terminal * This, Tn5250Display * d,
			       struct _Tn5250Window * w)
{
  (*((This)->create_window)) ((This), (d), (w));
}

void
tn5250_terminal_destroy_window (Tn5250Terminal * This, Tn5250Display * d)
{
  (*((This)->destroy_window)) ((This), (d));
}

void
tn5250_terminal_create_scrollbar (Tn5250Terminal * This, Tn5250Display * d)
{
  (*((This)->create_scrollbar)) ((This), (d));
}

void
tn5250_terminal_destroy_scrollbar (Tn5250Terminal * This, Tn5250Display * d)
{
  (*((This)->destroy_scrollbar)) ((This), (d));
}
