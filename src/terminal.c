/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2000 Jay 'Eraserhead' Felice
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
 * As a special exception, the copyright holder gives permission
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
#include "tn5250-private.h"

void tn5250_terminal_init (Tn5250Terminal *This) {
   (* ((This)->init)) ((This));
}
void tn5250_terminal_term (Tn5250Terminal *This) {
   (* ((This)->term)) ((This));
}
void tn5250_terminal_destroy (Tn5250Terminal *This) {
   (* ((This)->destroy)) ((This));
}
int tn5250_terminal_width (Tn5250Terminal *This) {
   return (* ((This)->width)) ((This));
}
int tn5250_terminal_height (Tn5250Terminal *This) {
   return (* ((This)->height)) ((This));
}
int tn5250_terminal_flags (Tn5250Terminal *This) {
   return (* ((This)->flags)) ((This));
}
void tn5250_terminal_update (Tn5250Terminal *This, Tn5250Display *d) {
   (* ((This)->update)) ((This),(d));
}
void tn5250_terminal_update_indicators (Tn5250Terminal *This, Tn5250Display *d){
   (* ((This)->update_indicators)) ((This),(d));
}
int tn5250_terminal_waitevent (Tn5250Terminal *This) {
   return (* ((This)->waitevent)) ((This));
}
int tn5250_terminal_getkey (Tn5250Terminal *This) {
   return (* ((This)->getkey)) ((This));
}
void tn5250_terminal_beep (Tn5250Terminal *This) {
   return (* ((This)->beep)) ((This));
}
int tn5250_terminal_config (Tn5250Terminal *This, Tn5250Config *conf) {
   return ((This)->config == NULL ? 0 : (* ((This)->config)) ((This),(conf)));
}
