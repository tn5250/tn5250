/* 
 * Copyright (C) 2002 Scott Klement
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

/***********
 ** This program is run by the Setup program when TN5250 is installed, to
 ** ask the user for a "default character map".  it then proceeds to put
 ** the selected map into tn5250rc.
 ***********/


#include "tn5250-private.h"
#include "resource.h"
#include <sys/stat.h>

char *maplist[] = {
  "United States (CCSID 37)",
  "Canada (CCSID 37)",
  "Portugal (CCSID 37)",
  "Brazil (CCSID 37)",
  "Australia (CCSID 37)",
  "New Zealand (CCSID 37)",
  "Netherlands (CCSID 37)",
  "Netherlands (CCSID 256)",
  "Austria (CCSID 273)", 
  "Germany (CCSID 273)", 
  "Denmark (CCSID 277)", 
  "Norway (CCSID 277)", 
  "Finland (CCSID 278)", 
  "Sweden (CCSID 278)", 
  "Italy (CCSID 280)", 
  "Spain (CCSID 284)", 
  "Latin America (CCSID 284)",
  "United Kingdom (CCSID 285)",
  "Katakana Extended (CCSID 290)",
  "France (CCSID 297)",
  "Arabic (CCSID 420)",
  "Hebrew (CCSID 424)",
  "Belgium (CCSID 500)",
  "Canada (CCSID 500)",
  "Switzerland (CCSID 500)",
  "Eastern Europe (CCSID 870)",
  "Iceland (CCSID 871)",
  "Greece (CCSID 875)",
  "Cyrillic (CCSID 880)",
  "Turkey (Latin3) (CCSID 905)",
  "Turkey (Latin5) (CCSID 1026)",
   NULL
};
  

BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, PSTR cmdLine, int iCmdShow){

   int ccsid;
   FILE *tn5250rc;
   char *fn;
   char *quesfmt, *ques;
   struct stat discard;
   int screenwidth, screenheight;
   int replace;

 /* setup program will pass an arg of the pathname to the install dir */

   if (strlen(cmdLine) < 1) {
        MessageBox(NULL, "Invalid command-line for DFTMAP", "Setup", MB_OK);
        exit(255);
   }


 /* calculate the filename of tn5250rc */

   fn = malloc(strlen(cmdLine) + strlen("\\tn5250rc") + 1);
   if (fn==NULL) 
       exit(255);
   sprintf(fn, "%s\\tn5250rc", cmdLine);


 /* if the file exists, should we mess with it? */

   if (stat(fn, &discard)==0) {
       quesfmt = "You already have a TN5250 configuration file called\r\n"
                 "%s\r\n\r\n"
                 "Would you like to replace it?";
       ques = malloc(strlen(quesfmt) + strlen(fn) + 1);
       sprintf(ques, quesfmt, fn);
       replace = MessageBox(NULL, ques, "Setup",
                            MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
       free(ques);
       if (replace != IDYES) {
           free(fn);
           exit(0);
       }
   }


 /* ask the user for a CCSID map */

   ccsid = DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_DFTMAP), NULL, DlgProc);
   if (ccsid < 1) {
        free(fn);
        exit(1);
   }


 /* write map to tn5250rc file */

   tn5250rc = fopen(fn, "w");
   if (tn5250rc != NULL) {
       fprintf(tn5250rc, "map=%d\n", ccsid);
       if (ccsid == 37) 
           fprintf(tn5250rc, "font_80=Terminal\n");
       fclose(tn5250rc);
   }

   free(fn);

return 0;
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

     HWND hwndList;
     int x, cx, cy;
     RECT wr;
     char duh[100];

     switch (message) {

        case WM_INITDIALOG:
            GetWindowRect(hDlg, &wr);
            cx = GetSystemMetrics(SM_CXSCREEN);
            cy = GetSystemMetrics(SM_CYSCREEN);
            cx = cx/2 - (wr.right-wr.left)/2;
            cy = cy/2 - (wr.bottom-wr.top)/2;
            SetWindowPos(hDlg, NULL, cx, cy, 0, 0, 
                         SWP_NOSIZE|SWP_NOZORDER|SWP_NOREDRAW);
 
            hwndList = GetDlgItem (hDlg, IDC_LIST_MAPS);
            x=0;
            while (maplist[x] != NULL) {
                SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)maplist[x++]);
            }
            SendMessage(hwndList, LB_SELECTSTRING, -1, (LPARAM)"United States");
            return TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {
               case IDOK:
                   hwndList = GetDlgItem (hDlg, IDC_LIST_MAPS);
                   x = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                   if (x == LB_ERR) {
                       MessageBox(NULL, "You must select a character map!",
                          "Setup", MB_OK);
                   }
                   else {
                       char *buf;
                       int len, c;
                       len = SendMessage(hwndList, LB_GETTEXTLEN, x, 0);
                       buf = (char *)malloc(len+1);
                       SendMessage(hwndList, LB_GETTEXT, x, (LPARAM)buf);
                       c = atoi(strstr(buf, "CCSID")+strlen("CCSID "));
                       free(buf);
                       EndDialog(hDlg, c);
                   }
                   return TRUE;
	       case IDCANCEL:
                   EndDialog(hDlg, -1);
                   return TRUE;
            }
            break;

     }
     
     return FALSE;

}
