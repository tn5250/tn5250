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

#include "tn5250-private.h"
#include <glib.h>

#define MAXROWS 51
#define MAXCOLS 165

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

int pdf_header();
void pdf_trailer(int offset, int size);
void pdf_xreftable();
int pdf_catalog();
int pdf_outlines();
int pdf_pages(int objnum);
int pdf_page(int objnum, int parent, int contents);
int pdf_font();
int pdf_contents(int objnum);
int pdf_procset();

unsigned char curchar;
unsigned char nextchar;
int current_line;
int new_line;
int mpp;
int ccp;
char page[MAXROWS][MAXCOLS];
int filesize;
int currow;
int maxrows;
int pagewidth, pagelength;

GArray * ObjectList;
int objcount;

int main()
{
   Tn5250CharMap *map = tn5250_char_map_new ("37");
   ObjectList = g_array_new(FALSE, FALSE, sizeof(int));
   current_line = 1;
   new_line = 1;
   mpp = 132;
   maxrows = 51;
   ccp = 1;
   filesize = 0;
   objcount = 0;

   memset(page, ' ', sizeof(page));
   for(currow = 0; currow < MAXROWS; currow++) {
      page[currow][0] = '\0';
   }
   
   filesize += pdf_header();
   g_array_append_val(ObjectList, filesize);
   objcount++;
   filesize += pdf_catalog();
   g_array_append_val(ObjectList, filesize);
   objcount++;
   filesize += pdf_outlines();
   g_array_append_val(ObjectList, filesize);
   objcount++;
   filesize += pdf_font();
   g_array_append_val(ObjectList, filesize);
   objcount++;
   filesize += pdf_procset();      
   g_array_append_val(ObjectList, filesize);
   objcount++;

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
      case SCS_0x34:{
	    scs2ascii_process34();
	    break;
	 }
      case SCS_0x2B:{
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
	    page[current_line-1][ccp-1] = tn5250_char_map_to_local(map, curchar);
	    ccp++;
	    fprintf(stderr, ">%x\n", curchar);
	 }
      }

   }
   filesize += pdf_page(6, 5, 7);
   g_array_append_val(ObjectList, filesize);
   objcount++;
   filesize += pdf_pages(5);
   g_array_append_val(ObjectList, filesize);
   objcount++;
   pdf_xreftable();
   pdf_trailer(filesize, 8);
   
   g_array_free(ObjectList, TRUE);
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
   new_line = 1;
   page[current_line-1][ccp-1] = '\0';
   ccp = 1;
   current_line++;
   fprintf(stderr, "NL\n");
}

static void scs2ascii_ff()
{
   int row;
   char *currow;
   int junk;
   int loop;
   
   fprintf(stderr, "FF\n");
   filesize += pdf_contents(7);
   g_array_append_val(ObjectList, filesize);
   objcount++;
   current_line = 1;
   memset(page, ' ', sizeof(page));
   for(loop   = 0; loop   < MAXROWS; loop  ++) {
      page[loop  ][0] = '\0';
   }
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
   case AVPP:{
	 scs2ascii_avpp();
	 break;
      }
   case AHPP:{
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
      current_line++;
      for (loop = 0; loop < position; loop++) {
	 page[current_line-1][ccp-1] = ' ';
	 ccp++;
      }
   } else {
      for (loop = 0; loop < position - ccp; loop++) {
	 page[current_line-1][ccp-1] = ' ';
	 ccp++;
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
   int width, length;

   fprintf(stderr, "SPPS = ");

   width = fgetc(stdin);
   width = (width << 8) + fgetc(stdin);
   pagewidth = width/1440*72;

   length = fgetc(stdin);
   length = (length << 8) + fgetc(stdin);
   pagelength = length/1440*72;

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

int pdf_header()
{
   char *text = "%PDF-1.0\n";
   
   printf("%s", text);
   
   return(strlen(text));
}

void pdf_trailer(int offset, int size)
{
   char text[255];
   
   sprintf(text,
           "trailer\n" \
           "<<\n" \
           "/Size %d\n" \
           "/Root 1 0 R\n" \
           ">>\n" \
           "startxref\n" \
           "%d\n" \
           "%%%%EOF\n", size, offset);
           
   printf("%s", text);
}

void pdf_xreftable()
{
   int curobj, value;

   printf("xref\n");
   printf("0 8\n");
   printf("0000000000 65535 f\n");
   for(curobj = 0; curobj < objcount-1; curobj++) {
      value = g_array_index(ObjectList, int, curobj);
      printf("%010d 00000 n\n", value);
   }      
}

int pdf_catalog()
{
   char *text = "1 0 obj\n" \
                "<<\n" \
                "/Type /Catalog\n" \
                "/Pages 5 0 R\n" \
                "/Outlines 2 0 R\n" \
                ">>\n" \
                "endobj\n";
   
   printf("%s", text);
   
   return(strlen(text));
}

int pdf_outlines()
{
   char *text = "2 0 obj\n" \
                "/Type Outlines\n" \
                "/Count 0\n" \
                ">>\n" \
                "endobj\n";

   printf("%s", text);                
   
   return(strlen(text));
}

int pdf_pages(int objnum)
{
   char text[255];
   
   sprintf(text,
           "%d 0 obj\n" \
           "<<\n" \
           "/Type /Pages\n" \
           "/Count 1\n" \
           "/Kids [6 0 R]\n" \
           ">>\n" \
           "endobj\n", objnum);
           
   printf("%s", text);
   
   return(strlen(text));
}

int pdf_font()
{
   char *text = "3 0 obj\n" \
                "<<\n" \
                "/Type /Font\n" \
                "/Subtype /Type1\n" \
                "/Name /F1\n" \
                "/BaseFont /Courier\n" \
                "/Encoding /MacRomanEncoding\n" \
                ">>\n" \
                "endobj\n";
                
   printf("%s", text);
   
   return(strlen(text));
}

int pdf_contents(int objnum)
{
   int row, length, streamlength;
   char temp[255];
    
   length = 0;
   streamlength = 0;
   for(row = 0; row < MAXROWS       ; row++) {
      sprintf(temp, "%d %d Td (%s) Tj\n", 0, 10, &(page[row][0]));
      streamlength += strlen(temp);
   }
   length += streamlength;

   sprintf(temp, "%i 0 obj\n", objnum);
   printf("%s", temp);
   length += strlen(temp);

   sprintf(temp, "<< /Length %i >>\n", streamlength+16);
   printf("%s", temp);
   length += strlen(temp);
   
   sprintf(temp, 
           "stream\n" \
           "BT\n" \
           "/F1 10 Tf\n");
   printf("%s", temp);
   length += strlen(temp);
   
   for(row = MAXROWS-1      ; row >=0; row--) {
      printf("%d %d Td (%s) Tj\n", 0, 10, &(page[row][0]));
   }
   
   sprintf(temp,
           "ET\n" \
           "endstream\n" \
           "endobj\n");
           
   printf("%s", temp);
   length += strlen(temp);
      
   return(length);

}

int pdf_page(int objnum, int parent, int contents)
{
   char text[255];
   
   sprintf(text,
           "%d 0 obj\n" \
           "<<\n" \
           "/Type /Page\n" \
           "/Parent %d 0 R\n" \
           "/Resources << /Font << /F1 3 0 R >> /ProcSet 4 0 R >>\n" \
           "/MediaBox [0 0 %d %d]\n" \
           "/Contents %d 0 R\n" \
           ">>\n" \
           "endobj\n", objnum, parent, pagewidth, pagelength, contents);
           
   printf("%s", text);
   
   return(strlen(text));
}

int pdf_procset()
{
   char *text = "4 0 obj\n" \
                "[/PDF /Text]\n" \
                "endobj\n";
                
   printf("%s", text);
   
   return(strlen(text));
}

/* vi:set sts=3 sw=3: */
