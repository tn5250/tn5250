/* scs2pdf -- Converts scs from standard input into PDF.
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

/* Modified by James Rich to follow the Adobe PDF-1.3 specification found
 * at http://partners.adobe.com/asn/developer/acrosdk/docs/PDFRef.pdf
 */

#include "tn5250-private.h"
#include "scs.h"

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

int scs2pdf_process34 (int *curpos, int *boldchars);
int scs2pdf_ahpp (int *curpos, int *boldchars);

int pdf_header ();
int pdf_catalog (int objnum, int outlinesobject, int pageobject);
int pdf_outlines (int objnum);
int pdf_begin_stream (int objnum, int fontname, int fontsize);
int pdf_end_stream ();
int pdf_stream_length (int objnum, int objlength);
int pdf_pages (int objnum, int pagechildren, int pages);
int pdf_page (int objnum, int parent, int contents, int procset, int font,
	      int boldfont, int pagewidth, int pagelength);
int pdf_procset (int objnum);
int pdf_font (int objnum, int fontname);
void pdf_xreftable (int objnum);
void pdf_trailer (int offset, int size, int root);
int pdf_process_char (char character, int flush);

unsigned char curchar;
unsigned char nextchar;

struct _expanding_array {
   int *data;
   int elems;
   int alloc;
};
typedef struct _expanding_array expanding_array;

expanding_array *array_new();
void array_append_val(expanding_array *array, int value);
int array_index(expanding_array *array, int idx);
void array_free(expanding_array *array);
   

expanding_array *ObjectList;

FILE *outfile;

int
main ()
{
  Tn5250CharMap *map;
  int pagewidth, pagelength;
  int objcount = 0;
  int filesize = 0;
  int streamsize = 0;
  int pagenumber;
  int textobjects[10000];
  int pageparent, procsetobject, fontobject, boldfontobject, rootobject;
  int i;
  int newpage = 0;
  int column;
  int new_line = 1;
  int columncheck = 0;
  int fontsize = 10, newfontsize;
  int boldchars, do_bold;
  char text[255];
  ObjectList = array_new ();

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

  /* Write out the PDF header.  filesize tracks how big the PDF is since
   * we need that information later.
   */
  filesize += pdf_header ();

  /* ObjectList contains an entry for the filesize when the object was
   * created.  Since the cross reference of a PDF needs to know what the
   * byte count is for the beginning of each object we use ObjectList to 
   * track it.
   */
  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_begin_stream (objcount, COURIER, fontsize);
#ifdef DEBUG
  fprintf (stderr, "objcount = %d\n", objcount);
#endif

  pagenumber = 1;
  column = 0;
  boldchars = 0;
  do_bold = 0;
  newfontsize = 0;

  while (!feof (stdin))
    {
      curchar = fgetc (stdin);
      switch (curchar)
	{
	case SCS_TRANSPARENT:
	  {
	    scs_transparent ();
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
	    newpage = 1;
	    break;
	  }
	case SCS_NL:
	  {
	    new_line = scs_nl ();

	    /* If the current position on the page (column) is further to the
	     * right than our current maximum line length (columncheck)
	     * increase our length.  This is used later to see if we need a
	     * larger page width if the page width is unspecified.
	     */
	    if (column > columncheck)
	      {
		columncheck = column;
	      }
	    column = 0;
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
	    boldchars = 0;
	    streamsize += scs2pdf_process34 (&column, &boldchars);

	    /* If scs2pdf_process34() tells us that there are bold characters
	     * to write and we aren't already in the middle of writing some
	     * bold character, flush the buffer and send the bold font to the
	     * PDF.
	     */
	    if ((boldchars > 0) && (do_bold == 0))
	      {
		do_bold = 1;
#ifdef DEBUG
		fprintf (stderr, "Starting bold font\n");
#endif
		streamsize += pdf_process_char ('\0', 1);
		sprintf (text, "\t\t/F%d %d Tf\n", COURIER_BOLD, fontsize);
		fprintf (outfile, "%s", text);
		streamsize += strlen (text);
	      }
	    break;
	  }
	case 0x2B:
	  {
	    scs_process2b (&pagewidth, &pagelength, &newfontsize);

	    /* If scs_process2b() wants to change the CPI (newfontsize), flush
	     * the buffer and send the new font to the PDF.  Note that right
	     * now we always reset the font to COURIER, even if we are using
	     * a different font.  This will be changed soon.
	     */
	    if (newfontsize > 0)
	      {
#ifdef DEBUG
		fprintf (stderr, "Changing font size to %d\n", newfontsize);
#endif
		streamsize += pdf_process_char ('\0', 1);
		sprintf (text, "\t\t/F%d %d Tf\n", COURIER, newfontsize);
		fprintf (outfile, "%s", text);
		streamsize += strlen (text);
		fontsize = newfontsize;
		newfontsize = 0;
	      }
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
	    if (newpage)
	      {
		/* We need to know what stream objects (textobjects) were
		 * put on this page.  We put one stream object on each
		 * page.
		 */
		textobjects[pagenumber - 1] = objcount;

		streamsize += pdf_process_char ('\0', 1);
		filesize += streamsize;
		filesize += pdf_end_stream ();

		array_append_val (ObjectList, filesize);
		objcount++;
		filesize += pdf_stream_length (objcount, streamsize);
		streamsize = 0;
#ifdef DEBUG
		fprintf (stderr, "objcount = %d\n", objcount);
#endif

		/* Ending the stream object above and starting a new one
		 * here constitutes a new page, in conjuction with the
		 * pdf_page() function below.
		 */
		array_append_val (ObjectList, filesize);
		objcount++;
		filesize += pdf_begin_stream (objcount, COURIER, fontsize);
#ifdef DEBUG
		fprintf (stderr, "objcount = %d\n", objcount);
#endif

		pagenumber++;
		newpage = 0;
		column = 0;
	      }
	    if (new_line)
	      {
		/* On newline flush the buffer and move the active line down
		 * 12 points.
		 */
		new_line = 0;
		streamsize += pdf_process_char ('\0', 1);
		fprintf (outfile, "0 -12 Td\n");
		streamsize += 9;
		column = 0;
	      }
	    streamsize += pdf_process_char (tn5250_char_map_to_local (map,
								      curchar),
					    0);
	    /* If you want to feed this program non-EBCDIC text then uncomment
	     * the line below and comment the line above.
	     */
	    /*
	     * streamsize += pdf_process_char(curchar, 0);
	     */
	    column++;

	    /* If we are currently printing bold character then decrement the
	     * bold character count (boldchars) given above by
	     * scs2pdf_process34() until there aren't any bold characters left
	     * to print.  When out of bold character reset the font and allow
	     * changing to bold again.
	     */
	    if (boldchars > 0)
	      {
		boldchars--;
		if (boldchars == 0)
		  {
#ifdef DEBUG
		    fprintf (stderr, "Ending bold font\n");
#endif
		    do_bold = 0;
		    streamsize += pdf_process_char ('\0', 1);
		    sprintf (text, "\t\t/F%d %d Tf\n", COURIER, fontsize);
		    fprintf (outfile, "%s", text);
		    streamsize += strlen (text);
		  }
	      }
	  }
	}

    }
  textobjects[pagenumber - 1] = objcount;
  streamsize += pdf_process_char ('\0', 1);
  filesize += streamsize;
  filesize += pdf_end_stream ();

  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_stream_length (objcount, streamsize);
#ifdef DEBUG
  fprintf (stderr, "stream length objcount = %d\n", objcount);
#endif

  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_catalog (objcount, objcount + 1, objcount + 5);
  rootobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "catalog objcount = %d\n", objcount);
#endif

  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_outlines (objcount);
#ifdef DEBUG
  fprintf (stderr, "outlines objcount = %d\n", objcount);
#endif

  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_procset (objcount);
  procsetobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "procedure set objcount = %d\n", objcount);
#endif

  /* We need to create a font object for every font we use.  Since we don't
   * necessarily know if we used bold just make a bold font object anyway.
   * It doesn't hurt to have objects that aren't used.
   */
  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_font (objcount, COURIER);
  fontobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "font objcount = %d\n", objcount);
#endif

  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_font (objcount, COURIER_BOLD);
  boldfontobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "bold font objcount = %d\n", objcount);
#endif

  /* If the page size was not specified and we have lines longer than we
   * think we can fit on a page increase the page width a half inch at a time.
   */
  if ((pagewidth == 0) && (columncheck > MAX_COLUMNS))
    {
      for (i = 0; i < 15; i++)
	{
	  if (columncheck < (MAX_COLUMNS + (5 * i)))
	    {
#ifdef DEBUG
	      fprintf (stderr, "columncheck = %d, pagewidth = %d\n",
		       columncheck, pagewidth);
#endif
	      break;
	    }
	  pagewidth = DEFAULT_PAGE_WIDTH + (720 * i);
	}
    }
  array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_pages (objcount, objcount + 1, pagenumber);
  pageparent = objcount;
#ifdef DEBUG
  fprintf (stderr, "pages objcount = %d\n", objcount);
#endif

  for (i = 0; i < pagenumber; i++)
    {
      array_append_val (ObjectList, filesize);
      objcount++;
      filesize += pdf_page (objcount,
			    pageparent,
			    textobjects[i], procsetobject, fontobject,
			    boldfontobject, pagewidth, pagelength);
#ifdef DEBUG
      fprintf (stderr, "page objcount = %d\n", objcount);
#endif
    }

  pdf_xreftable (objcount);
  pdf_trailer (filesize, objcount + 1, rootobject);

  array_free (ObjectList);
  tn5250_char_map_destroy (map);
  return (0);
}



/* This function is different than what is in scs.c because we want to pass
 * a pointer to the number of bold characters to print to scs2pdf_ahpp()
 * (which is a unique version of scs_ahpp()).  Since this program is the only
 * one that handles bold this is unique.
 */
int
scs2pdf_process34 (int *curpos, int *boldchars)
{
  int bytes;

  bytes = 0;
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
	bytes = scs2pdf_ahpp (curpos, boldchars);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
  return (bytes);
}



/* This function is different than what is in scs.c because we want to pass
 * a pointer to the number of bold characters to print.  Since this program
 * is the only one that handles bold this is unique.
 */
int
scs2pdf_ahpp (int *curpos, int *boldchars)
{
  int position, bytes;
  int i;

  bytes = 0;
  position = fgetc (stdin);

  if (*curpos > position)
    {
      /* Frank Richter <frichter@esda.com> noticed that we should be
       * going back and printing over the same line if *curpos is greater
       * than position.  Without this his reports were wrong.  His patch
       * fixes the printouts.  What this does now is to go back to the
       * beginning of the line and print blanks over what is already there
       * up to position.  At that point we will receive the same text that
       * is already at position, which we will print over the top of itself.
       * This is gives a bold effect on real SCS printers.  This ought to
       * be handled a better way to get bold.
       */
      *boldchars = *curpos - position;
      bytes += pdf_process_char ('\0', 1);
      fprintf (outfile, "0 0 Td\n");
      bytes += 7;

      for (i = 0; i < position; i++)
	{
	  bytes += pdf_process_char (' ', 0);
	}
    }
  else
    {
      for (i = 0; i < (position - *curpos); i++)
	{
	  bytes += pdf_process_char (' ', 0);
	}
    }
  *curpos = position;

#ifdef DEBUG
  fprintf (stderr, "AHPP %d\n", position);
#endif
  return (bytes);
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
pdf_begin_stream (int objnum, int fontname, int fontsize)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Length %d 0 R\n"
	   "\t>>\n"
	   "stream\n"
	   "\tBT\n" "\t\t/F%d %d Tf\n" "\t\t36 756 Td\n",
	   objnum, objnum + 1, fontname, fontsize);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}


/* And this ends the stream object started above.*/
int
pdf_end_stream ()
{
  char *text = "\tET\n" "endstream\n" "endobj\n\n";

  fprintf (outfile, "%s", text);

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

  /* Add 32 to objlength since that is how many characters pdf_begin_stream()
   * and pdf_end_stream() add to the stream.  These functions already add
   * to the filesize correctly so we don't add 32 to filesize anywhere.
   */
  sprintf (text, "%d 0 obj\n" "\t%d\n" "endobj\n\n", objnum, objlength + 32);

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
	  int boldfont, int pagewidth, int pagelength)
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
  width = (pagewidth / 1440.0) * 72;
  length = (pagelength / 1440.0) * 72;
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
      fprintf (outfile,
	       "%010d 00000 n \n", array_index (ObjectList, curobj));
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


expanding_array *array_new() {

   expanding_array *array = NULL;

   array = malloc(sizeof(expanding_array));
   if (array == NULL) 
        return NULL;

   array->data = NULL;
   array->elems = 0;
   array->alloc = 0;
   
   return array;
}


void array_append_val(expanding_array *array, int value) {

   int *newdata;

   array->elems ++;

   if (array->elems > array->alloc) {
       array->alloc += 64;
       if (array->data == NULL) {
            array->data = malloc(sizeof(int) * array->alloc);
       } else {
            newdata = realloc(array->data, sizeof(int) * array->alloc);
            if (newdata == NULL) 
                free(array->data);
            array->data = newdata;
       }
       if (array->data == NULL) {
            fprintf(stderr, "array_append_val: Out of memory!\n");
            exit(1);
       }
   }
        
   array->data[array->elems - 1] = value;

}

int array_index(expanding_array *array, int idx) {

    return array->data[idx];

}
     
void array_free(expanding_array *array) {
    if (array->data != NULL) 
        free(array->data);
    free(array);
    array = NULL;
}
/* vi:set sts=3 sw=3: */
