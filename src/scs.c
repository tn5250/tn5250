/* scs.c -- Converts scs to forms useable by scs2ascii, scs2ps, and scs2pdf.
 * Copyright (C) 2000 Michael Madore
 *
 * This file is part of TN5250.
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "scs.h"
#include <stdio.h>


void
scs_ht ()
{
  fprintf (stderr, "HT\n");
}

void
scs_rnl ()
{
  fprintf (stderr, "RNL\n");
}

int
scs_nl ()
{
#ifdef DEBUG
  fprintf (stderr, "NL\n");
#endif
  return (1);
}

void
scs_ff ()
{
#ifdef DEBUG
  fprintf (stderr, "FF\n");
#endif
}

void
scs_cr ()
{
#ifdef DEBUG
  fprintf (stderr, "CR\n");
#endif
}

void
scs_transparent ()
{

  int bytecount;
  int loop;

  bytecount = fgetc (stdin);
  fprintf (stderr, "TRANSPARENT (%x) = ", bytecount);
  for (loop = 0; loop < bytecount; loop++)
    {
      fprintf (stdout, "%c", fgetc (stdin));
    }
}

void
scs_rff ()
{
#ifdef DEBUG
  fprintf (stderr, "RFF\n");
#endif
}

void
scs_noop ()
{
#ifdef DEBUG
  fprintf (stderr, "NOOP\n");
#endif
}

void
scs_process34 (int *curpos)
{
  unsigned char curchar;

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
	scs_ahpp (curpos);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
  return;
}

void
scs_avpp ()
{
  fprintf (stderr, "AVPP %d\n", fgetc (stdin));
}

void
scs_ahpp (int *curpos)
{
  int position;
  int i;

  position = fgetc (stdin);

  if (*curpos > position)
    {
      for (i = 0; i < position; i++)
	{
	  /* Each application needs to process its own way for adding
	   * spaces.  Add one space for each iteration
	   */
	  /*printf(" "); */
	}
    }
  else
    {
      for (i = 0; i < (position - *curpos); i++)
	{
	  /* Each application needs to process its own way for adding
	   * spaces.  Add one space for each iteration
	   */
	  /*printf(" "); */
	}
    }
  *curpos = position;

#ifdef DEBUG
  fprintf (stderr, "AHPP %d\n", position);
#endif
  return;
}

void
scs_process2b (int *pagewidth, int *pagelength, int *cpi)
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case 0xD1:
      {
	scs_processd1 ();
	break;
      }
    case 0xD2:
      {
	scs_processd2 (pagewidth, pagelength, cpi);
	break;
      }
    case 0xD3:
      {
	scs_processd3 ();
	break;
      }
    case 0xC8:
      {
	scs_sgea ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2B command %x\n", curchar);
      }
    }
}

void
scs_processd3 ()
{
  unsigned char curchar;
  unsigned char nextchar;

  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);

  if (nextchar == 0xF6)
    {
      scs_sto (curchar);
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD3 %x %x", curchar, nextchar);
    }
}

void
scs_sto (unsigned char curchar)
{
  int loop;
  unsigned char nextchar;

  fprintf (stderr, "STO = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_sgea ()
{
  fprintf (stderr, "SGEA = %x %x %x\n", fgetc (stdin), fgetc (stdin),
	   fgetc (stdin));
}

void
scs_processd1 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case 0x06:
      {
	scs_process06 ();
	break;
      }
    case 0x07:
      {
	scs_process07 ();
	break;
      }
    case 0x03:
      {
	scs_processd103 ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD1 command %x\n", curchar);
      }
    }
}

void
scs_process06 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  if (curchar == 0x01)
    {
      fprintf (stderr, "GCGID = %x %x %x %x\n", fgetc (stdin), fgetc (stdin),
	       fgetc (stdin), fgetc (stdin));
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD106 command %x\n", curchar);
    }
}

void
scs_process07 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  if (curchar == 0x05)
    {
      fprintf (stderr, "FID = %x %x %x %x %x\n", fgetc (stdin), fgetc (stdin),
	       fgetc (stdin), fgetc (stdin), fgetc (stdin));
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD107 command %x\n", curchar);
    }
}

void
scs_processd103 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  if (curchar == 0x81)
    {
      fprintf (stderr, "SCGL = %x\n", fgetc (stdin));
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD103 command %x\n", curchar);
    }
}

void
scs_processd2 (int *pagewidth, int *pagelength, int *cpi)
{
  unsigned char curchar;
  unsigned char nextchar;

  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);


  switch (nextchar)
    {
    case 0x01:
      {
	scs_stab (curchar);
	break;
      }
    case 0x03:
      {
	scs_jtf (curchar);
	break;
      }
    case 0x0D:
      {
	scs_sjm (curchar);
	break;
      }
    case 0x40:
      {
	scs_spps (pagewidth, pagelength);
	break;
      }
    case 0x48:
      {
	scs_ppm (curchar);
	break;
      }
    case 0x49:
      {
	scs_svm (curchar);
	break;
      }
    case 0x4c:
      {
	scs_spsu (curchar);
	break;
      }
    case 0x85:
      {
	scs_sea (curchar);
	break;
      }
    case 0x11:
      {
	scs_shm (curchar);
	break;
      }
    default:
      {
	switch (curchar)
	  {
	  case 0x03:
	    {
	      scs_process03 (nextchar, curchar);
	      break;
	    }
	  case 0x04:
	    {
	      scs_process04 (nextchar, curchar, cpi);
	      break;
	    }
	  default:
	    {
	      fprintf (stderr, "ERROR: Unknown 0x2BD2 command %x\n", curchar);
	    }
	  }
      }
    }

}

void
scs_stab (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "STAB = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_jtf (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "JTF = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_sjm (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "SJM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_spps (int *pagewidth, int *pagelength)
{
  int width, length;

  width = fgetc (stdin);
  width = (width << 8) + fgetc (stdin);
  *pagewidth = width;

  length = fgetc (stdin);
  length = (length << 8) + fgetc (stdin);
  *pagelength = length;

#ifdef DEBUG
  fprintf (stderr, "SPPS (width = %d) (length = %d)\n", width, length);
#endif
}

void
scs_ppm (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "PPM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}


void
scs_svm (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "SVM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_spsu (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "SPSU (%x) = ", curchar);
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_sea (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "SEA (%x) = ", curchar);
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_shm (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

  fprintf (stderr, "SHM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

void
scs_process03 (unsigned char nextchar, unsigned char curchar)
{
  switch (nextchar)
    {
    case 0x45:
      {
	scs_sic ();
	break;
      }
    case 0x07:
      {
	scs_sil ();
	break;
      }
    case 0x09:
      {
	scs_sls ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD203 command %x\n", curchar);
      }
    }
}

void
scs_sic ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  fprintf (stderr, "SIC = %x\n", curchar);
}

void
scs_sil ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  fprintf (stderr, "SIL = %d", curchar);
}

void
scs_sls ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  fprintf (stderr, "SLS = %d\n", curchar);
}

void
scs_process04 (unsigned char nextchar, unsigned char curchar, int *cpi)
{
  switch (nextchar)
    {
    case 0x15:
      {
	scs_ssld ();
	break;
      }
    case 0x29:
      {
	scs_scs (cpi);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD204 command %x\n", curchar);
      }
    }
}

void
scs_ssld ()
{
  unsigned char curchar;
  unsigned char nextchar;

  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);
  fprintf (stderr, "SSLD = %d %d \n", curchar, nextchar);
}

void
scs_scs (int *cpi)
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  if (curchar == 0x00)
    {
      curchar = fgetc (stdin);

      /* Here we convert characters per inch (CPI) to point size.  In the
       * future we will probably want these to be user definable.
       */
      switch (curchar)
	{
	case 5:
	  {
	    *cpi = 14;
	    break;
	  }
	case 10:
	  {
	    *cpi = 10;
	    break;
	  }
	case 12:
	  {
	    *cpi = 9;
	    break;
	  }
	case 13:
	  {
	    *cpi = 8;
	    break;
	  }
	case 15:
	  {
	    *cpi = 7;
	    break;
	  }
	case 16:
	  {
	    *cpi = 6;
	    break;
	  }
	case 18:
	  {
	    *cpi = 5;
	    break;
	  }
	case 20:
	  {
	    *cpi = 4;
	    break;
	  }
	default:
	  {
	    *cpi = 10;
	    break;
	  }
	}
#ifdef DEBUG
      fprintf (stderr, "SCS = %d", curchar);
#endif
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD20429 command %x\n", curchar);
    }
}
