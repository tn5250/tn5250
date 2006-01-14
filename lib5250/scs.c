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

#include "tn5250-private.h"

/*
#define DEBUG
#define VERBOSE
*/


/* Device control */
void scs_sic (Tn5250SCS * This);
void scs_sea (Tn5250SCS * This);
void scs_noop (Tn5250SCS * This);
void scs_transparent (Tn5250SCS * This);
void scs_spsu (Tn5250SCS * This);

/* Page control */
void scs_ppm (Tn5250SCS * This);
void scs_spps (Tn5250SCS * This);
void scs_shf (Tn5250SCS * This);
void scs_svf (Tn5250SCS * This);
void scs_ff (Tn5250SCS * This);
void scs_rff (Tn5250SCS * This);
void scs_sto (Tn5250SCS * This);
void scs_shm (Tn5250SCS * This);
void scs_svm (Tn5250SCS * This);
void scs_sffc (Tn5250SCS * This);

/* Font controls */
void scs_scgl (Tn5250SCS * This);
void scs_scg (Tn5250SCS * This);
void scs_sfg (Tn5250SCS * This);
void scs_scd (int *cpi);

/* Cursor control */
void scs_pp (Tn5250SCS * This);
void scs_rdpp (Tn5250SCS * This);
void scs_ahpp (Tn5250SCS * This);
void scs_avpp (Tn5250SCS * This);
void scs_rrpp (Tn5250SCS * This);
void scs_sbs (Tn5250SCS * This);
void scs_sps (Tn5250SCS * This);
void scs_nl (Tn5250SCS * This);
void scs_irs (Tn5250SCS * This);
void scs_rnl (Tn5250SCS * This);
void scs_irt (Tn5250SCS * This);
void scs_stab (Tn5250SCS * This);
void scs_ht (Tn5250SCS * This);
void scs_it (Tn5250SCS * This);
void scs_sil (Tn5250SCS * This);
void scs_lf (Tn5250SCS * This);
void scs_cr (Tn5250SCS * This);
void scs_ssld (Tn5250SCS * This);
void scs_sls (Tn5250SCS * This);

/* Generation controls */
void scs_sgea (Tn5250SCS * This);

void scs_process2b (Tn5250SCS * This);
void scs_processd2 (Tn5250SCS * This);
void scs_process03 (unsigned char nextchar, unsigned char curchar);
void scs_scs (int *cpi);
void scs_process04 (unsigned char nextchar, unsigned char curchar, int *cpi);
void scs_processd1 ();
void scs_process06 ();
void scs_process07 ();
void scs_processd103 ();
void scs_jtf (unsigned char curchar);
void scs_sjm (unsigned char curchar);
void scs_processd3 (Tn5250SCS * This);
void scs_main (Tn5250SCS * This);
void scs_init (Tn5250SCS * This);
void scs_default (Tn5250SCS * This);


/* Set Initial Conditions (SIC).  This is part of Device Control.  SIC
 * has a one byte parameter that follows it with the following meanings:
 * 1 - word processing initialization
 * 255 - data processing initialization (default)
 * all others - invalid
 *
 * According to the manual, this control is ignored by all printers.
 */
void
scs_sic (Tn5250SCS * This)
{
  unsigned char curchar;

  curchar = fgetc (stdin);

  if (curchar != 1 && curchar != 255)
    {
      fprintf (stderr, "Invalid SIC parameter (SIC = %x)\n", curchar);
    }
  else
    {
#ifdef DEBUG
      fprintf (stderr, "SIC = %x", curchar);
#ifdef VERBOSE
      fprintf (stderr, "\tInitializing for data processing");
#endif
      fprintf (stderr, "\n");
#endif
    }
  return;
}


/* Set Exception Action (SEA).  This is part of Device Control.  SEA
 * introduces a sequence of byte pairs.  The byte pairs have a length
 * of (curchar-2) bytes.  The first byte of each pair is the exception
 * class.  The second is the action.
 *
 * The exception class bytes has the following meanings:
 * 0 - all exception classes
 * 1-4 - classes 1-4 respectively
 * all others - invalid
 *
 * The action may be:
 * 0 - accept.  On the printer this is the same is ignore (1).
 * 1 - ignore
 * 2 - terminate
 * 3 - suspend
 *
 * If we get an action of 2 or 3 we should probably exit(), but for
 * now do nothing.
 */
void
scs_sea (Tn5250SCS * This)
{
  unsigned char exception;
  unsigned char action;
  int loop;

  for (loop = 0; loop < This->curchar - 2; loop++)
    {
      exception = fgetc (stdin);
      if (exception > 4)
	{
	  fprintf (stderr, "Invalid exception class (%d)\n", exception);
	}
      else
	{
#ifdef DEBUG
	  fprintf (stderr, "SEA (length %x) = %d", This->curchar, exception);
#endif
	}
      action = fgetc (stdin);
      if (action > 3)
	{
	  fprintf (stderr,
		   "Invalid action (exception class: %d, action %d)\n",
		   exception, action);
	}
      else
	{
#ifdef DEBUG
	  fprintf (stderr, " %d", action);
#ifdef VERBOSE
	  switch (action)
	    {
	    case 0:
	      {
		if (exception == 0)
		  {
		    fprintf (stderr,
			     "\tUsing action ACCEPT for exception class %d (all exception classes)",
			     exception);
		  }
		else
		  {
		    fprintf (stderr,
			     "\tUsing action ACCEPT for exception class %d",
			     exception);
		  }
		break;
	      }
	    case 1:
	      {
		if (exception == 0)
		  {
		    fprintf (stderr,
			     "\tUsing action IGNORE for exception class %d (all exception classes)",
			     exception);
		  }
		else
		  {
		    fprintf (stderr,
			     "\tUsing action IGNORE for exception class %d",
			     exception);
		  }
		break;
	      }
	    case 2:
	      {
		if (exception == 0)
		  {
		    fprintf (stderr,
			     "\tUsing action TERMINATE for exception class %d (all exception classes)",
			     exception);
		  }
		else
		  {
		    fprintf (stderr,
			     "\tUsing action TERMINATE for exception class %d",
			     exception);
		  }
		break;
	      }
	    case 3:
	      {
		if (exception == 0)
		  {
		    fprintf (stderr,
			     "\tUsing action SUSPEND for exception class %d (all exception classes)",
			     exception);
		  }
		else
		  {
		    fprintf (stderr,
			     "\tUsing action SUSPEND for exception class %d",
			     exception);
		  }
		break;
	      }
	    }
#endif
	  fprintf (stderr, "\n");
#endif
	}
      loop++;
    }
  return;
}


/* Null (NUL).  This is part of Device Control.
 * Don't do anything
 */
void
scs_noop (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "NOOP\n");
#endif
  return;
}


/* ASCII Transparency (ATRN).  This is part of Device Control.
 * Only used to send PCL codes to an ascii printer.
 */
void
scs_transparent (Tn5250SCS * This)
{
  int bytecount;
  int loop;

  bytecount = fgetc (stdin);
  fprintf (stderr, "TRANSPARENT (%x) = ", bytecount);
  for (loop = 0; loop < bytecount; loop++)
    {
      fprintf (stderr, "%c", fgetc (stdin));
    }
  return;
}


/* Set Print Setup (SPSU).  This is part of Device Control.  SPSU
 * selects the paper source tray.
 *
 * The documentation is a little weird on this on.  But we ignore it
 * anyway, so who cares?  Usually the host sends us 03 which means if
 * the printer is currently set to manual feed, use tray 1.
 */
void
scs_spsu (Tn5250SCS * This)
{
  unsigned char trayparm;
  unsigned char nextchar;
  int loop;

  nextchar = fgetc (stdin);
  trayparm = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SPSU (%x) = %x%x", This->curchar, nextchar, trayparm);
#endif
  for (loop = 2; loop < This->curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, " %x", nextchar);
#endif
    }
#ifdef DEBUG
#ifdef VERBOSE
  if (trayparm < 4)
    {
      switch (trayparm)
	{
	case 0:
	  {
	    fprintf (stderr, "\tPaper source tray left unchanged");
	    break;
	  }
	case 1:
	  {
	    fprintf (stderr, "\tPaper source tray set to manual");
	    break;
	  }
	case 2:
	  {
	    fprintf (stderr, "\tPaper source tray set to tray 1");
	    break;
	  }
	case 3:
	  {
	    fprintf (stderr, "\tPaper source tray set to tray 1");
	    break;
	  }
	}
    }
#endif
  fprintf (stderr, "\n");
#endif
  return;
}


/* Page Presentation Media (PPM).  This is part of Page Controls.
 * This also selects paper source - in seeming conflict with SPSU.
 */
void
scs_ppm (Tn5250SCS * This)
{
  unsigned char formscontrol, sourcedrawer, destdraweroffset;
  unsigned char destdrawer, quality, duplex;
  unsigned char nextchar;

#ifdef DEBUG
  fprintf (stderr, "Begin Page Presentation Media (PPM)\n");
  fprintf (stderr, "Length of PPM parameters: %d\n", This->curchar);
#endif
  nextchar = fgetc (stdin);
  nextchar = fgetc (stdin);
  formscontrol = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "\tForms control = %x\n", formscontrol);
#endif
  if (This->curchar > 5)
    {
      sourcedrawer = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, "\tSource drawer = %x\n", sourcedrawer);
#endif
    }
  if (This->curchar > 6)
    {
      destdraweroffset = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, "\tDestination drawer offset = %x\n",
	       destdraweroffset);
#endif
    }
  if (This->curchar > 7)
    {
      destdrawer = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, "\tDestination drawer = %x\n", destdrawer);
#endif
    }
  if (This->curchar > 8)
    {
      quality = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, "\tQuality = %x\n", quality);
#endif
    }
  if (This->curchar > 9)
    {
      duplex = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, "\tDuplex = %x\n", duplex);
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "End Page Presentation Media (PPM)\n");
#endif
  return;
}


/* Set Presentation Page Size (SPPS).  This is part of Page Controls.
 * Sets the presetation surface width and optionally depth in units of
 * 1440ths of an inch.  Valid values are 0 - 32767.  A value of 0 results
 * in no change.
 *
 * Definately look at the IBM docs for this one.  Particularly note that
 * the presentation size can be different (even larger) than the actual
 * page size.
 */
void
scs_spps (Tn5250SCS * This)
{
  int width, length;

  width = fgetc (stdin);
  width = (width << 8) + fgetc (stdin);
  This->pagewidth = width;

  length = fgetc (stdin);
  length = (length << 8) + fgetc (stdin);
  This->pagelength = length;

#ifdef DEBUG
  fprintf (stderr, "SPPS (width = %d) (length = %d)", width, length);
#ifdef VERBOSE
  fprintf (stderr, "\tPrintable region is %d by %d inches",
	   (width / 1440), (length / 1440));
#endif
  fprintf (stderr, "\n");
#endif
  return;
}


/* Set Horizontal Format (SHF).  This is part of Page Controls.
 * Specifies the presentation surface width.
 */
void
scs_shf (Tn5250SCS * This)
{
  unsigned char shf1, shf2;

  shf1 = fgetc (stdin);
  shf2 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SHF = %x %x\n", shf1, shf2);
#endif
  return;
}


/* Set Vertical Format (SVF).  This is part of Page Controls.
 * Specifies the presentation surface depth.
 */
void
scs_svf (Tn5250SCS * This)
{
  unsigned char svf1, svf2;

  svf1 = fgetc (stdin);
  svf2 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SVF = %x %x\n", svf1, svf2);
#endif
  return;
}


/* Form Feed/page end (FF/PE).  This is part of Page Controls.
 * Prints the current page.
 */
void
scs_ff (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "FF\n");
#endif
  return;
}


/* Required Form Feed/page end (RFF/RPE).  This is part of Page Controls.
 * Prints the current page like form feed, but also restores the indent
 * level to the left margin
 */
void
scs_rff (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "RFF\n");
#endif
  return;
}


/* Set Text Orientation (STO).  This is part of Page Controls.
 * Sets the orientation of characters on a page.  It has two paramters:
 *
 * 2-byte parameter for character rotation.  This is always ignored.
 * 2-byte parameter for page rotation with the following meanings:
 *
 *   0000 - portrait
 *   2D00 - rotate clockwise 270 degrees
 *   5A00 - rotate clockwise 180 degrees
 *   8700 - rotate clockwise 90 degrees
 *   FFFE - select COR mode
 *   FFFF - calculate based on SPPS (default)
 *   all others - invalid
 */
void
scs_sto (Tn5250SCS * This)
{
  unsigned char charrot1;
  unsigned char charrot2;
  unsigned char pagerot1;
  unsigned char pagerot2;

#ifdef DEBUG
  fprintf (stderr, "STO = ");
#endif
  charrot1 = fgetc (stdin);
  charrot2 = fgetc (stdin);
  pagerot1 = fgetc (stdin);
  pagerot2 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "%x%x %x%x", charrot1, charrot2, pagerot1, pagerot2);
#ifdef VERBOSE
  switch (pagerot1)
    {
    case 0x00:
      {
	fprintf (stderr, "\tPrinting normal portrait");
	break;
      }
    case 0x2D:
      {
	fprintf (stderr, "\tPrinting landscape left");
	break;
      }
    case 0x5A:
      {
	fprintf (stderr, "\tPrinting portrait upside down");
	break;
      }
    case 0x87:
      {
	fprintf (stderr, "\tPrinting landscape right");
	break;
      }
    case 0xFF:
      {
	if (pagerot2 == 0xFE)
	  {
	    fprintf (stderr, "\tSelected COR mode");
	  }
	else
	  {
	    fprintf (stderr,
		     "\tSetting text orientation based on SPPS command");
	  }
      }
    }
#endif
  fprintf (stderr, "\n");
#endif
  if (pagerot1 != 0xFF && pagerot2 != 0xFF)
    {
      fprintf (stderr, "Unhandled page rotation!!\n");
    }
  return;
}


/* Set Horizontal Margins (SHM).  This is part of Page Controls.
 * Sets the left margin and optionally the right margin in terms
 * of 1440ths of an inch from the left and optionally right edges
 * of the paper.  Valid values are 0 - 32767.  A value of 0 means
 * to leave the margins unchanged.
 */
void
scs_shm (Tn5250SCS * This)
{
  unsigned char left1;
  unsigned char left2;
  unsigned char right1;
  unsigned char right2;

  left1 = fgetc (stdin);
  left2 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SHM = %d%d", left1, left2);
#endif
  if (This->curchar > 5)
    {
      right1 = fgetc (stdin);
      right2 = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, " %d%d", right1, right2);
#endif
#ifdef VERBOSE
      fprintf (stderr,
	       "\tSetting margins to %d%d 1440ths of an inch in from the left\n",
	       left1, left2);
      fprintf (stderr,
	       "\t\tedge and %d%d 1440ths of an inch in from the right edge",
	       right1, right2);
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "\n");
#endif
  return;
}


/* Set Vertical Margins (SVM).  This is part of Page Controls.
 * Sets the top margin and optionally the bottom margin in terms
 * of 1440ths of an inch from the top and optionally bottom edges
 * of the paper.  Valid values are 0 - 32767.  A value of 0 means
 * to leave the margins unchanged.
 */
void
scs_svm (Tn5250SCS * This)
{
  unsigned char top1;
  unsigned char top2;
  unsigned char bottom1;
  unsigned char bottom2;

  top1 = fgetc (stdin);
  top2 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SVM = %d%d", top1, top2);
#endif
  if (This->curchar > 5)
    {
      bottom1 = fgetc (stdin);
      bottom2 = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, " %d%d", bottom1, bottom2);
#ifdef VERBOSE
      fprintf (stderr,
	       "\tSetting margins to %d%d 1440ths of an inch in from the top\n",
	       top1, top2);
      fprintf (stderr,
	       "\t\tedge and %d%d 1440ths of an inch in from the bottom edge",
	       bottom1, bottom2);
#endif
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "\n");
#endif
  return;
}


/* Set Form Feed Control (SFFC).  This is part of Page Controls.
 * Specifies the number of form feeds to be issued before a page is
 * printed.  It has one 1-byte parameter.  The default is 0x01. Valid
 * values are 0x00 - 0xFF.  0x00 means no change.
 */
void
scs_sffc (Tn5250SCS * This)
{
  unsigned char nextchar;

  nextchar = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SFFC=%x\n", nextchar);
#ifdef VERBOSE
  fprintf (stderr,
	   "\tRequiring %x form feeds to be issued before printing page\n",
	   nextchar);
#endif
  fprintf (stderr, "\n");
#endif
  return;
}


/* Set CGCS through Local ID (SCGL).  This is part of Font Controls.
 * This has a 1-byte parameter that gives the local ID.  The default
 * is 0xFF
 */
void
scs_scgl (Tn5250SCS * This)
{
  unsigned char nextchar;

  nextchar = fgetc (stdin);
  if (nextchar != 0xFF)
    {
      fprintf (stderr, "SCGL = %x\n", nextchar);
    }
  else
    {
#ifdef DEBUG
      fprintf (stderr, "SCGL = %x\n", nextchar);
#endif
    }
  return;
}


/* Set GCGID through GCID (SCG).  This is part of Font Controls.
 * Sets the code page.  This has two 1-byte parameters that give the
 * the graphic character set global ID (which is ignored) and the code
 * page global ID.  The second parameter selects the code page.
 * 
 * See the documentation for a complete table of valid values.
 */
void
scs_scg (Tn5250SCS * This)
{
  unsigned char gcgid;
  unsigned char cpgid;

  gcgid = fgetc (stdin);
  cpgid = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "GCGID = %d, CPGID = %d\n", gcgid, cpgid);
#endif
  return;
}


/* Set FID through GFID (SFG).  This is part of Font Controls.
 * Selects the font.  This has two 2-byte parameters and one 1-byte
 * parameter.
 */
void
scs_sfg (Tn5250SCS * This)
{
  unsigned char globalfontid1;
  unsigned char globalfontid2;
  unsigned char fontwidth1;
  unsigned char fontwidth2;
  unsigned char fontattribute;

  globalfontid1 = fgetc (stdin);
  globalfontid2 = fgetc (stdin);
  fontwidth1 = fgetc (stdin);
  fontwidth2 = fgetc (stdin);
  fontattribute = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "FID = %x%x %x%x %x\n", globalfontid1, globalfontid2,
	   fontwidth1, fontwidth2, fontattribute);
#endif
  return;
}


/* Set Character Distance (SCD).  This is part of Font Controls.
 * This is also known and Set Print Density (SPD).  This is the SCS
 * alternative method of selecting fonts.  This has one 2-byte parameter.
 * The following values are supported:
 *
 * 0x0000 - unchanged
 * 0x0005 - courier 5
 * 0x000A - courier 10
 * 0x000B - courier 12 proportionally spaced
 * 0x000C - courier 12
 * 0x000F - courier 15
 * 0x00FF - use font from operation panel
 * all others - invalid
 */
void
scs_scd (int *cpi)
{
  unsigned char chardist1;
  unsigned char chardist2;

  chardist1 = fgetc (stdin);
  chardist2 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SCD = %x%x", chardist1, chardist2);
#endif
  /* Here we convert characters per inch (CPI) to point size.  In the
   * future we will probably want these to be user definable.
   */
  switch (chardist2)
    {
    case 0x05:
      {
	*cpi = 14;
#ifdef DEBUG
#ifdef VERBOSE
	fprintf (stderr, "\tFont set to Courier %d", *cpi);
#endif
#endif
	break;
      }
    case 0x0A:
      {
	*cpi = 10;
#ifdef DEBUG
#ifdef VERBOSE
	fprintf (stderr, "\tFont set to Courier %d", *cpi);
#endif
#endif
	break;
      }
    case 0x0B:
      {
	*cpi = 9;
#ifdef DEBUG
#ifdef VERBOSE
	fprintf (stderr, "\tFont set to Courier %d", *cpi);
#endif
#endif
	break;
      }
    case 0x0C:
      {
	*cpi = 10;
#ifdef DEBUG
#ifdef VERBOSE
	fprintf (stderr, "\tFont set to Courier %d", *cpi);
#endif
#endif
	break;
      }
    case 0x0F:
      {
	*cpi = 7;
#ifdef DEBUG
#ifdef VERBOSE
	fprintf (stderr, "\tFont set to Courier %d", *cpi);
#endif
#endif
	break;
      }
    default:
      {
	*cpi = 10;
#ifdef DEBUG
#ifdef VERBOSE
	fprintf (stderr, "\tFont set to Courier %d", *cpi);
#endif
#endif
	break;
      }
    }
#ifdef DEBUG
  fprintf (stderr, "\n");
#endif
  return;
}


/* Presentation Position (PP).  This is part of Cursor Controls.
 * Moves the current cursor position based on the given parameters.
 */
void
scs_pp (Tn5250SCS * This)
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case SCS_RDPP:
      {
	scs_rdpp (This);
	break;
      }
    case SCS_AVPP:
      {
	scs_avpp (This);
	break;
      }
    case SCS_AHPP:
      {
	scs_ahpp (This);
	break;
      }
    case SCS_RRPP:
      {
	scs_rrpp (This);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
  return;
}


/* Relative move Down (RDPP).  This is part of Cursor Controls.
 */
void
scs_rdpp (Tn5250SCS * This)
{
  unsigned char rdpp;

  rdpp = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "RDPP %d\n", rdpp);
#endif
  return;
}


/* Absolute Horizontal move (AHPP).  This is part of Cursor Controls.
 */
void
scs_ahpp (Tn5250SCS * This)
{
  int position;
  int i;

  position = fgetc (stdin);

  if (This->row > position)
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
      for (i = 0; i < (position - This->row); i++)
	{
	  /* Each application needs to process its own way for adding
	   * spaces.  Add one space for each iteration
	   */
	  /*printf(" "); */
	}
    }
  This->row = position;

#ifdef DEBUG
  fprintf (stderr, "AHPP %d\n", position);
#endif
  return;
}


/* Absolute Vertical move (AVPP).  This is part of Cursor Controls.
 */
void
scs_avpp (Tn5250SCS * This)
{
  unsigned char nextchar;

  nextchar = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "AVPP %d\n", nextchar);
#endif
  return;
}


/* Relative move Right (RRPP).  This is part of Cursor Controls.
 */
void
scs_rrpp (Tn5250SCS * This)
{
  unsigned char nextchar;

  nextchar = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "RRPP %d\n", nextchar);
#endif
  return;
}


/* Subscript (SBS).  This is part of Cursor Controls.
 */
void
scs_sbs (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "SBS\n");
#endif
  return;
}


/* Superscript (SPS).  This is part of Cursor Controls.
 */
void
scs_sps (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "SPS\n");
#endif
  return;
}


/* New Line (NL).  This is part of Cursor Controls.
 */
void
scs_nl (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "NL\n");
#endif
  return;
}


/* Interchange Record Separator (IRS).  This is part of Cursor Controls.
 * This is the same as a new line control.
 */
void
scs_irs (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "IRS\n");
#endif
  return;
}


/* Required New Line (RNL).  This is part of Cursor Controls.
 */
void
scs_rnl (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "RNL\n");
#endif
  return;
}


/* Index Return (IRT).  This is part of Cursor Controls.
 * Processed as a required new line.
 */
void
scs_irt (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "IRT\n");
#endif
  return;
}


/* Set horizontal Tab stops (STAB).  This is part of Cursor Controls.
 * Set tab stops.  This has two parameters:  one 1-byte parameter that
 * indicates if the tabs are floating or fixed and one variable sized
 * parameter that defines the tab stops.
 */
void
scs_stab (Tn5250SCS * This)
{
  unsigned char nextchar;
  int loop;

#ifdef DEBUG
  fprintf (stderr, "STAB = ");
#endif
  for (loop = 0; loop < This->curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, " %x", nextchar);
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "\n");
#endif
  return;
}


/* Horizontal Tab (HT).  This is part of Cursor Controls.
 * Moves the cursor to the right to the next tab stop.
 */
void
scs_ht (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "HT\n");
#endif
  return;
}


/* Indent Tab (IT).  This is part of Cursor Controls.
 * This is processed the same as horizontal tab except that the left
 * margin is moved to the nex tab stop.
 */
void
scs_it (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "IT\n");
#endif
  return;
}


/* Set Indent Level (SIL).  This is part of Cursor Controls.
 */
void
scs_sil (Tn5250SCS * This)
{
  unsigned char curchar;

  curchar = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SIL = %d", curchar);
#endif
  return;
}


/* Line Feed/index (LF).  This is part of Cursor Controls.
 * Moves the vertical position down one line increment.  The horizontal
 * position is unchanged.
 */
void
scs_lf (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "LF\n");
#endif
  return;
}


/* Carriage Return (CR).  This is part of Cursor Controls.
 * Moves the horizontal position to the left margin.  The vertical
 * position is unchanged.
 */
void
scs_cr (Tn5250SCS * This)
{
#ifdef DEBUG
  fprintf (stderr, "CR\n");
#endif
  return;
}

void
scs_process2b (Tn5250SCS * This)
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
	scs_processd2 (This);
	break;
      }
    case 0xD3:
      {
	scs_processd3 (This);
	break;
      }
    case 0xC8:
      {
	scs_sgea (This);
	break;
      }
    case 0xC1:
      {
	scs_shf (This);
	break;
      }
    case 0xC2:
      {
	scs_svf (This);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2B command %x\n", curchar);
      }
    }
  return;
}

void
scs_processd3 (Tn5250SCS * This)
{
  unsigned char curchar;
  unsigned char nextchar;

  curchar = fgetc (stdin);
  This->curchar = curchar;
  nextchar = fgetc (stdin);

  if (nextchar == 0xF6)
    {
      scs_sto (This);
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD3 %x %x", curchar, nextchar);
    }
  return;
}

void
scs_sgea (Tn5250SCS * This)
{
  unsigned char sgea1, sgea2, sgea3;

  sgea1 = fgetc (stdin);
  sgea2 = fgetc (stdin);
  sgea3 = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SGEA = %x %x %x\n", sgea1, sgea2, sgea3);
#endif
  return;
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
  return;
}

void
scs_process06 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  if (curchar == 0x01)
    {
      scs_scg (NULL);
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD106 command %x\n", curchar);
    }
  return;
}

void
scs_process07 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  if (curchar == 0x05)
    {
      scs_sfg (NULL);
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD107 command %x\n", curchar);
    }
  return;
}

void
scs_processd103 ()
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case 0x81:
      {
	scs_scgl (NULL);
	break;
      }
    case 0x87:
      {
	scs_sffc (NULL);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD103 command %x\n", curchar);
	break;
      }
    }
  return;
}

void
scs_processd2 (Tn5250SCS * This)
{
  unsigned char curchar;
  unsigned char nextchar;

  curchar = fgetc (stdin);
  This->curchar = curchar;
  nextchar = fgetc (stdin);


  switch (nextchar)
    {
    case 0x01:
      {
	scs_stab (This);
	break;
      }
    case 0x03:
      {
	scs_jtf (This->curchar);
	break;
      }
    case 0x0D:
      {
	scs_sjm (This->curchar);
	break;
      }
    case 0x40:
      {
	scs_spps (This);
	break;
      }
    case 0x48:
      {
	scs_ppm (This);
	break;
      }
    case 0x49:
      {
	scs_svm (This);
	break;
      }
    case 0x4c:
      {
	scs_spsu (This);
	break;
      }
    case 0x85:
      {
	scs_sea (This);
	break;
      }
    case 0x11:
      {
	scs_shm (This);
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
	      scs_process04 (nextchar, curchar, &(This->cpi));
	      break;
	    }
	  default:
	    {
	      fprintf (stderr, "ERROR: Unknown 0x2BD2 command %x\n", curchar);
	    }
	  }
      }
    }
  return;
}

void
scs_jtf (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

#ifdef DEBUG
  fprintf (stderr, "JTF = ");
#endif

  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, " %x", nextchar);
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "\n");
#endif
  return;
}

void
scs_sjm (unsigned char curchar)
{
  unsigned char nextchar;
  int loop;

#ifdef DEBUG
  fprintf (stderr, "SJM = ");
#endif

  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
#ifdef DEBUG
      fprintf (stderr, " %x", nextchar);
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "\n");
#endif
  return;
}

void
scs_process03 (unsigned char nextchar, unsigned char curchar)
{
  switch (nextchar)
    {
    case 0x45:
      {
	scs_sic (NULL);
	break;
      }
    case 0x07:
      {
	scs_sil (NULL);
	break;
      }
    case 0x09:
      {
	scs_sls (NULL);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD203 command %x\n", curchar);
      }
    }
  return;
}

void
scs_sls (Tn5250SCS * This)
{
  unsigned char curchar;

  curchar = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SLS = %d\n", curchar);
#endif
  return;
}

void
scs_process04 (unsigned char nextchar, unsigned char curchar, int *cpi)
{
  switch (nextchar)
    {
    case 0x15:
      {
	scs_ssld (NULL);
	break;
      }
    case 0x29:
      {
	scs_scd (cpi);
	/*scs_scs (cpi); */
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD204 command %x\n", curchar);
      }
    }
  return;
}

void
scs_ssld (Tn5250SCS * This)
{
  unsigned char curchar;
  unsigned char nextchar;

  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);
#ifdef DEBUG
  fprintf (stderr, "SSLD = %d %d \n", curchar, nextchar);
#endif
  return;
}

/* This function is obsolete - scs_scd() should be used */
void
scs_scs (int *cpi)
{
  unsigned char curchar;

  fprintf (stderr, "scs_scs was called but is obsolete!!!\n");
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
      fprintf (stderr, "SCS = %d\n", curchar);
#endif
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD20429 command %x\n", curchar);
    }
  return;
}


void
scs_default (Tn5250SCS * This)
{
  printf ("%c", This->curchar);
  return;
}


/* scs_main - reads the scs stream and calls functions to handle events
 */
void
scs_main (Tn5250SCS * This)
{
  int curchar;

  while ((curchar = fgetc (stdin)) != EOF)
    {
      This->curchar = curchar;
#ifdef DEBUG
      fprintf (stderr, "%x ", This->curchar);
#endif
      switch (This->curchar)
	{
	case SCS_TRANSPARENT:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing TRANSPARENT\n");
#endif
	    This->transparent (This);
	    break;
	  }
	case SCS_NOOP:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing NOOP\n");
#endif
	    This->noop (This);
	    break;
	  }
	case SCS_CR:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing CR\n");
#endif
	    This->cr (This);
	    break;
	  }
	case SCS_FF:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing FF\n");
#endif
	    This->ff (This);
	    break;
	  }
	case SCS_RFF:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing RFF\n");
#endif
	    This->rff (This);
	    break;
	  }
	case SCS_NL:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing NL\n");
#endif
	    This->nl (This);
	    break;
	  }
	case SCS_RNL:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing RNL\n");
#endif
	    This->rnl (This);
	    break;
	  }
	case SCS_HT:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing HT\n");
#endif
	    This->ht (This);
	    break;
	  }
	case SCS_PP:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing PP\n");
#endif
	    This->pp (This);
	    break;
	  }
	case 0x2B:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing 2B\n");
#endif
	    This->process2b (This);
	    break;
	  }
	case 0xFF:
	  {
	    /* This is a hack */
	    /* Don't know where the 0xFF is coming from */
	    fprintf (stderr, "Unhandled op 0xFF\n");
	    break;
	  }
	default:
	  {
#ifdef DEBUG
	    fprintf (stderr, "doing scsdefault()\n");
#endif
	    This->scsdefault (This);
	    break;
	  }
	}

    }
  return;
}


/* This initializes the scs callbacks
 */
Tn5250SCS *
tn5250_scs_new ()
{
  Tn5250SCS *scs = tn5250_new (Tn5250SCS, 1);

  if (scs == NULL)
    {
      return NULL;
    }

  scs->sic = scs_sic;
  scs->sea = scs_sea;
  scs->noop = scs_noop;
  scs->transparent = scs_transparent;
  scs->spsu = scs_spsu;
  scs->ppm = scs_ppm;
  scs->spps = scs_spps;
  scs->shf = scs_shf;
  scs->svf = scs_svf;
  scs->ff = scs_ff;
  scs->rff = scs_rff;
  scs->sto = scs_sto;
  scs->shm = scs_shm;
  scs->svm = scs_svm;
  scs->sffc = scs_sffc;
  scs->scgl = scs_scgl;
  scs->scg = scs_scg;
  scs->sfg = scs_sfg;
  scs->scd = scs_scd;
  scs->pp = scs_pp;
  scs->sbs = scs_sbs;
  scs->sps = scs_sps;
  scs->nl = scs_nl;
  scs->irs = scs_irs;
  scs->rnl = scs_rnl;
  scs->irt = scs_irt;
  scs->stab = scs_stab;
  scs->ht = scs_ht;
  scs->it = scs_it;
  scs->sil = scs_sil;
  scs->lf = scs_lf;
  scs->cr = scs_cr;
  scs->ssld = scs_ssld;
  scs->sls = scs_sls;
  scs->sgea = scs_sgea;
  scs->process2b = scs_process2b;
  scs->scsdefault = scs_default;
  scs->pagewidth = 0;
  scs->pagelength = 0;
  scs->cpi = 0;
  scs->column = 0;
  scs->row = 0;
  scs->curchar = 0;
  scs->data = NULL;
  return scs;
}
