/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997,1998,1999 Michael Madore
 * Converted by Rich Duzenbury from scs2ascii.c (Author Michael Madore)
 * to this file. 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 * As a special exception, the copyright holder gives permission
 * for additional uses of the text contained in its release of TN5250.
 * 
 * The exception is that, if you link the TN5250 library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the TN5250 library code into it.
 * 
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 * 
 * If you write modifications of your own for TN5250, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice. */

#include "tn5250-private.h"

static void scs2ps_sic();
static void scs2ps_sil();
static void scs2ps_sls();
static void scs2ps_process34();
static void scs2ps_process2b();
static void scs2ps_rff();
static void scs2ps_noop();
static void scs2ps_transparent();
static void scs2ps_processd2();
static void scs2ps_process03();
static void scs2ps_scs();
static void scs2ps_ssld();
static void scs2ps_process04();
static void scs2ps_processd1();
static void scs2ps_process06();
static void scs2ps_process07();
static void scs2ps_processd103();
static void scs2ps_stab();
static void scs2ps_jtf();
static void scs2ps_sjm();
static void scs2ps_spps();
static void scs2ps_ppm();
static void scs2ps_svm();
static void scs2ps_spsu();
static void scs2ps_sea();
static void scs2ps_shm();
static void scs2ps_sgea();
static void scs2ps_cr();
static void scs2ps_nl();
static void scs2ps_rnl();
static void scs2ps_ht();
static void scs2ps_ahpp();
static void scs2ps_avpp();
static void scs2ps_processd3();
static void scs2ps_sto();
static void scs2ps_ff();

static void scs2ps_jobheader();
static void scs2ps_jobfooter();
static void scs2ps_pageheader();
static void scs2ps_pagefooter();
static void scs2ps_printchar();
float scs2ps_getx();
float scs2ps_gety();

unsigned char curchar;
unsigned char nextchar;
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

int main()
{
   map = tn5250_char_map_new ("37");
   current_line = 1;
   new_line = 1;
   new_page = 1;
   mpp = 132;
   mlp = 66;
   ccp = 1;
   page = 0;

   tm = 36;  /* top margin in points */
   lm = 36;  /* left margin in points */
   rm = 36;  /* right margin in points */
   bm = 36;  /* bottom margin in points */
   pw = 612; /* maximum page width in points, 8.5 * 72 */
   pl = 792; /* maximum page length in points, 11 * 72 */

   /* printable area */
   paw = pw - lm - rm;
   pal = pl - tm - bm;

   /* calculate width & height of each character */
   /* kluge - can't seem to cast int to float, 
   /* (Boy it's been a long time since I coded any C!) */
   /* so I just assigned the ints to temporary float vars */
   /* This should be fixed! */
   mppf = mpp;
   pawf = paw;
   charwidth = pawf / mppf; 
   mlpf = mlp;
   palf = pal;
   charheight = palf / mlpf;

   scs2ps_jobheader();

   while (!feof(stdin)) {
      curchar = fgetc(stdin);
      switch (curchar) {
      case SCS_TRANSPARENT:{
	    scs2ps_transparent();
	    break;
	 }
      case SCS_RFF:{
	    scs2ps_rff();
	    break;
	 }
      case SCS_NOOP:{
	    scs2ps_noop();
	    break;
	 }
      case SCS_CR:{
	    scs2ps_cr();
	    break;
	 }
      case SCS_FF:{
	    scs2ps_ff();
	    break;
	 }
      case SCS_NL:{
	    scs2ps_nl();
	    break;
	 }
      case SCS_RNL:{
	    scs2ps_rnl();
	    break;
	 }
      case SCS_HT:{
	    scs2ps_ht();
	    break;
	 }
      case 0x34:{
	    scs2ps_process34();
	    break;
	 }
      case 0x2B:{
	    scs2ps_process2b();
	    break;
	 }
      case 0xFF:{
	    /* This is a hack */
	    /* Don't know where the 0xFF is coming from */
	    break;
	 }
      default:{
	    if (new_line) {
	       new_line = 0;
	    }
            scs2ps_printchar();
	    ccp++;
	    fprintf(stderr, ">%x\n", curchar);
	 }
      }

   }
   scs2ps_jobfooter();
   tn5250_char_map_destroy (map);
   return (0);
}

static void scs2ps_printchar()
{
  Tn5250Char printchar;

  printchar = tn5250_char_map_to_local(map, curchar);

  if (printchar != ' ') {

    /* print page header if needed */
    if (new_page == 1) {
      scs2ps_pageheader();
      new_page = 0;
    }

    /* escape any backslash, left paren, right paren */
    if ( (printchar == '\\') || (printchar == '(') || (printchar == ')') ) {
      printf("%.2f %.2f (\\%c) s\n", scs2ps_getx(), scs2ps_gety(), printchar);
    } else {
      printf("%.2f %.2f (%c) s\n", scs2ps_getx(), scs2ps_gety(), printchar);
    }
  }
}

static void scs2ps_jobheader()
{
  printf("%%!PS-Adobe-3.0\n");
  printf("%%%%Pages: (atend)\n");
  printf("%%%%Title: scs2ps\n");
  printf("%%%%BoundingBox: 0 0 %d %d\n", pw, pl);
  printf("%%%%LanguageLevel: 2\n");
  printf("%%%%EndComments\n\n");
  printf("%%%%BeginProlog\n");
  printf("%%%%BeginResource: procset general 1.0.0\n");
  printf("%%%%Title: (General Procedures)\n");
  printf("%%%%Version: 1.0\n");
  printf("/s { %% x y (string)\n");
  printf("  3 1 roll\n");
  printf("  moveto\n");
  printf("  show\n");
  printf("} def\n");
  printf("%%%%EndResource\n");
  printf("%%%%EndProlog\n\n");
}

static void scs2ps_jobfooter()
{
  printf("%%%%Trailer\n");
  printf("%%%%Pages: %d\n", page);
  printf("%%%%EOF\n");
}

static void scs2ps_pageheader()
{
  page++;
  printf("%%%%Page: %d %d\n",page,page);
  printf("%%%%BeginPageSetup\n");
  printf("/pgsave save def\n");
  printf("/Courier [%.2f 0 0 %.2f 0 0] selectfont\n", charwidth, charheight );
  printf("%%%%EndPageSetup\n");
}

static void scs2ps_pagefooter()
{
  printf("pgsave restore\n");
  printf("showpage\n");
  printf("%%%%PageTrailer\n");
}

float
scs2ps_getx()
{
  return lm + (ccp-1) * charwidth;
}

float
scs2ps_gety()
{
  return pl - (tm + (current_line * charheight));
}

static void scs2ps_ht()
{
   fprintf(stderr, "HT\n");
}

static void scs2ps_rnl()
{
   fprintf(stderr, "RNL\n");
}

static void scs2ps_nl()
{
   new_line = 1;
   current_line++;
   ccp = 1;
   fprintf(stderr, "NL\n");
}

static void scs2ps_ff()
{
   scs2ps_pagefooter();
   new_page = 1;
   current_line = 1;
   ccp = 1;
   fprintf(stderr, "FF\n");
}

static void scs2ps_cr()
{
   fprintf(stderr, "CR\n");
   ccp = 1;
}

static void scs2ps_transparent()
{

   int bytecount;
   int loop;

   bytecount = fgetc(stdin);
   fprintf(stderr, "TRANSPARENT (%x) = ", bytecount);
   for (loop = 0; loop < bytecount; loop++) {
      printf("%c", fgetc(stdin));
   }
}

static void scs2ps_rff()
{
   fprintf(stderr, "RFF\n");
}

static void scs2ps_noop()
{
   fprintf(stderr, "NOOP\n");
}

static void scs2ps_process34()
{

   curchar = fgetc(stdin);
   switch (curchar) {
   case SCS_AVPP:{
	 scs2ps_avpp();
	 break;
      }
   case SCS_AHPP:{
	 scs2ps_ahpp();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
   }
}

static void scs2ps_avpp()
{
   fprintf(stderr, "AVPP %d\n", fgetc(stdin));
}

static void scs2ps_ahpp()
{
/*   int position;
   int loop; */

   ccp = fgetc(stdin);

/*   position = fgetc(stdin);
   if (ccp > position) {
      printf("\r");
      for (loop = 0; loop < position; loop++) {
	 printf(" ");
      }
   } else {
      for (loop = 0; loop < position - ccp; loop++) {
	 printf(" ");
      }
   } */
   fprintf(stderr, "AHPP %d\n", ccp);
}

static void scs2ps_process2b()
{

   curchar = fgetc(stdin);
   switch (curchar) {
   case 0xD1:{
	 scs2ps_processd1();
	 break;
      }
   case 0xD2:{
	 scs2ps_processd2();
	 break;
      }
   case 0xD3:{
	 scs2ps_processd3();
	 break;
      }
   case 0xC8:{
	 scs2ps_sgea();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2B command %x\n", curchar);
      }
   }
}

static void scs2ps_processd3()
{
   curchar = fgetc(stdin);
   nextchar = fgetc(stdin);

   if (nextchar == 0xF6) {
      scs2ps_sto();
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD3 %x %x", curchar, nextchar);
   }
}

static void scs2ps_sto()
{
   int loop;

   fprintf(stderr, "STO = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_sgea()
{
   fprintf(stderr, "SGEA = %x %x %x\n", fgetc(stdin), fgetc(stdin), fgetc(stdin));
}

static void scs2ps_processd1()
{
   curchar = fgetc(stdin);
   switch (curchar) {
   case 0x06:{
	 scs2ps_process06();
	 break;
      }
   case 0x07:{
	 scs2ps_process07();
	 break;
      }
   case 0x03:{
	 scs2ps_processd103();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2BD1 command %x\n", curchar);
      }
   }
}

static void scs2ps_process06()
{
   curchar = fgetc(stdin);
   if (curchar == 0x01) {
      fprintf(stderr, "GCGID = %x %x %x %x\n", fgetc(stdin), fgetc(stdin),
	      fgetc(stdin), fgetc(stdin));
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD106 command %x\n", curchar);
   }
}

static void scs2ps_process07()
{
   curchar = fgetc(stdin);
   if (curchar == 0x05) {
      fprintf(stderr, "FID = %x %x %x %x %x\n", fgetc(stdin), fgetc(stdin),
	      fgetc(stdin), fgetc(stdin), fgetc(stdin));
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD107 command %x\n", curchar);
   }
}

static void scs2ps_processd103()
{
   curchar = fgetc(stdin);
   if (curchar == 0x81) {
      fprintf(stderr, "SCGL = %x\n", fgetc(stdin));
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD103 command %x\n", curchar);
   }
}

static void scs2ps_processd2()
{
   curchar = fgetc(stdin);
   nextchar = fgetc(stdin);


   switch (nextchar) {
   case 0x01:{
	 scs2ps_stab();
	 break;
      }
   case 0x03:{
	 scs2ps_jtf();
	 break;
      }
   case 0x0D:{
	 scs2ps_sjm();
	 break;
      }
   case 0x40:{
	 scs2ps_spps();
	 break;
      }
   case 0x48:{
	 scs2ps_ppm();
	 break;
      }
   case 0x49:{
	 scs2ps_svm();
	 break;
      }
   case 0x4c:{
	 scs2ps_spsu();
	 break;
      }
   case 0x85:{
	 scs2ps_sea();
	 break;
      }
   case 0x11:{
	 scs2ps_shm();
	 break;
      }
   default:{
	 switch (curchar) {
	 case 0x03:{
	       scs2ps_process03();
	       break;
	    }
	 case 0x04:{
	       scs2ps_process04();
	       break;
	    }
	 default:{
	       fprintf(stderr, "ERROR: Unknown 0x2BD2 command %x\n", curchar);
	    }
	 }
      }
   }

}

static void scs2ps_stab()
{
   int loop;

   fprintf(stderr, "STAB = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_jtf()
{
   int loop;

   fprintf(stderr, "JTF = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_sjm()
{
   int loop;

   fprintf(stderr, "SJM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_spps()
{
   int width;
   int length;

   fprintf(stderr, "SPPS = ");

   width = fgetc(stdin);
   width = (width << 8) + fgetc(stdin);

   length = fgetc(stdin);
   length = (length << 8) + fgetc(stdin);

   fprintf(stderr, "SPPS (width = %d) (length = %d)\n", width, length);

}

static void scs2ps_ppm()
{
   int loop;

   fprintf(stderr, "PPM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}


static void scs2ps_svm()
{
   int loop;

   fprintf(stderr, "SVM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_spsu()
{
   int loop;

   fprintf(stderr, "SPSU (%x) = ", curchar);
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_sea()
{
   int loop;

   fprintf(stderr, "SEA (%x) = ", curchar);
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_shm()
{
   int loop;

   fprintf(stderr, "SHM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ps_process03()
{
   switch (nextchar) {
   case 0x45:{
	 scs2ps_sic();
	 break;
      }
   case 0x07:{
	 scs2ps_sil();
	 break;
      }
   case 0x09:{
	 scs2ps_sls();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2BD203 command %x\n", curchar);
      }
   }
}

static void scs2ps_sic()
{
   curchar = fgetc(stdin);
   fprintf(stderr, "SIC = %x\n", curchar);
}

static void scs2ps_sil()
{
   curchar = fgetc(stdin);
   fprintf(stderr, "SIL = %d", curchar);
}

static void scs2ps_sls()
{
   curchar = fgetc(stdin);
   fprintf(stderr, "SLS = %d\n", curchar);
}

static void scs2ps_process04()
{
   switch (nextchar) {
   case 0x15:{
	 scs2ps_ssld();
	 break;
      }
   case 0x29:{
	 scs2ps_scs();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2BD204 command %x\n", curchar);
      }
   }
}

static void scs2ps_ssld()
{
   curchar = fgetc(stdin);
   nextchar = fgetc(stdin);
   fprintf(stderr, "SSLD = %d %d \n", curchar, nextchar);
}

static void scs2ps_scs()
{
   curchar = fgetc(stdin);
   if (curchar == 0x00) {
      curchar = fgetc(stdin);
      fprintf(stderr, "SCS = %d", curchar);
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD20429 command %x\n", curchar);
   }
}

/* vi:set sts=3 sw=3: */
