/* scs2pdf -- Converts scs from standard input into PDF.
 * Copyright (C) 2000 Michael Madore
 * Copyright (C) 2007-2008 James Rich
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

/* Modified by James Rich to follow the Adobe PDF-1.3 specification found
 * at http://partners.adobe.com/asn/developer/acrosdk/docs/PDFRef.pdf
 */

#include "tn5250-private.h"
#include <syslog.h>

/* If getopt.h exists then getopt_long() probably does as well.  If
 * getopt.h doesn't exist (like on Solaris) then we probably need to use
 * plain old getopt().
 */
#ifdef HAVE_GETOPT_H
#define _GNU_SOURCE
#include <getopt.h>
#endif

/*
#define DEBUG
#define VERBOSE
*/

/* Define a default page size of 8.5x11 inches.  These numbers are a little
 * magic.  The iSeries sends a page size in terms of characters per inch,
 * but PDF uses points.  The ratio of the numbers below and 1440 is the
 * same ratio as 612 and 72.  So multiplying the ratio of numbers below over
 * 1440 by 72 will result in a 8.5x11 inch page,
 * i.e. (12240/1440) = (612/72) = 8.5 = page width
 *
 * Note that the 1440 constant came from Michael Madore.
 */
#define DEFAULT_PAGE_WIDTH 12240
#define DEFAULT_PAGE_LENGTH 15840
/* Define a maximum number of columns that fit in the default page size
 * above.  If the page size is not specified and we are given more characters
 * to print on a line than MAX_COLUMNS we increase the page width.
 */
#define MAX_COLUMNS 80

/* Define font names with a number that the PDF can use.  Numbers must be
 * greater than 0, but it doesn't matter what numbers are chosen.
 */
#define COURIER 1
#define COURIER_BOLD 2

void scs2pdf_nl (Tn5250SCS * This);
void scs2pdf_pp (Tn5250SCS * This);
void scs2pdf_ff (Tn5250SCS * This);
int scs2pdf_ahpp (Tn5250SCS * This, int *boldchars);
void scs2pdf_cr (Tn5250SCS * This);
void scs2pdf_ssld (Tn5250SCS * This);
void scs2pdf_sld (Tn5250SCS * This);
void scs2pdf_process2b (Tn5250SCS * This);
void scs2pdf_default (Tn5250SCS * This);
void scs2pdf_setfont (Tn5250SCS * This);

void do_newpage (Tn5250SCS * This);

int pdf_header ();
int pdf_catalog (int objnum, int outlinesobject, int pageobject);
int pdf_outlines (int objnum);
int pdf_begin_stream (Tn5250SCS * This, int fontname);
int pdf_end_stream (Tn5250SCS * This);
int pdf_stream_length (int objnum, int objlength);
int pdf_pages (int objnum, int pagechildren, int pages);
int pdf_page (int objnum, int parent, int contents, int procset, int font,
	      int boldfont, int pagewidth, int pagelength, int pdfleftmargin,
	      int pdftopmargin);
int pdf_procset (int objnum);
int pdf_font (int objnum, int fontname);
void pdf_xreftable (int objnum);
void pdf_trailer (int offset, int size, int root);
int pdf_process_char (char character, int flush);
Tn5250SCS *tn5250_scs2pdf_new ();

void print_help ();

struct _Tn5250SCSPrivate
{
  int newfontsize;
  int fontpointsize;
  int fontscalingfactor;
  unsigned long objcount;
  unsigned long streamsize;
  unsigned long filesize;
  int fontsize;
  int pdfleftmargin;
  int pdftopmargin;
  unsigned int pagenumber;
  int columncheck;
  int boldchars;
  int do_bold;
  char text[255];
  int newpage;
};

unsigned char nextchar;
Tn5250CharMap *map;

struct _expanding_array
{
  int *data;
  int elems;
  int alloc;
};
typedef struct _expanding_array expanding_array;

expanding_array *array_new ();
void array_append_val (expanding_array * array, int value);
int array_index (expanding_array * array, int idx);
void array_free (expanding_array * array);

expanding_array *textobjects;
expanding_array *ObjectList;

FILE *outfile;

int
main (int argc, char **argv)
{
#ifdef HAVE_GETOPT_H
  extern char *optarg;
  struct option options[5];
#endif
  int pageparent, procsetobject, fontobject, boldfontobject, rootobject;
  int i;
  int usesyslog = 0;
  int loglevel = 0;
  Tn5250SCS *scs = NULL;
  FILE *initfile;
  int useinitfile = 0;


#ifdef HAVE_GETOPT_H
  options[0].name = "help";
  options[0].has_arg = no_argument;
  options[0].flag = NULL;
  options[0].val = 'H';
  options[1].name = "syslog";
  options[1].has_arg = no_argument;
  options[1].flag = NULL;
  options[1].val = 's';
  options[2].name = "loglevel";
  options[2].has_arg = required_argument;
  options[2].flag = NULL;
  options[2].val = 'l';
  options[3].name = "initfile";
  options[3].has_arg = required_argument;
  options[3].flag = NULL;
  options[3].val = 'i';
  options[4].name = 0;
  options[4].has_arg = 0;
  options[4].flag = 0;
  options[4].val = 0;
#endif

#ifdef HAVE_GETOPT_H
  while ((i = getopt_long (argc, argv, "Hsl:i:", options, NULL)) != EOF)
#else
  while ((i = getopt (argc, argv, "Hsl:i:")) != EOF)
#endif
    {
      switch (i)
	{
	case '?':
	  print_help ();
	  return -1;
	case 'H':
	  print_help ();
	  return -1;
	case 's':
	  usesyslog = 1;
	  break;
	case 'l':
	  loglevel = atoi (optarg);
	  if (loglevel > SCS_LOG_MAX)
	    {
	      printf ("Invalid log level.  Highest log level is %d\n",
		      SCS_LOG_MAX);
	      print_help ();
	      return -1;
	    }
	  break;
	case 'i':
	  initfile = fopen (optarg, "r+");

	  if (initfile == NULL)
	    {
	      fprintf (stderr,
		       "Could not open printer initialization file.\n");
	      return -1;
	    }
	  useinitfile = 1;
	  break;
	}
    }


  ObjectList = array_new ();
  textobjects = array_new ();


  /* set up the syslog communication */
  if (usesyslog)
    {
      openlog ("scs2pdf", LOG_PID, LOG_DAEMON);
    }


  /* This allows the user to select an output file other than stdout.
   * I don't know that this will ever be useful since you do pretty much
   * anything with output redirection.  Maybe some architectures will use
   * this.
   */
  if ((getenv ("TN5250_PDF")) != NULL)
    {
      outfile = fopen (getenv ("TN5250_PDF"), "w");
      if (outfile == NULL)
	{
	  fprintf (stderr, "Could not open output file.\n");
	  exit (-1);
	}
    }
  else
    {
      outfile = stdout;
    }

  /* Get the appropriate CCSID map from the user.  lp5250d will set this
   * environment variable appropriately from the setting in the user's
   * .tn5250rc file.
   */
  if ((getenv ("TN5250_CCSIDMAP")) != NULL)
    {
      map = tn5250_char_map_new (getenv ("TN5250_CCSIDMAP"));
    }
  else
    {
      map = tn5250_char_map_new ("37");
    }



  /* Initialize the scs toolkit */
  scs = tn5250_scs2pdf_new ();

  if (scs == NULL)
    {
      return (-1);
    }

  scs->cpi = 10;
  scs->topmargin = 240;
  scs->column = 1;
  scs->usesyslog = usesyslog;
  scs->loglevel = loglevel;
  scs->data->newfontsize = 0;
  scs->data->fontpointsize = 12;
  scs->data->fontscalingfactor = 100;
  scs->data->objcount = 0;
  scs->data->streamsize = 0;
  scs->data->filesize = 0;
  scs->data->pdfleftmargin = 18;
  scs->data->pdftopmargin = 18;
  scs->data->pagenumber = 1;
  scs->data->columncheck = 0;
  scs->data->boldchars = 0;
  scs->data->do_bold = 0;
  scs->data->text[0] = '\0';
  scs->data->newpage = 0;


  if (useinitfile)
    {
      int result = 0;

      if (scs->usesyslog)
	{
	  syslog (LOG_INFO, "Reading printer state initialization file");
	}
      for (i = 0; fscanf (initfile, "%d", &result) != EOF; i++)
	{
	  switch (i)
	    {
	    case 0:
	      scs->pagewidth = result;
	      break;
	    case 1:
	      scs->pagelength = result;
	      break;
	    case 2:
	      scs->charwidth = result;
	      break;
	    case 3:
	      scs->cpi = result;
	      break;
	    case 4:
	      scs->lpi = result;
	      break;
	    case 5:
	      scs->data->fontpointsize = result;
	      break;
	    case 6:
	      scs->data->fontscalingfactor = result;
	      break;
	    case 7:
	      scs->leftmargin = result;
	      break;
	    case 8:
	      scs->rightmargin = result;
	      break;
	    case 9:
	      scs->topmargin = result;
	      break;
	    case 10:
	      scs->bottommargin = result;
	      break;
	    case 11:
	      scs->rotation = result;
	      break;
	    }
	}
      rewind (initfile);

      if (scs->usesyslog)
	{
	  syslog (LOG_INFO, "Page width: %d", scs->pagewidth);
	  syslog (LOG_INFO, "Page length: %d", scs->pagelength);
	  syslog (LOG_INFO, "Character width: %d", scs->charwidth);
	  syslog (LOG_INFO, "CPI: %d", scs->cpi);
	  syslog (LOG_INFO, "LPI: %d", scs->lpi);
	  syslog (LOG_INFO, "Font size: %d", scs->data->fontpointsize);
	  syslog (LOG_INFO, "Font scaling factor: %d",
		  scs->data->fontscalingfactor);
	  syslog (LOG_INFO, "Left margin: %d", scs->leftmargin);
	  syslog (LOG_INFO, "Right margin: %d", scs->rightmargin);
	  syslog (LOG_INFO, "Top margin: %d", scs->topmargin);
	  syslog (LOG_INFO, "Bottom margin: %d", scs->bottommargin);
	  syslog (LOG_INFO, "Page orientation: %d", scs->rotation);
	  syslog (LOG_INFO, "End of printer state initialization");
	}
    }


  /* Write out the PDF header.  filesize tracks how big the PDF is since
   * we need that information later.
   */
  scs->data->filesize += pdf_header ();

  /* ObjectList contains an entry for the filesize when the object was
   * created.  Since the cross reference of a PDF needs to know what the
   * byte count is for the beginning of each object we use ObjectList to 
   * track it.
   */
  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_begin_stream (scs, COURIER);
#ifdef DEBUG
  fprintf (stderr, "objcount = %d\n", scs->data->objcount);
#endif



  /* Turn control over to the SCS toolkit and run the event loop */
  scs_main (scs);


  array_append_val (textobjects, scs->data->objcount);
  scs->data->streamsize += pdf_process_char ('\0', 1);
  scs->data->filesize += scs->data->streamsize;
  scs->data->filesize += pdf_end_stream (scs);

  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize +=
    pdf_stream_length (scs->data->objcount, scs->data->streamsize);
#ifdef DEBUG
  fprintf (stderr, "stream length objcount = %d\n", scs->data->objcount);
#endif

  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_catalog (scs->data->objcount,
				      scs->data->objcount + 1,
				      scs->data->objcount + 5);
  rootobject = scs->data->objcount;
#ifdef DEBUG
  fprintf (stderr, "catalog objcount = %d\n", scs->data->objcount);
#endif

  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_outlines (scs->data->objcount);
#ifdef DEBUG
  fprintf (stderr, "outlines objcount = %d\n", scs->data->objcount);
#endif

  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_procset (scs->data->objcount);
  procsetobject = scs->data->objcount;
#ifdef DEBUG
  fprintf (stderr, "procedure set objcount = %d\n", scs->data->objcount);
#endif

  /* We need to create a font object for every font we use.  Since we don't
   * necessarily know if we used bold just make a bold font object anyway.
   * It doesn't hurt to have objects that aren't used.
   */
  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_font (scs->data->objcount, COURIER);
  fontobject = scs->data->objcount;
#ifdef DEBUG
  fprintf (stderr, "font objcount = %d\n", scs->data->objcount);
#endif

  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_font (scs->data->objcount, COURIER_BOLD);
  boldfontobject = scs->data->objcount;
#ifdef DEBUG
  fprintf (stderr, "bold font objcount = %d\n", scs->data->objcount);
#endif

  /* If the page size was not specified and we have lines longer than we
   * think we can fit on a page increase the page width a half inch at a time.
   */
  if ((scs->pagewidth == 0) && (scs->data->columncheck > MAX_COLUMNS))
    {
      for (i = 0; i < 15; i++)
	{
	  if (scs->data->columncheck < (MAX_COLUMNS + (5 * i)))
	    {
#ifdef DEBUG
	      fprintf (stderr, "columncheck = %d, pagewidth = %d\n",
		       scs->data->columncheck, scs->pagewidth);
#endif
	      break;
	    }
	  scs->pagewidth = DEFAULT_PAGE_WIDTH + (720 * i);
	}
    }
  array_append_val (ObjectList, scs->data->filesize);
  scs->data->objcount++;
  scs->data->filesize += pdf_pages (scs->data->objcount,
				    scs->data->objcount + 1,
				    scs->data->pagenumber);
  pageparent = scs->data->objcount;
#ifdef DEBUG
  fprintf (stderr, "pages objcount = %d\n", scs->data->objcount);
#endif

  for (i = 0; i < scs->data->pagenumber; i++)
    {
      array_append_val (ObjectList, scs->data->filesize);
      scs->data->objcount++;
      scs->data->filesize += pdf_page (scs->data->objcount,
				       pageparent,
				       array_index (textobjects, i),
				       procsetobject, fontobject,
				       boldfontobject, scs->pagewidth,
				       scs->pagelength,
				       scs->data->pdfleftmargin,
				       scs->data->pdftopmargin);
#ifdef DEBUG
      fprintf (stderr, "page objcount = %d\n", scs->data->objcount);
#endif
    }

  pdf_xreftable (scs->data->objcount);
  pdf_trailer (scs->data->filesize, scs->data->objcount + 1, rootobject);

  if (useinitfile)
    {
      fprintf (initfile, "%d\n", scs->pagewidth);
      fprintf (initfile, "%d\n", scs->pagelength);
      fprintf (initfile, "%d\n", scs->charwidth);
      fprintf (initfile, "%d\n", scs->cpi);
      fprintf (initfile, "%d\n", scs->lpi);
      fprintf (initfile, "%d\n", scs->data->fontpointsize);
      fprintf (initfile, "%d\n", scs->data->fontscalingfactor);
      fprintf (initfile, "%d\n", scs->leftmargin);
      fprintf (initfile, "%d\n", scs->rightmargin);
      fprintf (initfile, "%d\n", scs->topmargin);
      fprintf (initfile, "%d\n", scs->bottommargin);
      fprintf (initfile, "%d\n", scs->rotation);
    }

  array_free (textobjects);
  array_free (ObjectList);
  tn5250_char_map_destroy (map);
  free (scs);
  return (0);
}



/* This initializes the scs callbacks
 */
Tn5250SCS *
tn5250_scs2pdf_new ()
{
  Tn5250SCS *scs = tn5250_scs_new ();

  if (scs == NULL)
    {
      fprintf (stderr,
	       "Unable to allocate memory in tn5250_scs2pdf_new ()!\n");
      return NULL;
    }

  scs->data = tn5250_new (struct _Tn5250SCSPrivate, 1);
  if (scs->data == NULL)
    {
      free (scs);
      return NULL;
    }

  /* And now set up our callbacks */
  scs->pp = scs2pdf_pp;
  scs->nl = scs2pdf_nl;
  scs->rnl = scs2pdf_nl;
  scs->ff = scs2pdf_ff;
  scs->rff = scs2pdf_ff;
  scs->cr = scs2pdf_cr;
  scs->ssld = scs2pdf_ssld;
  scs->sld = scs2pdf_sld;
  scs->setfont = scs2pdf_setfont;
  /*scs->process2b = scs2pdf_process2b; */
  scs->scsdefault = scs2pdf_default;
  return scs;
}


void
scs2pdf_default (Tn5250SCS * This)
{
  if (This->data->newpage == 1)
    {
      do_newpage (This);
    }

  This->data->streamsize +=
    pdf_process_char (tn5250_char_map_to_local (map, This->curchar), 0);

  /* If you want to feed this program non-EBCDIC text then uncomment
   * the line below and comment the line above.
   *
   * streamsize += pdf_process_char(curchar, 0);
   */
  This->column = This->column + 1;

  /* If the current position on the page (column) is further to the
   * right than our current maximum line length (columncheck)
   * increase our length.  This is used later to see if we need a
   * larger page width if the page width is unspecified.
   */
  if (This->column > This->data->columncheck)
    {
      This->data->columncheck = This->column;
    }

  /* If we are currently printing bold character then decrement the
   * bold character count (boldchars) given above by
   * scs2pdf_pp() until there aren't any bold characters left
   * to print.  When out of bold character reset the font and allow
   * changing to bold again.
   */
  if (This->data->boldchars > 0)
    {
      This->data->boldchars = This->data->boldchars - 1;
      if (This->data->boldchars == 0)
	{
#ifdef DEBUG
	  fprintf (stderr, "Ending bold font\n");
#endif
	  This->data->do_bold = 0;
	  This->data->streamsize += pdf_process_char ('\0', 1);
	  sprintf (This->data->text, "\t\t/F%d %d Tf\n" "\t\t/F%d %d Tz\n",
		   COURIER, This->data->fontpointsize, COURIER,
		   This->data->fontscalingfactor);
	  fprintf (outfile, "%s", This->data->text);
	  This->data->streamsize += strlen (This->data->text);
	}
    }
  return;
}



/* Custom process 0x2B function
 *
 * No longer used
 */
void
scs2pdf_process2b (Tn5250SCS * This)
{
  scs_process2b (This);

  /* If scs_process2b() wants to change the CPI (newfontsize), flush
   * the buffer and send the new font to the PDF.  Note that right
   * now we always reset the font to COURIER, even if we are using
   * a different font.  This will be changed soon.
   */
  /*
     if (This->cpi > 0)
     {
     #ifdef DEBUG
     fprintf (stderr, "Changing font size to %d\n", This->cpi);
     #endif
     This->data->streamsize += pdf_process_char ('\0', 1);
     sprintf (This->data->text, "\t\t/F%d %d Tf\n", COURIER, This->cpi);
     fprintf (outfile, "%s", This->data->text);
     This->data->streamsize += strlen (This->data->text);
     This->cpi = 0;
     }
   */
  return;
}



/* Handle new lines uniquely to track column number
 */
void
scs2pdf_nl (Tn5250SCS * This)
{
  float leftmargin;
  int leftmarginint;
  char text[256] = {'\0'};

  if (This->data->newpage == 1)
    {
      do_newpage (This);
    }

  scs_nl (This);

  /* If the current position on the page (column) is further to the
   * right than our current maximum line length (columncheck)
   * increase our length.  This is used later to see if we need a
   * larger page width if the page width is unspecified.
   */
  if (This->column > This->data->columncheck)
    {
      This->data->columncheck = This->column;
    }

  leftmargin = (This->leftmargin - 1) / 1440.0;
  leftmarginint = leftmargin * 72;

  /* On newline flush the buffer and move the active line down
   * 12 points.
   */
  This->data->streamsize += pdf_process_char ('\0', 1);
  sprintf (text, "%d -%d Td", leftmarginint, 72 / This->lpi);
  fprintf (outfile, "%s\n", text);
  This->data->streamsize += (strlen(text) + 1);
  This->column = 1;
  return;
}



void
scs2pdf_ff (Tn5250SCS * This)
{
  scs_ff (This);
  This->column = 1;
  This->data->newpage = 1;
  return;
}



/* This function is different than what is in scs.c because we want to pass
 * a pointer to the number of bold characters to print to scs2pdf_ahpp()
 * (which is a unique version of scs_ahpp()).  Since this program is the only
 * one that handles bold this is unique.
 */
void
scs2pdf_pp (Tn5250SCS * This)
{
  unsigned char curchar;
  int bytes;

  bytes = 0;
  This->data->boldchars = 0;
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
	bytes = scs2pdf_ahpp (This, &(This->data->boldchars));
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
  This->data->streamsize += bytes;

  /* If scs2pdf_ahpp() tells us that there are bold characters
   * to write and we aren't already in the middle of writing some
   * bold character, flush the buffer and send the bold font to the
   * PDF.
   */
  if ((This->data->boldchars > 0) && (This->data->do_bold == 0))
    {
      This->data->do_bold = 1;
#ifdef DEBUG
      fprintf (stderr, "Starting bold font\n");
#endif
      This->data->streamsize += pdf_process_char ('\0', 1);
      sprintf (This->data->text, "\t\t/F%d %d Tf\n" "\t\t/F%d %d Tz\n",
	       COURIER_BOLD, This->data->fontpointsize, COURIER,
	       This->data->fontscalingfactor);
      fprintf (outfile, "%s", This->data->text);
      This->data->streamsize += strlen (This->data->text);
    }
  return;
}



/* This function is different than what is in scs.c because we want to pass
 * a pointer to the number of bold characters to print.  Since this program
 * is the only one that handles bold this is unique.
 */
int
scs2pdf_ahpp (Tn5250SCS * This, int *boldchars)
{
  int position, bytes;
  int i;

  if (This->data->newpage == 1)
    {
      do_newpage (This);
    }

  bytes = 0;
  position = fgetc (stdin);

  if ((This->usesyslog) && (This->loglevel > 0))
    {
      syslog (LOG_INFO,
	      "PP sent absolute horizontal move of %d (cursor currently on column %d)",
	      position, This->column);
    }

  if (This->column > position)
    {
      if ((This->usesyslog) && (This->loglevel > 0))
	{
	  syslog (LOG_INFO, "Moving left");
	}
    }
  else
    {
      if ((This->usesyslog) && (This->loglevel > 0))
	{
	  syslog (LOG_INFO, "Moving right");
	}
    }

#ifdef DEBUG
  fprintf (stderr, "AHPP %d (current position: %d)\n", position,
	   This->column);
#endif

  if ((This->column - 1) > position)
    {
      /* Frank Richter <frichter@esda.com> noticed that we should be
       * going back and printing over the same line if This->column is greater
       * than position.  Without this his reports were wrong.  His patch
       * fixes the printouts.  What this does now is to go back to the
       * beginning of the line and print blanks over what is already there
       * up to position.  At that point we will receive the same text that
       * is already at position, which we will print over the top of itself.
       * This is gives a bold effect on real SCS printers.  This ought to
       * be handled a better way to get bold.
       */
      *boldchars = This->column - position;
      bytes += pdf_process_char ('\0', 1);
      fprintf (outfile, "0 0 Td\n");
      bytes += 7;

      for (i = 0; i < position - 1; i++)
	{
	  bytes += pdf_process_char (' ', 0);
	}
    }
  else
    {
      for (i = 0; i < (position - This->column); i++)
	{
	  bytes += pdf_process_char (' ', 0);
	}
    }
  This->column = position;
  return (bytes);
}



/* Handle carriage return uniquely to track column number
 */
void
scs2pdf_cr (Tn5250SCS * This)
{
  if (This->data->newpage == 1)
    {
      do_newpage (This);
    }

  scs_cr (This);

  /* If the current position on the page (column) is further to the
   * right than our current maximum line length (columncheck)
   * increase our length.  This is used later to see if we need a
   * larger page width if the page width is unspecified.
   */
  if (This->column > This->data->columncheck)
    {
      This->data->columncheck = This->column;
    }
  /* On carriage return flush the buffer and move to the beginning of the
   * current line.
   */
  This->data->streamsize += pdf_process_char ('\0', 1);
  fprintf (outfile, "0 0 Td\n");
  This->data->streamsize += 7;
  This->column = 1;
  return;
}



/* Handle line density uniquely for font sizing.
 */
void
scs2pdf_ssld (Tn5250SCS * This)
{
  scs_ssld (This);
  /* This doesn't give good results yet, so just leave it at 12 */
  /*This->data->fontpointsize = 72 / This->lpi;*/
  return;
}



/* Handle line density uniquely for font sizing.
 */
void
scs2pdf_sld (Tn5250SCS * This)
{
  scs_sld (This);
  /* This doesn't give good results yet, so just leave it at 12 */
  /*This->data->fontpointsize = 72 / This->lpi;*/
  return;
}



/* Set the correct font width.
 */
void
scs2pdf_setfont (Tn5250SCS * This)
{
  float scale;

  /* The scaling factor is based on a 12 point Courier font which is 10 CPI.
   * 10 CPI means each character 144 units wide (one inch = 1440 units : 1440
   * units divided by 10 characters per inch equals 144 units per character)
   */
  scale = This->charwidth / 144.0;
  This->data->fontscalingfactor = scale * 100;

  This->data->streamsize += pdf_process_char ('\0', 1);
  sprintf (This->data->text, "\t\t/F%d %d Tz\n", COURIER,
	   This->data->fontscalingfactor);
  fprintf (outfile, "%s", This->data->text);
  This->data->streamsize += strlen (This->data->text);
  return;
}



void
do_newpage (Tn5250SCS * This)
{
  /* We need to know what stream objects (textobjects) were
   * put on this page.  We put one stream object on each
   * page.
   */
  array_append_val (textobjects, This->data->objcount);

  This->data->streamsize += pdf_process_char ('\0', 1);
  This->data->filesize += This->data->streamsize;
  This->data->filesize += pdf_end_stream (This);

  array_append_val (ObjectList, This->data->filesize);
  This->data->objcount = This->data->objcount + 1;
#ifdef DEBUG
  fprintf (stderr, "objcount: %d\n", This->data->objcount);
#endif
  This->data->filesize += pdf_stream_length (This->data->objcount,
					     This->data->streamsize);
  This->data->streamsize = 0;
#ifdef DEBUG
  fprintf (stderr, "objcount = %d\n", This->data->objcount);
#endif

  /* Ending the stream object above and starting a new one
   * here constitutes a new page, in conjuction with the
   * pdf_page() function below.
   */
  array_append_val (ObjectList, This->data->filesize);
  This->data->objcount = This->data->objcount + 1;
  This->data->filesize += pdf_begin_stream (This, COURIER);
#ifdef DEBUG
  fprintf (stderr, "objcount = %d\n", This->data->objcount);
#endif

  This->data->pagenumber++;
#ifdef DEBUG
  fprintf (stderr, "pagenumber: %d\n", This->data->pagenumber);
#endif
  This->column = 1;
  This->data->newpage = 0;
  return;
}



/* This header is required on all PDFs to identify what level of the PDF
 * specification was used to create this PDF.
 */
int
pdf_header ()
{
  char *text = "%PDF-1.3\n\n";

  fprintf (outfile, "%s", text);

  return (strlen (text));
}



/* This is required to tell the reader where to find stuff.*/
int
pdf_catalog (int objnum, int outlinesobject, int pageobject)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Type /Catalog\n"
	   "\t\t/Outlines %d 0 R\n"
	   "\t\t/Pages %d 0 R\n"
	   "\t>>\n" "endobj\n\n", objnum, outlinesobject, pageobject);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}



/* We don't really use outlines but they are required.*/
int
pdf_outlines (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Type Outlines\n"
	   "\t\t/Count 0\n" "\t>>\n" "endobj\n\n", objnum);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}



/* Each time we begin a page we use this to start the stream object that the
 * page will contain.
 */
int
pdf_begin_stream (Tn5250SCS * This, int fontname)
{
  char text1[255] = { '\0' };
  char text2[255] = { '\0' };
  int leftmargin;
  int pagelength;
  float topsize;
  int topmargin;

  sprintf (text1,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Length %d 0 R\n"
	   "\t>>\n"
	   "stream\n", This->data->objcount, This->data->objcount + 1);
  fprintf (outfile, "%s", text1);

  if (This->leftmargin == 0)
    {
      leftmargin = 1;
    }
  else
    {
      leftmargin = This->leftmargin;
    }

  if (This->pagelength == 0)
    {
      pagelength = 15840;
    }
  else
    {
      pagelength = This->pagelength;
    }

  topsize = ((pagelength - This->topmargin) / 1440.0) * 72.0;
  topmargin = topsize - This->data->pdftopmargin;
  sprintf (text2,
	   "\tBT\n" "\t\t/F%d %d Tf\n" "\t\t/F%d %d Tz\n" "\t\t%d %d Td\n",
	   fontname, This->data->fontpointsize, COURIER,
	   This->data->fontscalingfactor,
	   (((leftmargin - 1) / 1440) * 72) + This->data->pdfleftmargin,
	   topmargin);
  fprintf (outfile, "%s", text2);
  This->data->streamsize += strlen (text2);

  /* Don't return the length added to the stream size since the stream size
   * will be added to the file size once the stream has finished.
   */
  /*return (strlen (text1) + strlen (text2)); */
  return (strlen (text1));
}


/* And this ends the stream object started above.*/
int
pdf_end_stream (Tn5250SCS * This)
{
  char *text = "\tET\n" "endstream\n" "endobj\n\n";

  fprintf (outfile, "%s", text);
  This->data->streamsize += 4;
  return (strlen (text));
}



/* Since we don't know how long the stream object is when we start it we use
 * an indirect object to specify its length.  That indirect object points to
 * this function's output which is the length of the stream object created.
 */
int
pdf_stream_length (int objnum, int objlength)
{
  char text[255];

  sprintf (text, "%d 0 obj\n" "\t%d\n" "endobj\n\n", objnum, objlength);
  fprintf (outfile, "%s", text);
  return (strlen (text));
}



/* This starts the page tree.  We only have one root (this function) which
 * contains all the page leaves (created by pdf_page()).
 */
int
pdf_pages (int objnum, int pagechildren, int pages)
{
  char text[255];
  int bytes;
  int i;

  sprintf (text,
	   "%d 0 obj\n" "\t<<\n" "\t\t/Type /Pages\n" "\t\t/Kids [", objnum);
  fprintf (outfile, "%s", text);
  bytes = strlen (text);
  sprintf (text, " %d 0 R\n", pagechildren);
  fprintf (outfile, "%s", text);
  bytes += strlen (text);
  for (i = 1; i < pages; i++)
    {
      sprintf (text, "\t\t      %d 0 R\n", pagechildren + i);
      fprintf (outfile, "%s", text);
      bytes += strlen (text);
    }
  sprintf (text,
	   "\t\t      ]\n" "\t\t/Count %d\n" "\t>>\n" "endobj\n\n", pages);

  fprintf (outfile, "%s", text);
  bytes += strlen (text);

  return (bytes);
}



/* This describes the page size and contents for a page.  This is called once
 * for each page that is in the PDF.
 */
int
pdf_page (int objnum, int parent, int contents, int procset, int font,
	  int boldfont, int pagewidth, int pagelength, int pdfleftmargin,
	  int pdftopmargin)
{
  char text[255];
  float width, length;

  /* PDF uses 72 points per inch so 8.5x11 in. page is 612x792 points
   * You can set MediaBox to [0 0 612 792] to force this
   */
  if (pagewidth == 0)
    {
#ifdef DEBUG
      fprintf (stderr, "No page width given, using default.\n");
#endif
      pagewidth = DEFAULT_PAGE_WIDTH;
    }
  if (pagelength == 0)
    {
#ifdef DEBUG
      fprintf (stderr, "No page length given, using default.\n");
#endif
      pagelength = DEFAULT_PAGE_LENGTH;
    }
  width = ((pagewidth / 1440.0) * 72) + (pdfleftmargin * 2);
  length = ((pagelength / 1440.0) * 72) + (pdftopmargin * 2);
#ifdef DEBUG
  fprintf (stderr, "Setting page to %d (%f) by %d (%f) points\n",
	   (int) width, width, (int) length, length);
#endif
  sprintf (text,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Type /Page\n"
	   "\t\t/Parent %d 0 R\n"
	   "\t\t/MediaBox [0 0 %d %d]\n"
	   "\t\t/Contents %d 0 R\n"
	   "\t\t/Resources\n"
	   "\t\t\t<<\n"
	   "\t\t\t\t/ProcSet %d 0 R\n"
	   "\t\t\t\t/Font\n"
	   "\t\t\t\t\t<< /F%d %d 0 R\n"
	   "\t\t\t\t\t   /F%d %d 0 R >>\n"
	   "\t\t\t>>\n"
	   "\t>>\n"
	   "endobj\n\n",
	   objnum, parent, (int) width, (int) length,
	   contents, procset, COURIER, font, COURIER_BOLD, boldfont);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}



/* The required procedure set.*/
int
pdf_procset (int objnum)
{
  char text[255];

  sprintf (text, "%d 0 obj\n" "\t[/PDF /Text]\n" "endobj\n\n", objnum);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}



/* This creates the font objects used in the PDF.*/
int
pdf_font (int objnum, int fontname)
{
  char text[255];

  switch (fontname)
    {
    case COURIER:
      {
	sprintf (text,
		 "%d 0 obj\n"
		 "\t<<\n"
		 "\t\t/Type /Font\n"
		 "\t\t/Subtype /Type1\n"
		 "\t\t/Name /F%d\n"
		 "\t\t/BaseFont /Courier\n"
		 "\t\t/Encoding /WinAnsiEncoding\n" "\t>>\n" "endobj\n\n",
		 objnum, fontname);
	break;
      }
    case COURIER_BOLD:
      {
	sprintf (text,
		 "%d 0 obj\n"
		 "\t<<\n"
		 "\t\t/Type /Font\n"
		 "\t\t/Subtype /Type1\n"
		 "\t\t/Name /F%d\n"
		 "\t\t/BaseFont /Courier-Bold\n"
		 "\t\t/Encoding /WinAnsiEncoding\n" "\t>>\n" "endobj\n\n",
		 objnum, fontname);
	break;
      }
    default:
      {
	sprintf (text,
		 "%d 0 obj\n"
		 "\t<<\n"
		 "\t\t/Type /Font\n"
		 "\t\t/Subtype /Type1\n"
		 "\t\t/Name /F%d\n"
		 "\t\t/BaseFont /Courier\n"
		 "\t\t/Encoding /WinAnsiEncoding\n" "\t>>\n" "endobj\n\n",
		 objnum, fontname);
	break;
      }
    }

  fprintf (outfile, "%s", text);

  return (strlen (text));
}



/* The required cross reference table.*/
void
pdf_xreftable (int objnum)
{
  int curobj;

  /* This part is important to get right or the PDF cannot be read.
   * The cross reference section always begins with the keyword 'xref'
   */
  fprintf (outfile, "xref\n");
  /* Then we follow with one or more cross reference subsections.  Since
   * this is always the first revision this cross reference will have no
   * more than one subsection.  The subsection numbering begins with 0.
   * After the subsection number we must indicate how many entries (objects)
   * are in this subsection.
   */
  fprintf (outfile, "0 %d\n", objnum + 1);
  /* The entries consist of a 10-digit byte offset (the number of bytes
   * from the beginning of the file to the beginning of the object to 
   * which the entry refers), followed by a space, followed by a 5-digit
   * generation number, followed by a two character end of line sequence
   * (either carriage return and line feed or space and line feed).
   * Object 0 is always free and always has a generation number of 65535
   * so we list that first.  We will never have more free entries.
   */
  fprintf (outfile, "0000000000 65535 f \n");
  /* The generation number will always be zeros for all in-use objects
   * since we are not updating anything.  We must have an entry for all
   * objects we created.
   */
  for (curobj = 0; curobj < objnum; curobj++)
    {
      fprintf (outfile, "%010d 00000 n \n", array_index (ObjectList, curobj));
    }
}



/* And the required trailer.*/
void
pdf_trailer (int offset, int size, int root)
{
  char text[255];

  sprintf (text,
	   "\ntrailer\n"
	   "\t<<\n"
	   "\t\t/Size %d\n"
	   "\t\t/Root %d 0 R\n"
	   "\t>>\n" "startxref\n" "%d\n" "%%%%EOF\n", size, root, offset);

  fprintf (outfile, "%s", text);
}



/* Here we process the characters given in the input stream (stdin).  If
 * flush is 1 then flush the buffer.  The buffer is 255 bytes long since
 * that is what Adobe recommends because of limitations of some operating
 * environments.
 */
int
pdf_process_char (char character, int flush)
{
  static int bufloc = 0;
  static char buf[249];
  int byteswritten;

  byteswritten = 0;

  if (character == '(' || character == ')')
    {
      byteswritten = pdf_process_char ('\\', 0);
    }

  if (bufloc >= 247 || flush == 1)
    {
      /* This should never happen */
      if (bufloc > 247)
	{
	  buf[247] = character;
	  buf[248] = '\0';
	}
      else
	{
	  buf[bufloc] = character;
	  buf[bufloc + 1] = '\0';
	}
      fprintf (outfile, "(%s) Tj\n", buf);
      byteswritten += strlen (buf);
      memset (buf, '\0', 249);
      bufloc = 0;
      return (byteswritten + 6);
    }
  else
    {
      buf[bufloc] = character;
      bufloc++;
      return (byteswritten);
    }
}


expanding_array *
array_new ()
{
  expanding_array *array = NULL;

  array = malloc (sizeof (expanding_array));
  if (array == NULL)
    return NULL;

  array->data = NULL;
  array->elems = 0;
  array->alloc = 0;

  return array;
}


void
array_append_val (expanding_array * array, int value)
{
  int *newdata;

  array->elems++;

  if (array->elems > array->alloc)
    {
      array->alloc += 64;
      if (array->data == NULL)
	{
	  array->data = malloc (sizeof (int) * array->alloc);
	}
      else
	{
	  newdata = realloc (array->data, sizeof (int) * array->alloc);
	  if (newdata == NULL)
	    free (array->data);
	  array->data = newdata;
	}
      if (array->data == NULL)
	{
	  fprintf (stderr, "array_append_val: Out of memory!\n");
	  exit (1);
	}
    }

  array->data[array->elems - 1] = value;
  return;
}


int
array_index (expanding_array * array, int idx)
{
  return array->data[idx];
}


void
array_free (expanding_array * array)
{
  if (array->data != NULL)
    {
      free (array->data);
    }
  free (array);
  array = NULL;
  return;
}


void
print_help ()
{
  printf ("Usage: scs2pdf [OPTION]\n");
  printf ("Convert SCS print stream to PDF\n");
  printf ("\n");
#ifdef HAVE_GETOPT_H
  printf ("Mandatory arguments to long options are mandatory ");
  printf ("for short options too.\n");
#endif
#ifdef HAVE_GETOPT_H
  printf ("  -s, --syslog\t\tTurn on logging to system log daemon\n");
  printf ("  -l, --loglevel\tSet the logging level of detail (0 to %d)\n",
	  SCS_LOG_MAX);
  printf ("  -i, --initfile\tFile to use for printer state initialization\n");
  printf ("  -H, --help\t\tprint this help and exit\n");
#else
  printf ("  -s\tTurn on logging to system log daemon\n");
  printf ("  -l\tSet the logging level of detail (0 to %d)\n", SCS_LOG_MAX);
  printf ("  -i\tFile to use for printer state initialization\n");
  printf ("  -H\tprint this help and exit\n");
#endif
  printf ("\n");
  printf ("Report bugs to <linux5250@midrange.com>.\n");
  return;
}
