/*
 * Copyright (C) 2002-2008 Scott Klement
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
#include <shlobj.h>
#include <string.h>

void use_options_file(const char *fn, FILE *rcfile, const char *path);
HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszArgs,
    LPSTR lpszPathLink, LPSTR lpszDesc);
HRESULT GetShellFolderPath(int nFolder, LPSTR lpszShellPath);
void install_icon(const char *buf, const char *path);
void copyrcfile(FILE *optf, FILE *rcfile);


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
   char *fn, *optfn;
   char *quesfmt, *ques;
   struct stat discard;
   int screenwidth, screenheight;
   int replace;
   int use_opt;


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


/* calculate the filename of options */

   optfn = malloc(strlen(cmdLine) + strlen("\\options") + 1);
   if (optfn==NULL) 
       exit(255);
   sprintf(optfn, "%s\\options", cmdLine);


/* check if options file exists */

   use_opt = 0;
   if (stat(optfn, &discard)==0) {
      use_opt = 1;
   }


 /* if the tn5250rc file exists, should we mess with it? */

   if (!use_opt && stat(fn, &discard)==0) {
       quesfmt = "You already have a TN5250 configuration file called\r\n"
                 "%s\r\n\r\n"
                 "Would you like to replace it?";
       ques = malloc(strlen(quesfmt) + strlen(fn) + 1);
       sprintf(ques, quesfmt, fn);
       replace = MessageBox(NULL, ques, "Setup",
                            MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
       free(ques);
       if (replace != IDYES) {
           free(optfn);
           free(fn);
           exit(0);
       }
   }


 /* ask the user for a CCSID map */

   ccsid = DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_DFTMAP), NULL, DlgProc);
   if (ccsid < 1) {
        free(optfn);
        free(fn);
        exit(1);
   }


 /* write map to tn5250rc file */

   tn5250rc = fopen(fn, "w");
   if (tn5250rc != NULL) {
       fprintf(tn5250rc, "map=%d\n", ccsid);
       if (ccsid == 37) 
           fprintf(tn5250rc, "font_80=Terminal\n");
       if (use_opt) 
            use_options_file(optfn, tn5250rc, cmdLine);
       fclose(tn5250rc);
   }

   free(fn);
   free(optfn);

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

void use_options_file(const char *fn, FILE *rcfile, const char *path) {

    char buf[1024];
    FILE *optf;
    int first;
   
    optf = fopen(fn, "r");
    if (optf==NULL) {
         MessageBox(NULL, "Unable to open Options file!", "Setup", MB_OK);
         return;
    }

    first=1;
    while (fgets(buf, 1024, optf)!= NULL) {

        if ((*buf)=='\0' || (*buf)=='#') {
             /* skip these lines */
        }
        else if (first) {
             char msg[2048];
             sprintf(msg, "This installation has been customized for\n"
                      "\t%s\n"
                      "Do you want to install these customizations?",
                      buf);
             if (MessageBox(NULL, msg, "Tn5250 Setup", MB_YESNO)!=IDYES)  {
                  fclose(optf);
                  return;
             }
             first = 0;
        }
        else if (!strncmp(buf, "icon:", 5)) {
             install_icon(buf, path);
        }
        else if (!strncmp(buf, "tn5250rc:", 9)) {
             copyrcfile(optf, rcfile);
        }
    }

    fclose(optf);
    return;
}

void install_icon(const char *buf, const char *path) {
     char *mybuf, *profile;
     char *location, *type, *name;
     char objpath[MAX_PATH], iconpath[MAX_PATH];
     char pgmname[20];
     int x;

     mybuf = malloc(strlen(buf) + 1);
     if (mybuf == NULL) {
         MessageBox(NULL, "Out of Memory!", "Setup", MB_OK);
         exit (1);
     }

     strcpy(mybuf, buf);
     if ((name = strchr(mybuf, '\n'))!=NULL) *name = '\0';
     if ((name = strchr(mybuf, '\r'))!=NULL) *name = '\0';

     profile = mybuf + 6;

     location = strchr(profile, '\\');
     if (location == NULL) {
         MessageBox(NULL, "Invalid icon specification!", "Setup", MB_OK);
         free(mybuf);
         return;
     }

     *location = '\0';
     location++;

     type = strchr(location, '\\');
     if (type == NULL) {
         MessageBox(NULL, "Invalid icon specification!", "Setup", MB_OK);
         free(mybuf);
         return;
     }
     
     *type = '\0';
     type++;

     name = strchr(type, '\\');
     if (name == NULL) {
         MessageBox(NULL, "Invalid icon specification!", "Setup", MB_OK);
         free(mybuf);
         return;
     }
     
     *name = '\0';
     name++;

     for (x=0; x<strlen(location); x++) 
          location[x] = tolower(location[x]);

     for (x=0; x<strlen(type); x++) 
          type[x] = tolower(type[x]);

     if (strcmp(location, "desktop")==0) {
         GetShellFolderPath(CSIDL_DESKTOPDIRECTORY, iconpath);
     } else if (strcmp(location, "cdesktop")==0) {
         GetShellFolderPath(CSIDL_COMMON_DESKTOPDIRECTORY, iconpath);
     } else if (strcmp(location, "start menu")==0) {
         GetShellFolderPath(CSIDL_STARTMENU, iconpath);
     } else if (strcmp(location, "programs")==0) {
         GetShellFolderPath(CSIDL_PROGRAMS, iconpath);
     } else {
         free(mybuf);
         MessageBox(NULL, "Invalid icon location!", "Setup", MB_OK);
         return;
     }

     if (strcmp(type, "printer")==0
          || strcmp(type, "print")==0
          || strcmp(type, "lp5250")==0
          || strcmp(type, "lp5250d")==0)
         strcpy(pgmname, "LP5250D.EXE");
     else if (strcmp(type, "display")==0
          || strcmp(type, "terminal")==0
          || strcmp(type, "tn5250")==0)
         strcpy(pgmname, "TN5250.EXE");
     else {
         free(mybuf);
         MessageBox(NULL, "Invalid icon location!", "Setup", MB_OK);
         return;
     }
          
     strcat(iconpath, "\\");
     strcat(iconpath, name);
     strcat(iconpath, ".lnk");

     strcpy(objpath, path);
     strcat(objpath, "\\");
     strcat(objpath, pgmname);

     if (CreateLink(objpath, profile, iconpath, "TN5250 Shortcut")!=S_OK) {
         MessageBox(NULL, "CreateLink failed", "Setup", MB_OK);
     }
  
     free(mybuf);

     return;
}

void copyrcfile(FILE *optf, FILE *rcfile) {
     char buf[1026];
     while (fgets(buf, 1024, optf)!=NULL) 
          fputs(buf, rcfile);
}


/*******
 **  CreateLink(ObjectPath, Args, LinkPath, Description)
 **
 **   Creates a Windows link (ShortCut) 
 **
 **     ObjectPath = path to program/object you want to link to
 **           Args = arguments to put on command-line to ObjectPath
 **       LinkPath = where the link gets put 
 **    Description = description of link
 **
 **  Returns S_OK upon success...  anything else is an error
 ********/

HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszArgs,
    LPSTR lpszPathLink, LPSTR lpszDesc)
{
    HRESULT hres;
    IShellLinkA *psl;
    char msg[1024];

    CoInitialize(NULL);

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(&CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID *) &psl);

    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target, and add the
        // description.
        psl->lpVtbl->SetPath(psl, lpszPathObj);
        psl->lpVtbl->SetArguments(psl, lpszArgs);

        psl->lpVtbl->SetDescription(psl, lpszDesc);

       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
        hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile,
            (LPVOID *) &ppf);

        if (SUCCEEDED(hres)) {
            WORD wsz[MAX_PATH];

            // Ensure that the string is ANSI.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1,
                wsz, MAX_PATH);

            // Save the link by calling IPersistFile::Save.
            hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
            ppf->lpVtbl->SaveCompleted(ppf, wsz);
            ppf->lpVtbl->Release(ppf);

            if (hres == 0x8007007b) {
               sprintf(msg, "SaveLink: Bad filename: %s", lpszPathLink);
               MessageBox(NULL, msg, "Setup", MB_OK);
            }
            else if (hres != S_OK) {
               sprintf(msg, "SaveLink: Unknown failure (0x%X)", hres);
               MessageBox(NULL, msg, "Setup", MB_OK);
            }
        }
        else {
          sprintf(msg, "QueryInterface: Unknown failure (0x%X)", hres);
          MessageBox(NULL, msg, "Setup", MB_OK);
        }
        psl->lpVtbl->Release(psl);
    }
    else {
        if (hres == REGDB_E_CLASSNOTREG) {
          MessageBox(NULL, "CoCreateInstance: Class Not Registered", 
                     "Setup", MB_OK);
        } else if (hres == CLASS_E_NOAGGREGATION) {
          MessageBox(NULL, "CoCreateInstance: Class cannot be part of an"
                    " Aggregate", "Setup", MB_OK);
        } else {
          sprintf(msg, "CoCreateInstance: Unknown failure (0x%X)", hres);
          MessageBox(NULL, msg, "Setup", MB_OK);
        }
    }
    CoUninitialize();
    return hres;
}


/******
 **  GetShellFolderPath(Folder, Path):
 **     Get a Windows Path.   **
 **  Folder = AppData, Cache, Cookies, Desktop, Favorites, Fonts, History,
 **            NetHood, Personal, Printhood, Programs, Recent, SendTo,
 **            Start Menu, Startup, Templates or ShellNew
 **
 **    Path = Returned path
 **
 **  Returns S_OK for success, S_FALSE otherwise
 *******/   

HRESULT GetShellFolderPath(int nFolder, LPSTR lpszShellPath) {

  LPITEMIDLIST pidl;
  LPMALLOC     pShellMalloc;
  char         szDir[MAX_PATH];
  HRESULT      hres;
  char         msg[1024];

  hres = SHGetMalloc(&pShellMalloc);
  if (!SUCCEEDED(hres)) {
      sprintf(msg, "GetShellFolderPath: SHGetMalloc failed (hresult 0x%X)",
                    hres);
      MessageBox(NULL, msg, "Setup", MB_OK);
      return hres;
  } 
       
  hres = SHGetSpecialFolderLocation(NULL, nFolder, &pidl);
  if (!SUCCEEDED(hres)) {
      sprintf(msg, "GetShellFolderPath: SHGetSpecialFolderLocation"
                   " failed. (folder=0x%X, hresult 0x%X)", nFolder, hres);
      MessageBox(NULL, msg, "Setup", MB_OK);
      pShellMalloc->lpVtbl->Release(pShellMalloc);
      return hres;
  } 

 
  if (SHGetPathFromIDList(pidl, szDir) == FALSE) {
      pShellMalloc->lpVtbl->Free(pShellMalloc, pidl);
      pShellMalloc->lpVtbl->Release(pShellMalloc);
      MessageBox(NULL, "GetShellFolderPath: SHGetPathFromIDList"
                       " failed", "Setup", MB_OK);
      return S_FALSE;
  }

  strcpy(lpszShellPath, szDir);
  return S_OK;
}
