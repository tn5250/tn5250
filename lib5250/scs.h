/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997-2008 Michael Madore
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

/* These are defined in Chapter 16 Data Streams Non-IPDS Mode with AS/400
 * of the IPDS and SCS Technical Reference (IPDS_Tech_Ref.pdf)
 */
#define SCS_NOOP	0x00	/* Null */
#define SCS_RPT		0x0A	/* Repeat */
#define SCS_SW		0x2A	/* Switch */
#define SCS_TRANSPARENT	0x03	/* ASCII transparency */
#define SCS_BELL	0x2F	/* Bell/stop */

#define SCS_FF		0x0C	/* Form Feed/page end */
#define SCS_RFF		0x3A	/* Required Form Feed/required page end */

#define SCS_PP		0x34	/* Presentation Position */
#define SCS_RDPP	0x4C	/* Relative move Down */
#define SCS_AHPP	0xC0	/* Absolute Horizontal move */
#define SCS_AVPP	0xC4	/* Absolute Vertical move */
#define SCS_RRPP	0xC8	/* Relative move to the Right */
#define SCS_SBS		0x38	/* subscript */
#define SCS_SPS		0x09	/* superscript */
#define SCS_NL		0x15	/* New Line */
#define SCS_IRS		0x1E	/* Interchange Record Separator */
#define SCS_RNL		0x06	/* Required New Line */
#define SCS_HT		0x05	/* Horizontal Tab */
#define SCS_IT		0x39	/* Indent Tab */
#define SCS_LF		0x25	/* Line Feed */
#define SCS_CR		0x0D

/* These are for page handling convenience */
#define SCS_ROTATE0	0
#define SCS_ROTATE90	1
#define SCS_ROTATE180	2
#define SCS_ROTATE270	3

/* Logging levels */
#define SCS_LOG_BASIC	0
#define SCS_LOG_DETAIL	1
#define SCS_LOG_MAX	1

#endif



/* This struct holds the callback functions and data used by scs_main ()
 */
struct _Tn5250SCS
{
  struct _Tn5250SCSPrivate *data;

  void (*sic) (struct _Tn5250SCS * This);
  void (*sea) (struct _Tn5250SCS * This);
  void (*noop) (struct _Tn5250SCS * This);
  void (*rpt) (struct _Tn5250SCS * This);
  void (*sw) (struct _Tn5250SCS * This);
  void (*transparent) (struct _Tn5250SCS * This);
  void (*bel) (struct _Tn5250SCS * This);
  void (*spsu) (struct _Tn5250SCS * This);
  void (*ppm) (struct _Tn5250SCS * This);
  void (*spps) (struct _Tn5250SCS * This);
  void (*shf) (struct _Tn5250SCS * This);
  void (*svf) (struct _Tn5250SCS * This);
  void (*ff) (struct _Tn5250SCS * This);
  void (*rff) (struct _Tn5250SCS * This);
  void (*sto) (struct _Tn5250SCS * This);
  void (*shm) (struct _Tn5250SCS * This);
  void (*svm) (struct _Tn5250SCS * This);
  void (*sffc) (struct _Tn5250SCS * This);
  void (*scgl) (struct _Tn5250SCS * This);
  void (*scg) (struct _Tn5250SCS * This);
  void (*sfg) (struct _Tn5250SCS * This);
  void (*scd) (struct _Tn5250SCS * This);
  void (*pp) (struct _Tn5250SCS * This);
  void (*sbs) (struct _Tn5250SCS * This);
  void (*sps) (struct _Tn5250SCS * This);
  void (*nl) (struct _Tn5250SCS * This);
  void (*irs) (struct _Tn5250SCS * This);
  void (*rnl) (struct _Tn5250SCS * This);
  void (*irt) (struct _Tn5250SCS * This);
  void (*stab) (struct _Tn5250SCS * This);
  void (*ht) (struct _Tn5250SCS * This);
  void (*it) (struct _Tn5250SCS * This);
  void (*sil) (struct _Tn5250SCS * This);
  void (*lf) (struct _Tn5250SCS * This);
  void (*cr) (struct _Tn5250SCS * This);
  void (*ssld) (struct _Tn5250SCS * This);
  void (*sld) (struct _Tn5250SCS * This);
  void (*sls) (struct _Tn5250SCS * This);
  void (*sgea) (struct _Tn5250SCS * This);
  void (*process2b) (struct _Tn5250SCS * This);
  void (*cpi2points) (struct _Tn5250SCS * This);
  void (*setfont) (struct _Tn5250SCS * This);
  void (*scsdefault) (struct _Tn5250SCS * This);
  int pagewidth;
  int pagelength;
  int charwidth;
  int cpi;
  int lpi;
  int leftmargin;
  int rightmargin;
  int topmargin;
  int bottommargin;
  int column;
  int row;
  int rotation;
  int usesyslog;
  int loglevel;
  unsigned char curchar;
};

typedef struct _Tn5250SCS Tn5250SCS;


Tn5250SCS *tn5250_scs_new ();
