/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997,1998,1999 Michael Madore
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
#include "scs.h"

static void scs2ascii_process34 (int *curpos);
static void scs2ascii_ahpp (int *curpos);
void scs2ascii_transparent ();

unsigned char curchar;
unsigned char nextchar;
int current_line;
int mpp;

int
main ()
{
  Tn5250CharMap *map;
  int new_line = 1;
  int ccp = 1;
  int width;
  int length;
  int cpi;  /* This is unused */
  current_line = 1;
  mpp = 132;

  if ((getenv ("TN5250_CCSIDMAP")) != NULL)
    {
      map = tn5250_char_map_new (getenv ("TN5250_CCSIDMAP"));
    }
  else
    {
      map = tn5250_char_map_new ("37");
    }

  while (!feof (stdin))
    {
      curchar = fgetc (stdin);
      switch (curchar)
	{
	case SCS_TRANSPARENT:
	  {
	    scs2ascii_transparent ();
	    break;
	  }
	case SCS_RFF:
	  {
	    scs_rff ();
	    break;
	  }
	case SCS_NOOP:
	  {
	    scs_noop ();
	    break;
	  }
	case SCS_CR:
	  {
	    scs_cr ();
	    break;
	  }
	case SCS_FF:
	  {
	    scs_ff ();
	    printf ("\f");
	    break;
	  }
	case SCS_NL:
	  {
	    if (!new_line)
	      {
	      }
	    printf ("\n");
	    new_line = scs_nl ();
	    ccp = 1;
	    break;
	  }
	case SCS_RNL:
	  {
	    scs_rnl ();
	    break;
	  }
	case SCS_HT:
	  {
	    scs_ht ();
	    break;
	  }
	case 0x34:
	  {
	    scs2ascii_process34 (&ccp);
	    break;
	  }
	case 0x2B:
	  {
	    scs_process2b (&width, &length, &cpi);
	    break;
	  }
	case 0xFF:
	  {
	    /* This is a hack */
	    /* Don't know where the 0xFF is coming from */
	    break;
	  }
	default:
	  {
	    if (new_line)
	      {
		new_line = 0;
	      }
	    printf ("%c", tn5250_char_map_to_local (map, curchar));
	    ccp++;
	    fprintf (stderr, ">%x\n", curchar);
	  }
	}

    }
  tn5250_char_map_destroy (map);
  return (0);
}

static void
scs2ascii_process34 (int *curpos)
{

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case SCS_AVPP:
      {
	scs_avpp ();
	break;
      }
    case SCS_AHPP:
      {
	scs2ascii_ahpp (curpos);
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

  position = fgetc (stdin);
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
  fprintf (stderr, "AHPP %d\n", position);
}

void
scs2ascii_transparent ()
{

  int bytecount;
  int loop;

  bytecount = fgetc (stdin);
  fprintf (stderr, "TRANSPARENT (%x) = ", bytecount);
  for (loop = 0; loop < bytecount; loop++)
    {
      printf ("%c", fgetc (stdin));
    }
}

/* vi:set sts=3 sw=3: */
