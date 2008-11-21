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

#include "tn5250-private.h"

/*
#define DEBUG
*/


static void scs2ascii_pp (Tn5250SCS * This);
static void scs2ascii_ahpp (int *curpos);
static void scs2ascii_avpp (int *curline);
void scs2ascii_transparent (Tn5250SCS * This);
void scs2ascii_ff (Tn5250SCS * This);
void scs2ascii_nl (Tn5250SCS * This);
void scs2ascii_default (Tn5250SCS * This);
Tn5250SCS *tn5250_scs2ascii_new ();

Tn5250CharMap *map;

int
main ()
{
  Tn5250SCS *scs = NULL;

  if ((getenv ("TN5250_CCSIDMAP")) != NULL)
    {
      map = tn5250_char_map_new (getenv ("TN5250_CCSIDMAP"));
    }
  else
    {
      map = tn5250_char_map_new ("37");
    }


  /* Initialize the scs toolkit */
  scs = tn5250_scs2ascii_new ();

  if (scs == NULL)
    {
      return (-1);
    }

  /* And now set up our callbacks */
  scs->column = 1;
  scs->row = 1;


  /* Turn control over to the SCS toolkit and run the event loop */
  scs_main (scs);

  tn5250_char_map_destroy (map);
  free (scs);
  return (0);
}




/* This initializes the scs callbacks
 */
Tn5250SCS *
tn5250_scs2ascii_new ()
{
  Tn5250SCS *scs = tn5250_scs_new ();

  if (scs == NULL)
    {
      fprintf (stderr,
	       "Unable to allocate memory in tn5250_scs2ascii_new ()!\n");
      return NULL;
    }

  /*
     scs->data = tn5250_new(struct _Tn5250SCSPrivate, 1);
     if (scs->data == NULL) {
     free(scs);
     return;
     }
   */

  /* And now set up our callbacks */
  scs->transparent = scs2ascii_transparent;
  scs->ff = scs2ascii_ff;
  scs->rff = scs2ascii_ff;
  scs->nl = scs2ascii_nl;
  scs->rnl = scs2ascii_nl;
  scs->pp = scs2ascii_pp;
  scs->scsdefault = scs2ascii_default;
  return scs;
}



void
scs2ascii_default (Tn5250SCS * This)
{
#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_default()\n");
#endif
#endif
  printf ("%c", tn5250_char_map_to_local (map, This->curchar));
  This->column++;
#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "%c (%x)\n",
	   tn5250_char_map_to_local (map, This->curchar), This->curchar);
#endif
#endif
  return;
}

void
scs2ascii_ff (Tn5250SCS * This)
{
#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_ff()\n");
#endif
#endif
  scs_ff (This);
  printf ("\f");
  This->row = 1;
  return;
}

void
scs2ascii_nl (Tn5250SCS * This)
{
#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_nl()\n");
#endif
#endif
  scs_nl (This);
  printf ("\n");
  This->column = 1;
  This->row = This->row + 1;
  return;
}

static void
scs2ascii_pp (Tn5250SCS * This)
{
  unsigned char curchar;

#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_pp()\n");
#endif
#endif
  curchar = fgetc (stdin);
  switch (curchar)
    {
    case SCS_AVPP:
      {
	scs2ascii_avpp (&(This->row));
	break;
      }
    case SCS_AHPP:
      {
	scs2ascii_ahpp (&(This->column));
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
}

static void
scs2ascii_ahpp (int *curpos)
{
  int position;
  int loop;

#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_ahpp()\n");
#endif
#endif
  position = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "AHPP %d (current position: %d)\n", position, *curpos);
#endif

  if (*curpos > position)
    {
      printf ("\r");
      for (loop = 0; loop < position; loop++)
	{
	  printf (" ");
	}
    }
  else
    {
      for (loop = 0; loop < position - *curpos; loop++)
	{
	  printf (" ");
	}
    }
  *curpos = position;
  return;
}

void
scs2ascii_transparent (Tn5250SCS * This)
{

  int bytecount;
  int loop;

#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_transparent()\n");
#endif
#endif
  bytecount = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "TRANSPARENT (%x) = ", bytecount);
#endif

  for (loop = 0; loop < bytecount; loop++)
    {
      printf ("%c", fgetc (stdin));
    }
}

static void
scs2ascii_avpp (int *curline)
{
  int line;

#ifdef DEBUG
#ifdef VERBOSE
  fprintf (stderr, "doing scs2ascii_avpp()\n");
#endif
#endif
  line = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "AVPP %d\n", line);
#endif

  if (*curline > line)
    {
      printf ("\f");
      *curline = 1;
    }

  while (*curline < line)
    {
      printf ("\n");
      (*curline)++;
    }

}

/* vi:set sts=3 sw=3: */
