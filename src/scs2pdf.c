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
#include <glib.h>

static void scs2ascii_sic ();
static void scs2ascii_sil ();
static void scs2ascii_sls ();
int scs2ascii_process34 (int *curpos);
static void scs2ascii_process2b ();
static void scs2ascii_rff ();
static void scs2ascii_noop ();
static void scs2ascii_transparent ();
static void scs2ascii_processd2 ();
static void scs2ascii_process03 ();
static void scs2ascii_scs ();
static void scs2ascii_ssld ();
static void scs2ascii_process04 ();
static void scs2ascii_processd1 ();
static void scs2ascii_process06 ();
static void scs2ascii_process07 ();
static void scs2ascii_processd103 ();
static void scs2ascii_stab ();
static void scs2ascii_jtf ();
static void scs2ascii_sjm ();
static void scs2ascii_spps ();
static void scs2ascii_ppm ();
static void scs2ascii_svm ();
static void scs2ascii_spsu ();
static void scs2ascii_sea ();
static void scs2ascii_shm ();
static void scs2ascii_sgea ();
static void scs2ascii_cr ();
static void scs2ascii_nl ();
static void scs2ascii_rnl ();
static void scs2ascii_ht ();
int scs2ascii_ahpp (int *curpos);
static void scs2ascii_avpp ();
static void scs2ascii_processd3 ();
static void scs2ascii_sto ();
static void scs2ascii_ff ();

int pdf_header ();
int pdf_catalog (int objnum, int outlinesobject, int pageobject);
int pdf_outlines (int objnum);
int pdf_begin_stream (int objnum);
int pdf_end_stream ();
int pdf_stream_length (int objnum, int objlength);
int pdf_pages (int objnum, int pagechildren, int pages);
int pdf_page (int objnum, int parent, int contents, int procset, int font);
int pdf_procset (int objnum);
int pdf_font (int objnum);
void pdf_xreftable (int objnum);
void pdf_trailer (int offset, int size, int root);
int pdf_process_char (char character, int flush);

unsigned char curchar;
unsigned char nextchar;
int new_line;
int pagewidth = 612, pagelength = 792;

GArray *ObjectList;

int
main ()
{
  Tn5250CharMap *map = tn5250_char_map_new ("37");
  int objcount = 0;
  int filesize = 0;
  int streamsize = 0;
  int pagenumber;
  int textobjects[10000];
  int pageparent, procsetobject, fontobject, rootobject;
  int i;
  int newpage = 0;
  int column;
  ObjectList = g_array_new (FALSE, FALSE, sizeof (int));
  new_line = 1;

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
	    scs2ascii_transparent ();
	    break;
	  }
	case SCS_RFF:
	  {
	    scs2ascii_rff ();
	    break;
	  }
	case SCS_NOOP:
	  {
	    scs2ascii_noop ();
	    break;
	  }
	case SCS_CR:
	  {
	    scs2ascii_cr ();
	    break;
	  }
	case SCS_FF:
	  {
	    scs2ascii_ff ();
	    newpage = 1;
	    break;
	  }
	case SCS_NL:
	  {
	    scs2ascii_nl ();
	    break;
	  }
	case SCS_RNL:
	  {
	    scs2ascii_rnl ();
	    break;
	  }
	case SCS_HT:
	  {
	    scs2ascii_ht ();
	    break;
	  }
	case 0x34:
	  {
	    streamsize += scs2ascii_process34 (&column);
	    break;
	  }
	case 0x2B:
	  {
	    scs2ascii_process2b ();
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
	      }
	    if (new_line)
	      {
		new_line = 0;
		streamsize += pdf_process_char ('\0', 1);
		printf ("0 -12 Td\n");
		streamsize += 9;
		column = 0;
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
			    textobjects[i], procsetobject, fontobject);
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

static void
scs2ascii_ht ()
{
  fprintf (stderr, "HT\n");
}

static void
scs2ascii_rnl ()
{
  fprintf (stderr, "RNL\n");
}

static void
scs2ascii_nl ()
{
  new_line = 1;
#ifdef DEBUG
  fprintf (stderr, "NL\n");
#endif
}

static void
scs2ascii_ff ()
{
#ifdef DEBUG
  fprintf (stderr, "FF\n");
#endif
}

static void
scs2ascii_cr ()
{
  fprintf (stderr, "CR\n");
}

static void
scs2ascii_transparent ()
{

  int bytecount;
  int loop;

  bytecount = fgetc (stdin);
  fprintf (stderr, "TRANSPARENT (%x) = ", bytecount);
  for (loop = 0; loop < bytecount; loop++)
    {
      /*printf("%c", fgetc(stdin)); */
    }
}

static void
scs2ascii_rff ()
{
  fprintf (stderr, "RFF\n");
}

static void
scs2ascii_noop ()
{
#ifdef DEBUG
  fprintf (stderr, "NOOP\n");
#endif
}

int
scs2ascii_process34 (int *curpos)
{
  int bytes;

  bytes = 0;
  curchar = fgetc (stdin);
  switch (curchar)
    {
    case SCS_AVPP:
      {
	scs2ascii_avpp ();
	break;
      }
    case SCS_AHPP:
      {
	bytes = scs2ascii_ahpp (curpos);
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
  return (bytes);
}

static void
scs2ascii_avpp ()
{
  fprintf (stderr, "AVPP %d\n", fgetc (stdin));
}

int
scs2ascii_ahpp (int *curpos)
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
      printf ("0 -12 Td\n");
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

static void
scs2ascii_process2b ()
{

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case 0xD1:
      {
	scs2ascii_processd1 ();
	break;
      }
    case 0xD2:
      {
	scs2ascii_processd2 ();
	break;
      }
    case 0xD3:
      {
	scs2ascii_processd3 ();
	break;
      }
    case 0xC8:
      {
	scs2ascii_sgea ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2B command %x\n", curchar);
      }
    }
}

static void
scs2ascii_processd3 ()
{
  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);

  if (nextchar == 0xF6)
    {
      scs2ascii_sto ();
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD3 %x %x", curchar, nextchar);
    }
}

static void
scs2ascii_sto ()
{
  int loop;

  fprintf (stderr, "STO = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      /*fprintf(stderr, " %x", nextchar); */
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_sgea ()
{
  fprintf (stderr, "SGEA = %x %x %x\n", fgetc (stdin), fgetc (stdin),
	   fgetc (stdin));
}

static void
scs2ascii_processd1 ()
{
  curchar = fgetc (stdin);
  switch (curchar)
    {
    case 0x06:
      {
	scs2ascii_process06 ();
	break;
      }
    case 0x07:
      {
	scs2ascii_process07 ();
	break;
      }
    case 0x03:
      {
	scs2ascii_processd103 ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD1 command %x\n", curchar);
      }
    }
}

static void
scs2ascii_process06 ()
{
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

static void
scs2ascii_process07 ()
{
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

static void
scs2ascii_processd103 ()
{
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

static void
scs2ascii_processd2 ()
{
  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);


  switch (nextchar)
    {
    case 0x01:
      {
	scs2ascii_stab ();
	break;
      }
    case 0x03:
      {
	scs2ascii_jtf ();
	break;
      }
    case 0x0D:
      {
	scs2ascii_sjm ();
	break;
      }
    case 0x40:
      {
	scs2ascii_spps ();
	break;
      }
    case 0x48:
      {
	scs2ascii_ppm ();
	break;
      }
    case 0x49:
      {
	scs2ascii_svm ();
	break;
      }
    case 0x4c:
      {
	scs2ascii_spsu ();
	break;
      }
    case 0x85:
      {
	scs2ascii_sea ();
	break;
      }
    case 0x11:
      {
	scs2ascii_shm ();
	break;
      }
    default:
      {
	switch (curchar)
	  {
	  case 0x03:
	    {
	      scs2ascii_process03 ();
	      break;
	    }
	  case 0x04:
	    {
	      scs2ascii_process04 ();
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

static void
scs2ascii_stab ()
{
  int loop;

  fprintf (stderr, "STAB = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_jtf ()
{
  int loop;

  fprintf (stderr, "JTF = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_sjm ()
{
  int loop;

  fprintf (stderr, "SJM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_spps ()
{
  int width, length;

  fprintf (stderr, "SPPS = ");

  width = fgetc (stdin);
  width = (width << 8) + fgetc (stdin);
  pagewidth = width / 1440 * 72;

  length = fgetc (stdin);
  length = (length << 8) + fgetc (stdin);
  pagelength = length / 1440 * 72;

  fprintf (stderr, "SPPS (width = %d) (length = %d)\n", width, length);

}

static void
scs2ascii_ppm ()
{
  int loop;

  fprintf (stderr, "PPM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}


static void
scs2ascii_svm ()
{
  int loop;

  fprintf (stderr, "SVM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_spsu ()
{
  int loop;

  fprintf (stderr, "SPSU (%x) = ", curchar);
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_sea ()
{
  int loop;

  fprintf (stderr, "SEA (%x) = ", curchar);
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_shm ()
{
  int loop;

  fprintf (stderr, "SHM = ");
  for (loop = 0; loop < curchar - 2; loop++)
    {
      nextchar = fgetc (stdin);
      fprintf (stderr, " %x", nextchar);
    }
  fprintf (stderr, "\n");
}

static void
scs2ascii_process03 ()
{
  switch (nextchar)
    {
    case 0x45:
      {
	scs2ascii_sic ();
	break;
      }
    case 0x07:
      {
	scs2ascii_sil ();
	break;
      }
    case 0x09:
      {
	scs2ascii_sls ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD203 command %x\n", curchar);
      }
    }
}

static void
scs2ascii_sic ()
{
  curchar = fgetc (stdin);
  fprintf (stderr, "SIC = %x\n", curchar);
}

static void
scs2ascii_sil ()
{
  curchar = fgetc (stdin);
  fprintf (stderr, "SIL = %d", curchar);
}

static void
scs2ascii_sls ()
{
  curchar = fgetc (stdin);
  fprintf (stderr, "SLS = %d\n", curchar);
}

static void
scs2ascii_process04 ()
{
  switch (nextchar)
    {
    case 0x15:
      {
	scs2ascii_ssld ();
	break;
      }
    case 0x29:
      {
	scs2ascii_scs ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x2BD204 command %x\n", curchar);
      }
    }
}

static void
scs2ascii_ssld ()
{
  curchar = fgetc (stdin);
  nextchar = fgetc (stdin);
  fprintf (stderr, "SSLD = %d %d \n", curchar, nextchar);
}

static void
scs2ascii_scs ()
{
  curchar = fgetc (stdin);
  if (curchar == 0x00)
    {
      curchar = fgetc (stdin);
      fprintf (stderr, "SCS = %d", curchar);
    }
  else
    {
      fprintf (stderr, "ERROR: Unknown 0x2BD20429 command %x\n", curchar);
    }
}

int
pdf_header ()
{
  char *text = "%PDF-1.3\n\n";

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_catalog (int objnum, int outlinesobject, int pageobject)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n" \
	   "\t<<\n" \
	   "\t\t/Type /Catalog\n" \
	   "\t\t/Outlines %d 0 R\n" \
	   "\t\t/Pages %d 0 R\n" \
	   "\t>>\n" \
	   "endobj\n\n",
	   objnum, outlinesobject, pageobject);

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_outlines (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n" \
	   "\t<<\n" \
	   "\t\t/Type Outlines\n" \
	   "\t\t/Count 0\n" \
	   "\t>>\n" \
	   "endobj\n\n",
	   objnum);

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_begin_stream (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n" \
	   "\t<<\n" \
	   "\t\t/Length %d 0 R\n" \
	   "\t>>\n" \
	   "stream\n" \
	   "\tBT\n" \
	   "\t\t/F1 10 Tf\n" \
	   "\t\t36 756 Td\n",
	   objnum, objnum + 1);

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_end_stream ()
{
  char *text = "\tET\n" \
    "endstream\n" \
    "endobj\n\n";

  printf ("%s", text);

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
  sprintf (text,
	   "%d 0 obj\n" \
	   "\t%d\n" \
	   "endobj\n\n",
	   objnum, objlength + 32);

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_pages (int objnum, int pagechildren, int pages)
{
  char text[255];
  int bytes;
  int i;

  sprintf (text,
	   "%d 0 obj\n" \
	   "\t<<\n" \
	   "\t\t/Type /Pages\n" \
	   "\t\t/Kids [",
	   objnum);
  printf ("%s", text);
  bytes = strlen (text);
  sprintf (text,
	   " %d 0 R\n",
	   pagechildren);
  printf ("%s", text);
  bytes += strlen (text);
  for (i = 1; i < pages; i++)
    {
      sprintf (text,
	       "\t\t      %d 0 R\n",
	       pagechildren + i);
      printf ("%s", text);
      bytes += strlen (text);
    }
  sprintf (text,
	   "\t\t      ]\n" \
	   "\t\t/Count %d\n" \
	   "\t>>\n" \
	   "endobj\n\n",
	   pages);

  printf ("%s", text);
  bytes += strlen (text);

  return (bytes);
}

int
pdf_page (int objnum, int parent, int contents, int procset, int font)
{
  char text[255];

  /* PDF uses 72 points per inch so 8.5x11 in. page is 612x792 points
   * You can set MediaBox to [0 0 612 792] to force this
   */
  sprintf (text,
	   "%d 0 obj\n" \
	   "\t<<\n" \
	   "\t\t/Type /Page\n" \
	   "\t\t/Parent %d 0 R\n" \
	   "\t\t/MediaBox [0 0 %d %d]\n" \
	   "\t\t/Contents %d 0 R\n" \
	   "\t\t/Resources\n" \
	   "\t\t\t<<\n" \
	   "\t\t\t\t/ProcSet %d 0 R\n" \
	   "\t\t\t\t/Font\n" \
	   "\t\t\t\t\t<< /F1 %d 0 R >>\n" \
	   "\t\t\t>>\n" \
	   "\t>>\n" \
	   "endobj\n\n",
	   objnum, parent, pagewidth, pagelength, contents, procset, font);

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_procset (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n" \
	   "\t[/PDF /Text]\n" \
	   "endobj\n\n",
	   objnum);

  printf ("%s", text);

  return (strlen (text));
}

int
pdf_font (int objnum)
{
  char text[255];

  sprintf (text,
	   "%d 0 obj\n" \
	   "\t<<\n" \
	   "\t\t/Type /Font\n" \
	   "\t\t/Subtype /Type1\n" \
	   "\t\t/Name /F1\n" \
	   "\t\t/BaseFont /Courier\n" \
	   "\t\t/Encoding /MacRomanEncoding\n" \
	   "\t>>\n" \
	   "endobj\n\n",
	   objnum);

  printf ("%s", text);

  return (strlen (text));
}

void
pdf_xreftable (int objnum)
{
  int curobj;

  /* This part is important to get right or the PDF cannot be read.
   * The cross reference section always begins with the keyword 'xref'
   */
  printf ("xref\n");
  /* Then we follow with one or more cross reference subsections.  Since
   * this is always the first revision this cross reference will have no
   * more than one subsection.  The subsection numbering begins with 0.
   * After the subsection number we must indicate how many entries (objects)
   * are in this subsection.
   */
  printf ("0 %d\n", objnum + 1);
  /* The entries consist of a 10-digit byte offset (the number of bytes
   * from the beginning of the file to the beginning of the object to 
   * which the entry refers), followed by a space, followed by a 5-digit
   * generation number, followed by a two character end of line sequence
   * (either carriage return and line feed or space and line feed).
   * Object 0 is always free and always has a generation number of 65535
   * so we list that first.  We will never have more free entries.
   */
  printf ("0000000000 65535 f \n");
  /* The generation number will always be zeros for all in-use objects
   * since we are not updating anything.  We must have an entry for all
   * objects we created.
   */
  for (curobj = 0; curobj < objnum; curobj++)
    {
      printf ("%010d 00000 n \n", g_array_index (ObjectList, int, curobj));
    }
}

void
pdf_trailer (int offset, int size, int root)
{
  char text[255];

  sprintf (text,
	   "\ntrailer\n" \
	   "\t<<\n" \
	   "\t\t/Size %d\n" \
	   "\t\t/Root %d 0 R\n" \
	   "\t>>\n" \
	   "startxref\n" \
	   "%d\n" \
	   "%%%%EOF\n",
	   size, root, offset);

  printf ("%s", text);
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
      byteswritten = pdf_process_char('\\', 0);
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
      printf ("(%s) Tj\n", buf);
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
