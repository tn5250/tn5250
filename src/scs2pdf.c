/* scs2pdf -- Converts scs from standard input into PDF.
 * Copyright (C) 2000 Michael Madore
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

/* Modified by James Rich to follow the Adobe PDF-1.3 specification found
 * at http://partners.adobe.com/asn/developer/acrosdk/docs/PDFRef.pdf
 */

#include "tn5250-private.h"
#include "scs.h"
#include <glib.h>

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

int scs2pdf_process34 (int *curpos);
int scs2pdf_ahpp (int *curpos);

int pdf_header ();
int pdf_catalog (int objnum, int outlinesobject, int pageobject);
int pdf_outlines (int objnum);
int pdf_begin_stream (int objnum);
int pdf_end_stream ();
int pdf_stream_length (int objnum, int objlength);
int pdf_pages (int objnum, int pagechildren, int pages);
int pdf_page (int objnum, int parent, int contents, int procset, int font,
	      int pagewidth, int pagelength);
int pdf_procset (int objnum);
int pdf_font (int objnum);
void pdf_xreftable (int objnum);
void pdf_trailer (int offset, int size, int root);
int pdf_process_char (char character, int flush);

unsigned char curchar;
unsigned char nextchar;

int columncount;

GArray *ObjectList;

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
  int pageparent, procsetobject, fontobject, rootobject;
  int i;
  int newpage = 0;
  int column;
  int new_line = 1;
  int columncheck = 0;
  ObjectList = g_array_new (FALSE, FALSE, sizeof (int));
  columncount = 0;

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

  if ((getenv ("TN5250_CCSIDMAP")) != NULL)
    {
      map = tn5250_char_map_new (getenv ("TN5250_CCSIDMAP"));
    }
  else
    {
      map = tn5250_char_map_new ("37");
    }

  filesize += pdf_header ();

  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_begin_stream (objcount);
#ifdef DEBUG
  fprintf (stderr, "objcount = %d\n", objcount);
#endif

  pagenumber = 1;
  column = 0;

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
	    if (columncount > columncheck)
	      {
		columncheck = columncount;
		columncount = 0;
	      }
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
	    streamsize += scs2pdf_process34 (&column);
	    break;
	  }
	case 0x2B:
	  {
	    scs_process2b (&pagewidth, &pagelength);
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
		textobjects[pagenumber - 1] = objcount;

		streamsize += pdf_process_char ('\0', 1);
		filesize += streamsize;
		filesize += pdf_end_stream ();

		g_array_append_val (ObjectList, filesize);
		objcount++;
		filesize += pdf_stream_length (objcount, streamsize);
		streamsize = 0;
#ifdef DEBUG
		fprintf (stderr, "objcount = %d\n", objcount);
#endif

		g_array_append_val (ObjectList, filesize);
		objcount++;
		filesize += pdf_begin_stream (objcount);
#ifdef DEBUG
		fprintf (stderr, "objcount = %d\n", objcount);
#endif

		pagenumber++;
		newpage = 0;
		column = 0;
		columncount = 0;
	      }
	    if (new_line)
	      {
		new_line = 0;
		streamsize += pdf_process_char ('\0', 1);
		fprintf (outfile, "0 -12 Td\n");
		streamsize += 9;
		column = 0;
		columncount = 0;
	      }
	    streamsize += pdf_process_char (tn5250_char_map_to_local (map,
								      curchar),
					    0);
	    /*streamsize += pdf_process_char(curchar, 0); */
	    column++;
	  }
	}

    }
  textobjects[pagenumber - 1] = objcount;
  streamsize += pdf_process_char ('\0', 1);
  filesize += streamsize;
  filesize += pdf_end_stream ();

  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_stream_length (objcount, streamsize);
#ifdef DEBUG
  fprintf (stderr, "stream length objcount = %d\n", objcount);
#endif

  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_catalog (objcount, objcount + 1, objcount + 4);
  rootobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "catalog objcount = %d\n", objcount);
#endif

  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_outlines (objcount);
#ifdef DEBUG
  fprintf (stderr, "outlines objcount = %d\n", objcount);
#endif

  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_procset (objcount);
  procsetobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "procedure set objcount = %d\n", objcount);
#endif

  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_font (objcount);
  fontobject = objcount;
#ifdef DEBUG
  fprintf (stderr, "font objcount = %d\n", objcount);
#endif

  if ((pagewidth == 0) && (columncheck > MAX_COLUMNS))
    {
      for (i = 0; i < 15; i++)
	{
	  if (columncheck < (MAX_COLUMNS + (5 * i)))
	    {
#ifdef DEBUG
	      fprintf (stderr, "columncheck = %d, pagewidth = %d\n",
		       columncheck,
		       pagewidth);
#endif
	      break;
	    }
	  pagewidth = DEFAULT_PAGE_WIDTH + (720 * i);
	}
    }
  g_array_append_val (ObjectList, filesize);
  objcount++;
  filesize += pdf_pages (objcount, objcount + 1, pagenumber);
  pageparent = objcount;
#ifdef DEBUG
  fprintf (stderr, "pages objcount = %d\n", objcount);
#endif

  for (i = 0; i < pagenumber; i++)
    {
      g_array_append_val (ObjectList, filesize);
      objcount++;
      filesize += pdf_page (objcount,
			    pageparent,
			    textobjects[i], procsetobject, fontobject,
			    pagewidth, pagelength);
#ifdef DEBUG
      fprintf (stderr, "page objcount = %d\n", objcount);
#endif
    }

  pdf_xreftable (objcount);
  pdf_trailer (filesize, objcount + 1, rootobject);

  g_array_free (ObjectList, TRUE);
  tn5250_char_map_destroy (map);
  return (0);
}

int
scs2pdf_process34 (int *curpos)
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
	bytes = scs2pdf_ahpp (curpos);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
  return (bytes);
}

int
scs2pdf_ahpp (int *curpos)
{
  int position, bytes;
  int i;

  bytes = 0;
  position = fgetc (stdin);

  if (*curpos > position)
    {
      /* I think we should be writing a new line here but the output is
       * all screwed up when we do.  Since it looks correct without the
       * new line I'm going to leave it out. */
      /*
         bytes += pdf_process_char ('\0', 1);
         fprintf (outfile, "0 -12 Td\n");
         bytes += 9;
       */

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

int
pdf_header ()
{
  char *text = "%PDF-1.3\n\n";

  fprintf (outfile, "%s", text);

  return (strlen (text));
}

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

int
pdf_begin_stream (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Length %d 0 R\n"
	   "\t>>\n"
	   "stream\n"
	   "\tBT\n" "\t\t/F1 10 Tf\n" "\t\t36 756 Td\n", objnum, objnum + 1);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}

int
pdf_end_stream ()
{
  char *text = "\tET\n" "endstream\n" "endobj\n\n";

  fprintf (outfile, "%s", text);

  return (strlen (text));
}

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

int
pdf_page (int objnum, int parent, int contents, int procset, int font,
	  int pagewidth, int pagelength)
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
	   "\t\t\t\t\t<< /F1 %d 0 R >>\n"
	   "\t\t\t>>\n"
	   "\t>>\n"
	   "endobj\n\n",
	   objnum, parent, (int) width, (int) length,
	   contents, procset, font);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}

int
pdf_procset (int objnum)
{
  char text[255];

  sprintf (text, "%d 0 obj\n" "\t[/PDF /Text]\n" "endobj\n\n", objnum);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}

int
pdf_font (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n"
	   "\t<<\n"
	   "\t\t/Type /Font\n"
	   "\t\t/Subtype /Type1\n"
	   "\t\t/Name /F1\n"
	   "\t\t/BaseFont /Courier\n"
	   "\t\t/Encoding /MacRomanEncoding\n" "\t>>\n" "endobj\n\n", objnum);

  fprintf (outfile, "%s", text);

  return (strlen (text));
}

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
	       "%010d 00000 n \n", g_array_index (ObjectList, int, curobj));
    }
}

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

  if (character != '\\')
    {
      columncount++;
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

/* vi:set sts=3 sw=3: */
