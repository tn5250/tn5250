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
#ifndef SCS
#define SCS

#define SCS_NOOP	0x00
#define SCS_TRANSPARENT	0x03
#define SCS_SW		0x2A
#define SCS_RFF		0x3A

#define SCS_AVPP	0xC4
#define SCS_AHPP	0xC0
#define SCS_CR		0x0D
#define SCS_NL		0x15
#define SCS_RNL		0x06
#define SCS_HT		0x05
#define SCS_FF		0x0C

#endif

void scs_sic ();
void scs_sil ();
void scs_sls ();
void scs_process34 (int *curpos);
void scs_process2b (int *pagewidth, int *pagelength, int *cpi);
void scs_rff ();
void scs_noop ();
void scs_transparent ();
void scs_processd2 (int *pagewidth, int *pagelength, int *cpi);
void scs_process03 (unsigned char nextchar, unsigned char curchar);
void scs_scs (int *cpi);
void scs_ssld ();
void scs_process04 (unsigned char nextchar, unsigned char curchar, int *cpi);
void scs_processd1 ();
void scs_process06 ();
void scs_process07 ();
void scs_processd103 ();
void scs_stab (unsigned char curchar);
void scs_jtf (unsigned char curchar);
void scs_sjm (unsigned char curchar);
void scs_spps (int *pagewidth, int *pagelength);
void scs_ppm (unsigned char curchar);
void scs_svm (unsigned char curchar);
void scs_spsu (unsigned char curchar);
void scs_sea (unsigned char curchar);
void scs_shm (unsigned char curchar);
void scs_sgea ();
void scs_cr ();
int scs_nl ();
void scs_rnl ();
void scs_ht ();
void scs_ahpp (int *curpos);
void scs_avpp ();
void scs_processd3 ();
void scs_sto (unsigned char curchar);
void scs_ff ();
