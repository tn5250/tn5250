/* tn5250 -- an implentation of the 5250 telnet protocol.
 * Copyright (C) 1997 Michael Madore
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef FORMATTABLE_H
#define FORMATTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

   struct _Tn5250Table {

      /* How we keep track of multiple saved format tables */
      struct _Tn5250Table *	next;
      struct _Tn5250Table *     prev;
      unsigned char		id; /* Saved buffer id */

      Tn5250Field /*@null@*/ *field_list;
      int numfields;
      int MasterMDT;
      int message_line;
      int curfield;
   };

   typedef struct _Tn5250Table Tn5250Table;

   extern Tn5250Table *tn5250_table_new(void);
   extern Tn5250Table *tn5250_table_copy(Tn5250Table *table);
   extern void tn5250_table_destroy(Tn5250Table /*@only@*/ * This);

   extern void tn5250_table_add_field(Tn5250Table * This, Tn5250Field * field);
   extern void tn5250_table_clear(Tn5250Table * This);
   extern Tn5250Field *tn5250_table_field_yx (Tn5250Table *This, int y, int x);

#define tn5250_table_field_n(This,n) \
   (tn5250_field_list_find_by_id ((This)->field_list, (n)))

#define tn5250_table_mdt(This) ((This)->MasterMDT)
#define tn5250_table_set_mdt(This) (void) ((This)->MasterMDT = 1)
#define tn5250_table_clear_mdt(This) (void) ((This)->MasterMDT = 0)
#define tn5250_table_set_message_line(This,row) (void) ((This)->message_line = (row))

#define tn5250_table_field_count(This) ((This)->numfields)
#define tn5250_table_mdt(This) ((This)->MasterMDT)
#define tn5250_table_message_line(This) ((This)->message_line)

#ifdef __cplusplus
}

#endif
#endif
