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

static void scs2ascii_sic();
static void scs2ascii_sil();
static void scs2ascii_sls();
static void scs2ascii_process34();
static void scs2ascii_process2b();
static void scs2ascii_rff();
static void scs2ascii_noop();
static void scs2ascii_transparent();
static void scs2ascii_processd2();
static void scs2ascii_process03();
static void scs2ascii_scs();
static void scs2ascii_ssld();
static void scs2ascii_process04();
static void scs2ascii_processd1();
static void scs2ascii_process06();
static void scs2ascii_process07();
static void scs2ascii_processd103();
static void scs2ascii_stab();
static void scs2ascii_jtf();
static void scs2ascii_sjm();
static void scs2ascii_spps();
static void scs2ascii_ppm();
static void scs2ascii_svm();
static void scs2ascii_spsu();
static void scs2ascii_sea();
static void scs2ascii_shm();
static void scs2ascii_sgea();
static void scs2ascii_cr();
static void scs2ascii_nl();
static void scs2ascii_rnl();
static void scs2ascii_ht();
static void scs2ascii_ahpp();
static void scs2ascii_avpp();
static void scs2ascii_processd3();
static void scs2ascii_sto();
static void scs2ascii_ff();

unsigned char curchar;
unsigned char nextchar;
int current_line;
int new_line;
int mpp;
int ccp;

int main()
{
   Tn5250CharMap *map = tn5250_char_map_new ("37");
   current_line = 1;
   new_line = 1;
   mpp = 132;
   ccp = 1;

   while (!feof(stdin)) {
      curchar = fgetc(stdin);
      switch (curchar) {
      case SCS_TRANSPARENT:{
	    scs2ascii_transparent();
	    break;
	 }
      case SCS_RFF:{
	    scs2ascii_rff();
	    break;
	 }
      case SCS_NOOP:{
	    scs2ascii_noop();
	    break;
	 }
      case SCS_CR:{
	    scs2ascii_cr();
	    break;
	 }
      case SCS_FF:{
	    scs2ascii_ff();
	    break;
	 }
      case SCS_NL:{
	    scs2ascii_nl();
	    break;
	 }
      case SCS_RNL:{
	    scs2ascii_rnl();
	    break;
	 }
      case SCS_HT:{
	    scs2ascii_ht();
	    break;
	 }
      case 0x34:{
	    scs2ascii_process34();
	    break;
	 }
      case 0x2B:{
	    scs2ascii_process2b();
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
	    printf("%c", tn5250_char_map_to_local(map, curchar));
	    ccp++;
	    fprintf(stderr, ">%x\n", curchar);
	 }
      }

   }
   tn5250_char_map_destroy (map);
   return (0);
}

static void scs2ascii_ht()
{
   fprintf(stderr, "HT\n");
}

static void scs2ascii_rnl()
{
   fprintf(stderr, "RNL\n");
}

static void scs2ascii_nl()
{
   if (!new_line) {
   }
   printf("\n");
   new_line = 1;
   ccp = 1;
   fprintf(stderr, "NL\n");
}

static void scs2ascii_ff()
{
   printf("\f");
   fprintf(stderr, "FF\n");
}

static void scs2ascii_cr()
{
   fprintf(stderr, "CR\n");
}

static void scs2ascii_transparent()
{

   int bytecount;
   int loop;

   bytecount = fgetc(stdin);
   fprintf(stderr, "TRANSPARENT (%x) = ", bytecount);
   for (loop = 0; loop < bytecount; loop++) {
      printf("%c", fgetc(stdin));
   }
}

static void scs2ascii_rff()
{
   fprintf(stderr, "RFF\n");
}

static void scs2ascii_noop()
{
   fprintf(stderr, "NOOP\n");
}

static void scs2ascii_process34()
{

   curchar = fgetc(stdin);
   switch (curchar) {
   case SCS_AVPP:{
	 scs2ascii_avpp();
	 break;
      }
   case SCS_AHPP:{
	 scs2ascii_ahpp();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x34 command %x\n", curchar);
      }
   }
}

static void scs2ascii_avpp()
{
   fprintf(stderr, "AVPP %d\n", fgetc(stdin));
}

static void scs2ascii_ahpp()
{
   int position;
   int loop;

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
   fprintf(stderr, "AHPP %d\n", position);
}

static void scs2ascii_process2b()
{

   curchar = fgetc(stdin);
   switch (curchar) {
   case 0xD1:{
	 scs2ascii_processd1();
	 break;
      }
   case 0xD2:{
	 scs2ascii_processd2();
	 break;
      }
   case 0xD3:{
	 scs2ascii_processd3();
	 break;
      }
   case 0xC8:{
	 scs2ascii_sgea();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2B command %x\n", curchar);
      }
   }
}

static void scs2ascii_processd3()
{
   curchar = fgetc(stdin);
   nextchar = fgetc(stdin);

   if (nextchar == 0xF6) {
      scs2ascii_sto();
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD3 %x %x", curchar, nextchar);
   }
}

static void scs2ascii_sto()
{
   int loop;

   fprintf(stderr, "STO = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_sgea()
{
   fprintf(stderr, "SGEA = %x %x %x\n", fgetc(stdin), fgetc(stdin), fgetc(stdin));
}

static void scs2ascii_processd1()
{
   curchar = fgetc(stdin);
   switch (curchar) {
   case 0x06:{
	 scs2ascii_process06();
	 break;
      }
   case 0x07:{
	 scs2ascii_process07();
	 break;
      }
   case 0x03:{
	 scs2ascii_processd103();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2BD1 command %x\n", curchar);
      }
   }
}

static void scs2ascii_process06()
{
   curchar = fgetc(stdin);
   if (curchar == 0x01) {
      fprintf(stderr, "GCGID = %x %x %x %x\n", fgetc(stdin), fgetc(stdin),
	      fgetc(stdin), fgetc(stdin));
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD106 command %x\n", curchar);
   }
}

static void scs2ascii_process07()
{
   curchar = fgetc(stdin);
   if (curchar == 0x05) {
      fprintf(stderr, "FID = %x %x %x %x %x\n", fgetc(stdin), fgetc(stdin),
	      fgetc(stdin), fgetc(stdin), fgetc(stdin));
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD107 command %x\n", curchar);
   }
}

static void scs2ascii_processd103()
{
   curchar = fgetc(stdin);
   if (curchar == 0x81) {
      fprintf(stderr, "SCGL = %x\n", fgetc(stdin));
   } else {
      fprintf(stderr, "ERROR: Unknown 0x2BD103 command %x\n", curchar);
   }
}

static void scs2ascii_processd2()
{
   curchar = fgetc(stdin);
   nextchar = fgetc(stdin);


   switch (nextchar) {
   case 0x01:{
	 scs2ascii_stab();
	 break;
      }
   case 0x03:{
	 scs2ascii_jtf();
	 break;
      }
   case 0x0D:{
	 scs2ascii_sjm();
	 break;
      }
   case 0x40:{
	 scs2ascii_spps();
	 break;
      }
   case 0x48:{
	 scs2ascii_ppm();
	 break;
      }
   case 0x49:{
	 scs2ascii_svm();
	 break;
      }
   case 0x4c:{
	 scs2ascii_spsu();
	 break;
      }
   case 0x85:{
	 scs2ascii_sea();
	 break;
      }
   case 0x11:{
	 scs2ascii_shm();
	 break;
      }
   default:{
	 switch (curchar) {
	 case 0x03:{
	       scs2ascii_process03();
	       break;
	    }
	 case 0x04:{
	       scs2ascii_process04();
	       break;
	    }
	 default:{
	       fprintf(stderr, "ERROR: Unknown 0x2BD2 command %x\n", curchar);
	    }
	 }
      }
   }

}

static void scs2ascii_stab()
{
   int loop;

   fprintf(stderr, "STAB = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_jtf()
{
   int loop;

   fprintf(stderr, "JTF = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_sjm()
{
   int loop;

   fprintf(stderr, "SJM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_spps()
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

static void scs2ascii_ppm()
{
   int loop;

   fprintf(stderr, "PPM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}


static void scs2ascii_svm()
{
   int loop;

   fprintf(stderr, "SVM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_spsu()
{
   int loop;

   fprintf(stderr, "SPSU (%x) = ", curchar);
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_sea()
{
   int loop;

   fprintf(stderr, "SEA (%x) = ", curchar);
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_shm()
{
   int loop;

   fprintf(stderr, "SHM = ");
   for (loop = 0; loop < curchar - 2; loop++) {
      nextchar = fgetc(stdin);
      fprintf(stderr, " %x", nextchar);
   }
   fprintf(stderr, "\n");
}

static void scs2ascii_process03()
{
   switch (nextchar) {
   case 0x45:{
	 scs2ascii_sic();
	 break;
      }
   case 0x07:{
	 scs2ascii_sil();
	 break;
      }
   case 0x09:{
	 scs2ascii_sls();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2BD203 command %x\n", curchar);
      }
   }
}

static void scs2ascii_sic()
{
   curchar = fgetc(stdin);
   fprintf(stderr, "SIC = %x\n", curchar);
}

static void scs2ascii_sil()
{
   curchar = fgetc(stdin);
   fprintf(stderr, "SIL = %d", curchar);
}

static void scs2ascii_sls()
{
   curchar = fgetc(stdin);
   fprintf(stderr, "SLS = %d\n", curchar);
}

static void scs2ascii_process04()
{
   switch (nextchar) {
   case 0x15:{
	 scs2ascii_ssld();
	 break;
      }
   case 0x29:{
	 scs2ascii_scs();
	 break;
      }
   default:{
	 fprintf(stderr, "ERROR: Unknown 0x2BD204 command %x\n", curchar);
      }
   }
}

static void scs2ascii_ssld()
{
   curchar = fgetc(stdin);
   nextchar = fgetc(stdin);
   fprintf(stderr, "SSLD = %d %d \n", curchar, nextchar);
}

static void scs2ascii_scs()
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
