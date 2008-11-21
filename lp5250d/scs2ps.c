/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997-2008 Michael Madore
 * Converted by Rich Duzenbury from scs2ascii.c (Author Michael Madore)
 * to this file. 
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

static void scs2ps_pp (Tn5250SCS * This);
static void scs2ps_cr (Tn5250SCS * This);
static void scs2ps_nl (Tn5250SCS * This);
static void scs2ps_ahpp ();
static void scs2ps_ff (Tn5250SCS * This);
static void scs2ps_default (Tn5250SCS * This);

static void scs2ps_jobheader ();
static void scs2ps_jobfooter ();
static void scs2ps_pageheader ();
static void scs2ps_pagefooter ();
static void scs2ps_printchar (unsigned char curchar);
float scs2ps_getx ();
float scs2ps_gety ();
Tn5250SCS *tn5250_scs2ps_new ();

int current_line;
int new_line;
int mpp;
int ccp;

int mlp;
int new_page;
int page;
int pw;
int pl;
int tm;
int bm;
int lm;
int rm;
int paw;
int pal;
float palf;
float pawf;
float mlpf;
float mppf;
float charwidth;
float charheight;

Tn5250CharMap *map;

int
main ()
{
  int pagewidth, pagelength;	/* These are unused for now */
  int cpi;			/* This is unused for now */
  Tn5250SCS *scs = NULL;

  current_line = 1;
  new_line = 1;
  new_page = 1;
  mpp = 132;
  mlp = 66;
  ccp = 1;
  page = 0;

  if ((getenv ("TN5250_CCSIDMAP")) != NULL)
    {
      map = tn5250_char_map_new (getenv ("TN5250_CCSIDMAP"));
    }
  else
    {
      map = tn5250_char_map_new ("37");
    }


  /* Initialize the scs toolkit */
  scs = tn5250_scs2ps_new ();

  if (scs == NULL)
    {
      return (-1);
    }

  scs->pagewidth = pagewidth;
  scs->pagelength = pagelength;
  scs->cpi = cpi;

  tm = 36;			/* top margin in points */
  lm = 36;			/* left margin in points */
  rm = 36;			/* right margin in points */
  bm = 36;			/* bottom margin in points */
  pw = 612;			/* maximum page width in points, 8.5 * 72 */
  pl = 792;			/* maximum page length in points, 11 * 72 */

  /* printable area */
  paw = pw - lm - rm;
  pal = pl - tm - bm;

  /* calculate width & height of each character
   * kluge - can't seem to cast int to float, 
   * (Boy it's been a long time since I coded any C!)
   * so I just assigned the ints to temporary float vars
   * This should be fixed!
   */
  mppf = mpp;
  pawf = paw;
  charwidth = pawf / mppf;
  mlpf = mlp;
  palf = pal;
  charheight = palf / mlpf;

  scs2ps_jobheader ();


  /* Turn control over to the SCS toolkit and run the event loop */
  scs_main (scs);

  scs2ps_jobfooter ();
  tn5250_char_map_destroy (map);
  free (scs);
  return (0);
}



/* This initializes the scs callbacks
 */
Tn5250SCS *
tn5250_scs2ps_new ()
{
  Tn5250SCS *scs = tn5250_scs_new ();

  if (scs == NULL)
    {
      fprintf (stderr,
	       "Unable to allocate memory in tn5250_scs2ps_new ()!\n");
      return NULL;
    }

  /*
     scs->data = tn5250_new (struct _Tn5250SCSPrivate, 1);
     if (scs->data == NULL)
     {
     free (scs);
     return NULL;
     }
   */

  /* And now set up our callbacks */
  scs->ff = scs2ps_ff;
  scs->rff = scs2ps_ff;
  scs->nl = scs2ps_nl;
  scs->rnl = scs2ps_nl;
  scs->pp = scs2ps_pp;
  scs->cr = scs2ps_cr;
  scs->scsdefault = scs2ps_default;
  return scs;
}


static void
scs2ps_printchar (unsigned char curchar)
{
  Tn5250Char printchar;

  printchar = tn5250_char_map_to_local (map, curchar);

  if (printchar != ' ')
    {

      /* print page header if needed */
      if (new_page == 1)
	{
	  scs2ps_pageheader ();
	  new_page = 0;
	}

      /* escape any backslash, left paren, right paren */
      if ((printchar == '\\') || (printchar == '(') || (printchar == ')'))
	{
	  printf ("%.2f %.2f (\\%c) s\n", scs2ps_getx (), scs2ps_gety (),
		  printchar);
	}
      else
	{
	  printf ("%.2f %.2f (%c) s\n", scs2ps_getx (), scs2ps_gety (),
		  printchar);
	}
    }
}

static void
scs2ps_jobheader ()
{
  printf ("%%!PS-Adobe-3.0\n");
  printf ("%%%%Pages: (atend)\n");
  printf ("%%%%Title: scs2ps\n");
  printf ("%%%%BoundingBox: 0 0 %d %d\n", pw, pl);
  printf ("%%%%LanguageLevel: 2\n");
  printf ("%%%%EndComments\n\n");
  printf ("%%%%BeginProlog\n");
  printf ("%%%%BeginResource: procset general 1.0.0\n");
  printf ("%%%%Title: (General Procedures)\n");
  printf ("%%%%Version: 1.0\n");
  printf ("/s { %% x y (string)\n");
  printf ("  3 1 roll\n");
  printf ("  moveto\n");
  printf ("  show\n");
  printf ("} def\n");
  printf ("%%%%EndResource\n");
  printf ("%%%%EndProlog\n\n");
}

static void
scs2ps_jobfooter ()
{
  printf ("%%%%Trailer\n");
  printf ("%%%%Pages: %d\n", page);
  printf ("%%%%EOF\n");
}

static void
scs2ps_pageheader ()
{
  page++;
  printf ("%%%%Page: %d %d\n", page, page);
  printf ("%%%%BeginPageSetup\n");
  printf ("/pgsave save def\n");
  printf ("/Courier [%.2f 0 0 %.2f 0 0] selectfont\n", charwidth, charheight);
  printf ("%%%%EndPageSetup\n");
}

static void
scs2ps_pagefooter ()
{
  printf ("pgsave restore\n");
  printf ("showpage\n");
  printf ("%%%%PageTrailer\n");
}

float
scs2ps_getx ()
{
  return lm + (ccp - 1) * charwidth;
}

float
scs2ps_gety ()
{
  return pl - (tm + (current_line * charheight));
}




static void
scs2ps_default (Tn5250SCS * This)
{
  if (new_line)
    {
      new_line = 0;
    }
  scs2ps_printchar (This->curchar);
  ccp++;
  fprintf (stderr, ">%x\n", This->curchar);
  return;
}

static void
scs2ps_nl (Tn5250SCS * This)
{
  new_line = 1;
  current_line++;
  ccp = 1;
  fprintf (stderr, "NL\n");
}

static void
scs2ps_ff (Tn5250SCS * This)
{
  scs2ps_pagefooter ();
  new_page = 1;
  current_line = 1;
  ccp = 1;
  fprintf (stderr, "FF\n");
}

static void
scs2ps_cr (Tn5250SCS * This)
{
  fprintf (stderr, "CR\n");
  ccp = 1;
}

static void
scs2ps_pp (Tn5250SCS * This)
{
  unsigned char curchar;

  curchar = fgetc (stdin);
  switch (curchar)
    {
    case SCS_AVPP:
      {
	scs_avpp (NULL);
	break;
      }
    case SCS_AHPP:
      {
	scs2ps_ahpp ();
	break;
      }
    default:
      {
	fprintf (stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
    }
}

static void
scs2ps_ahpp ()
{
/*
  int position;
  int loop;
*/

  ccp = fgetc (stdin);

/*
  position = fgetc(stdin);
  if (ccp > position) {
      printf("\r");
      for (loop = 0; loop < position; loop++) {
	 printf(" ");
      }
   } else {
      for (loop = 0; loop < position - ccp; loop++) {
	 printf(" ");
      }
   }
*/
  fprintf (stderr, "AHPP %d\n", ccp);
}

/* vi:set sts=3 sw=3: */
