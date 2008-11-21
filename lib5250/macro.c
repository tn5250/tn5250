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

#include "tn5250-private.h"

#define	MAX_LINESZ	103	/* Maximum macro line size */
#define CR		0x0D
#define LF		0x0A
#define	MAX_SPECKEY	12	/* Maximum special key name size */

struct _MacroKey {
   int	km_code ;
   char	km_str[MAX_SPECKEY] ;
} ;

typedef struct _MacroKey MacroKey ;

/* Table based on terminal.h */
static const MacroKey MKey[] = {
	{K_ENTER ,	"ENTER" },
	{K_NEWLINE ,	"NEWLINE" },
	{K_TAB ,	"TAB" },
	{K_BACKTAB ,	"BACKTAB" },
	{K_F1 ,		"F1" },
	{K_F2 ,		"F2" },
	{K_F3 ,		"F3" },
	{K_F4 ,		"F4" },
	{K_F5 ,		"F5" },
	{K_F6 ,		"F6" },
	{K_F7 ,		"F7" },
	{K_F8 ,		"F8" },
	{K_F9 ,		"F9" },
	{K_F10 ,	"F10" },
	{K_F11 ,	"F11" },
	{K_F12 ,	"F12" },
	{K_F13 ,	"F13" },
	{K_F14 ,	"F14" },
	{K_F15 ,	"F15" },
	{K_F16 ,	"F16" },
	{K_F17 ,	"F17" },
	{K_F18 ,	"F18" },
	{K_F19 ,	"F19" },
	{K_F20 ,	"F20" },
	{K_F21 ,	"F21" },
	{K_F22 ,	"F22" },
	{K_F23 ,	"F23" },
	{K_F24 ,	"F24" },
	{K_LEFT ,	"LEFT" },
	{K_RIGHT ,	"RIGHT" },
	{K_UP ,		"UP" },
	{K_DOWN ,	"DOWN" },
	{K_ROLLDN ,	"ROLLDN" },
	{K_ROLLUP ,	"ROLLUP" },
	{K_BACKSPACE ,	"BACKSPACE" },
	{K_HOME ,	"HOME" },
	{K_END ,	"END" },
	{K_INSERT ,	"INSERT" },
	{K_DELETE ,	"DELETE" },
	{K_RESET ,	"RESET" },
	{K_PRINT ,	"PRINT" },
	{K_HELP ,	"HELP" },
	{K_SYSREQ ,	"SYSREQ" },
	{K_CLEAR ,	"CLEAR" },
	{K_REFRESH ,	"REFRESH" },
	{K_FIELDEXIT ,	"FIELDEXIT" },
	{K_TESTREQ ,	"TESTREQ" },
	{K_TOGGLE ,	"TOGGLE" },
	{K_ERASE ,	"ERASE" },
	{K_ATTENTION ,	"ATTENTION" },
	{K_DUPLICATE ,	"DUPLICATE" },
	{K_FIELDMINUS ,	"FIELDMINUS" },
	{K_FIELDPLUS ,	"FIELDPLUS" },
	{K_PREVWORD ,	"PREVWORD" },
	{K_NEXTWORD ,	"NEXTWORD" },
	{K_FIELDHOME ,	"FIELDHOME" },
	{K_EXEC ,	"EXEC" },
	{K_MEMO ,	"MEMO" },
	{K_COPY_TEXT ,	"COPY_TEXT" },
	{K_PASTE_TEXT ,	"PASTE_TEXT" },
	{0 ,		""}
} ;

char PState[12] ;		/* Printable state */

/*
 * Clean trailing CRs. Returns line size
 */
int macro_buffer_clean (char *Buff)
{
   int i ;

   i = strlen(Buff) - 1 ;
   while ((i >= 0) && ((Buff[i] == CR) || (Buff[i] == LF)))
      Buff[i--] = 0 ;

   return (i+1) ;
}

/*
 * Return macro number
 */
int macro_isnewmacro (char *Buff)
{
   int i,Num ;

   Num = 0 ;
   if ((Buff[0] == '[') && (Buff[1] == 'M'))
   {
      i = 2 ;
      while (isdigit(Buff[i]))
      {
         Num = (Num * 10) + Buff[i] - '0' ;
         i++ ;
      }
      if (Buff[i] != ']')
         Num = 0 ;
   }

   return (Num) ;
}

/*
 * Determines the macro size
 */
int macro_macrosize (int *Mac)
{
   int i ;

   i = 0 ;
   if (Mac != NULL)
      while (*Mac != 0)
      {
         i++ ;
         Mac++ ;
      }

   return (i) ;
}

/*
 * Reads a special key from macro text
 */
int macro_specialkey (char *Buff, int * Pt)
{
   int i,j ;

   if (Buff[*Pt] == '[')
   {
      i = 1 ;
      while ((Buff[*Pt+i] != 0) && (Buff[*Pt+i] != ']') && (i <= MAX_SPECKEY))
         i++ ;

      if (Buff[*Pt+i] == ']')
      {
         j = 0 ;
         while ((MKey[j].km_code != 0) && 
                (strncmp(MKey[j].km_str,&Buff[*Pt+1],i-1) != 0))
            j++ ;
         if (MKey[j].km_code != 0)
         {
            *Pt += i ;
            return (MKey[j].km_code) ;
         }
      }
   }
   return (0) ;
}

/*
 * Add the line contents to macro
 * TODO : add an escape character
 * TODO : resize macro buffer when special keys are transcoded
 */
void macro_addline (int **PDest, char *Buff, int Sz)
{
   int i,j,key ;
   int *Buffer ;
   int NSz, OSz ;

   if (*PDest == NULL)
   {
      Buffer = (int *)malloc((Sz+1)*sizeof(int));
      OSz = 0 ;
   }
   else
   {
      OSz = macro_macrosize(*PDest) ;
      NSz = OSz + Sz + 1 ;
      Buffer = *PDest ;
      Buffer = (int *)realloc(*PDest,NSz*sizeof(int)) ;
   }

   if (Buffer != NULL)
   {
      *PDest = Buffer ;

      i = j = 0 ;
      while (Buff[j] != 0)
      {
         if ((key = macro_specialkey (Buff,&j)) > 0)
            Buffer[OSz+i] = key ;	/* maybe should resize buffer here */
         else
            Buffer[OSz+i] = (int)Buff[j] ;
         i++ ;
         j++ ;
      }

      Buffer[OSz+i] = 0 ;
   }
}

/*
 * Load the macro definitions file
 */
char macro_loadfile (Tn5250Macro *Macro)
{
   FILE *MFile ;
   int Sz,Num,CurMacro ;
   char Buffer[MAX_LINESZ] ;

   if (Macro->fname != NULL)
   {
      if ((MFile = fopen (Macro->fname,"rt")) != NULL)
      {
         CurMacro = 0 ;
         while (fgets(Buffer,MAX_LINESZ,MFile) != NULL)
         {
            Sz = macro_buffer_clean (Buffer) ;
            if ((Num=macro_isnewmacro(Buffer)) > 0)
            {
               if (Num <= 24)
                  CurMacro = Num ;
            }
            else
               if ((CurMacro > 0) && (Sz > 0))
                  macro_addline (&Macro->BuffM[CurMacro-1],Buffer,Sz) ;
         }
         fclose (MFile) ;
      }
      return (1) ;
   }

   return (0) ;
}

/*
 * Clear macros in memory
 */
void macro_clearmem (Tn5250Macro *Macro)
{
   int i ;

   for (i=0;i < 24;i++)
      if (Macro->BuffM[i] != NULL)
      {
         free (Macro->BuffM[i]) ;
         Macro->BuffM[i] = NULL ;
      }
}

#ifdef __WIN32__
/*
 * Get the macro file name
 *    Win32 version -- Looks for a file in the same dir as the .exe
 */
char *macro_filename (Tn5250Display *Dsp)
{
#define PATHSIZE 1024
   LPTSTR apppath;
   LPTSTR dir, fname;
   DWORD len;
   LPTSTR lpMsgBuf;
   const char *cnf ;

   apppath = malloc(PATHSIZE+1);

   if (GetModuleFileName(NULL, apppath, PATHSIZE)<1) {
       FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, 
          GetLastError(),
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          lpMsgBuf,
          0, NULL
       );
       TN5250_LOG(("GetModuleFileName Error: %s\n", lpMsgBuf));
       MessageBox(NULL, lpMsgBuf, "TN5250", MB_OK);
       LocalFree(lpMsgBuf);
       return (NULL);
   }

   if (strrchr(apppath, '\\')) {
        len = strrchr(apppath, '\\') - apppath;
        apppath[len+1] = '\0';
   }

   dir = malloc(strlen(apppath) + 15);

   strcpy(dir, apppath);
   strcat(dir, "tn5250macros");
   free(apppath);

   fname = dir ;
   if ((cnf = tn5250_config_get (Dsp->config,"macros")) != NULL)
   {
      fname = (char *)malloc (strlen(cnf)+1) ;
      if (fname != NULL)
      {
         memcpy (fname,cnf,strlen(cnf)+1) ;
         free (dir) ;
      }
   }

   return (fname);
}

#else

/*
 * Get the macro file name
 */
char *macro_filename (Tn5250Display *Dsp)
{
   struct passwd *pwent;
   char *dir, *fname;
   const char *cnf ;

   pwent = getpwuid (getuid ());
   if (pwent == NULL)
      return (NULL) ;

   dir = (char *)malloc (strlen (pwent->pw_dir) + 16);
   if (dir == NULL)
      return (NULL) ;

   strcpy (dir, pwent->pw_dir);
   strcat (dir, "/.tn5250macros");

   fname = dir ;
   if ((cnf = tn5250_config_get (Dsp->config,"macros")) != NULL)
   {
      fname = (char *)malloc (strlen(cnf)+1) ;
      if (fname != NULL)
      {
         memcpy (fname,cnf,strlen(cnf)+1) ;
         free (dir) ;
      }
   }

   return (fname) ;
}

#endif

/****f* lib5250/tn5250_macro_init
 * NAME
 *    tn5250_macro_init
 * SYNOPSIS
 *    ret = tn5250_macro_init
 * INPUTS
 *    None
 * DESCRIPTION
 *    Macro system initialization
 *****/
Tn5250Macro *tn5250_macro_init()
{
   Tn5250Macro *This;
   int i ;

   This = tn5250_new(Tn5250Macro, 1);
   if (This == NULL)
      return NULL;

   This->RState = 0 ;
   This->EState = 0 ;
   This->TleBuff = 0 ;
   for (i=0;i < 24; i++)
      This->BuffM[i] = NULL ;

   return This;
}

/*
 * Write one macro to file
 */
void macro_write (int Num, int *Buff, FILE *MF)
{
   int i,j,Sz ;

   fprintf (MF,"[M%02i]\n",Num) ;

   i = Sz = 0 ;
   while (Buff[i] != 0)
   {
      j = 0 ;
      while ((MKey[j].km_code != 0) && (MKey[j].km_code != Buff[i]))
         j++ ;
      if (MKey[j].km_code == 0)
      {
         if (Sz + 1 > MAX_LINESZ-3)
         {
            fprintf (MF,"\n") ;
            Sz = 0 ;
         }
         fprintf (MF,"%c",(char)Buff[i]) ;
         Sz++ ;
      }
      else
      {
         if (Sz + strlen(MKey[j].km_str) + 2 > MAX_LINESZ-3)
         {
            fprintf (MF,"\n") ;
            Sz = 0 ;
         }
         fprintf (MF,"[%s]",MKey[j].km_str) ;
         Sz += strlen(MKey[j].km_str) + 2 ;
      }

      i++ ;
   }

   fprintf (MF,"\n\n") ;
}

/*
 * Save the macro definitions file
 */
char macro_savefile (Tn5250Macro *Macro)
{
   FILE *MFile ;
   int i;

   if (Macro->fname != NULL)
   {
      if ((MFile = fopen (Macro->fname,"wt")) != NULL)
      {
         for (i=0;i<24;i++)
         if (Macro->BuffM[i] != NULL)
            macro_write (i+1,Macro->BuffM[i],MFile) ;
   
         fclose (MFile) ;
      }
      return (1) ;
   }
   return (0) ;
}

/****f* lib5250/tn5250_macro_exit
 * NAME
 *    tn5250_macro_exit
 * SYNOPSIS
 *    tn5250_macro_exit (This)
 * INPUTS
 *    Tn5250Macro *      This       - 
 * DESCRIPTION
 *    Macro system termination
 *****/
void tn5250_macro_exit(Tn5250Macro * This)
{
   int i ;

   if (This != NULL)
   {
      /* macro_savefile (This) ; */

      if (This->fname != NULL)
         free (This->fname) ;

      for (i=0;i<24;i++)
	 free (This->BuffM[i]) ;
      free (This) ;
   }
}

/****f* lib5250/tn5250_macro_attach
 * NAME
 *    tn5250_macro_attach
 * SYNOPSIS
 *    tn5250_macro_attach (This);
 * INPUTS
 *    Tn5250Display *      This       - The display to attach to
 *    Tn5250Macro *       Macro     - The macro object to attach
 * DESCRIPTION
 *    Attach the macro system to display structure
 * TODO
 *    macro_detach ?
 *****/
int tn5250_macro_attach (Tn5250Display *This, Tn5250Macro *Macro)
{
   if ((This->macro == NULL) && (Macro != NULL))
   {
      Macro->fname = macro_filename(This) ;

      if (Macro->fname == NULL)
         TN5250_LOG (("Macro: fname NULL\n")) ;
      else
         TN5250_LOG (("Macro: fname=%s\n",Macro->fname)) ;

      /* macro_loadfile (Macro) ; */

      This->macro = Macro ;
      return (1) ;
   }
   return 0;
}

/****f* lib5250/tn5250_macro_rstate
 * NAME
 *    tn5250_macro_rstate
 * SYNOPSIS
 *    tn5250_macro_rstate (This);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Returns the current macro recording state
 *****/
char tn5250_macro_rstate (Tn5250Display *This)
{
	if (This->macro != NULL)
		return (This->macro->RState) ;
	return 0;
}

/****f* lib5250/tn5250_macro_startdef
 * NAME
 *    tn5250_macro_startdef
 * SYNOPSIS
 *    tn5250_macro_startdef (This);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Starts a macro definition
 *****/
char tn5250_macro_startdef (Tn5250Display *This)
{
	if (This->macro != NULL)
	{
	    This->macro->RState = 1 ;
	    This->macro->FctnKey = 0 ;
	    This->macro->TleBuff = MACRO_BUFSIZE ;
	    return (1) ;
	}
	return (0) ;
}

/****f* lib5250/tn5250_macro_enddef
 * NAME
 *    tn5250_macro_enddef
 * SYNOPSIS
 *    tn5250_macro_enddef (This);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Ends a macro definition
 *****/
void tn5250_macro_enddef (Tn5250Display *This)
{
   int NumMacro ;
   int *Buffer ;

	if (This->macro != NULL)
        {
	    if (This->macro->RState > 1)
	    {
	       NumMacro = This->macro->FctnKey - K_F1 ;
	       if (This->macro->TleBuff > 0)
	       {
	          This->macro->BuffM[NumMacro][This->macro->TleBuff++] = 0 ;

	          Buffer = (int *)realloc(This->macro->BuffM[NumMacro],
				This->macro->TleBuff*sizeof(int)) ;
	          if (Buffer != NULL)
	             This->macro->BuffM[NumMacro] = Buffer ;
	       }
	       else
	       {
	          free (This->macro->BuffM[NumMacro]) ;
	          This->macro->BuffM[NumMacro] = NULL ;
	       }

               macro_savefile (This->macro) ;
	    }
	    This->macro->RState = 0 ;
        }
}

/****f* lib5250/tn5250_macro_recfunct
 * NAME
 *    tn5250_macro_recfunct
 * SYNOPSIS
 *    tn5250_macro_recfunct (This,key);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 *    int 		   key     - function key received
 * DESCRIPTION
 *    Receives a function key. Return True if macro definition key
 *****/
char tn5250_macro_recfunct (Tn5250Display *This, int key)
{
   int *Buffer ;
   int NumMacro ;

	if ((This->macro != NULL) && (This->macro->RState == 1))
	{
	       Buffer = (int *)malloc((MACRO_BUFSIZE+1)*sizeof(int));
	       if (Buffer != NULL)
	       {
		  This->macro->RState = 2 ;
	          This->macro->FctnKey = key ;
		  NumMacro = key - K_F1 ;
		  if ((NumMacro >= 0) && (NumMacro < 24))
		  {
                     macro_clearmem (This->macro) ;
                     macro_loadfile (This->macro) ;

		     if (This->macro->BuffM[NumMacro] != NULL)
			free (This->macro->BuffM[NumMacro]) ;
		     This->macro->BuffM[NumMacro] = Buffer ;
		     This->macro->TleBuff = 0 ;
		     return (1) ;
		  }
		  else
		     free (Buffer) ;
	       }
	}
	return (0) ;
}

/****f* lib5250/tn5250_macro_reckey
 * NAME
 *    tn5250_macro_reckey
 * SYNOPSIS
 *    tn5250_macro_reckey (This,key);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 *    int 		   key     - key received
 * DESCRIPTION
 *    Receives a key.
 *****/
void tn5250_macro_reckey (Tn5250Display *This, int key)
{
   int NumMacro ;

	if ((This->macro != NULL) && (This->macro->RState == 2) &&
	    (key != K_MEMO))
	{
	    NumMacro = This->macro->FctnKey - K_F1 ;
	    if (This->macro->TleBuff < MACRO_BUFSIZE)
	       This->macro->BuffM[NumMacro][This->macro->TleBuff++] = key ;
	}
}

/****f* lib5250/tn5250_macro_printstate
 * NAME
 *    tn5250_macro_printstate
 * SYNOPSIS
 *    tn5250_macro_printstate (This,key);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Returns a printable macro state (always 11 char long)
 *****/
char  * tn5250_macro_printstate (Tn5250Display *This)
{
   int NumKey ;

   PState[0] = 0 ;
   if (This->macro != NULL) 
      if (This->macro->RState > 0)    /* recording state */
      {
	 if (This->macro->RState == 1)
	    sprintf (PState,"R %04i     ",MACRO_BUFSIZE-This->macro->TleBuff) ;
	 else
	 {
	    NumKey = This->macro->FctnKey - K_F1 + 1 ;
	    sprintf (PState,"R %04i  F%02i",MACRO_BUFSIZE-This->macro->TleBuff,NumKey) ;
	 }
      }
      else			      /* execution state */
      if (This->macro->EState > 0)
      {
	 if (This->macro->EState == 1)
	    sprintf (PState,"P          ") ;
	 else
	 {
	    NumKey = This->macro->FctnKey - K_F1 + 1 ;
	    sprintf (PState,"P F%02i      ",NumKey) ;
	 }
      }
   return (PState) ;
}

/****f* lib5250/tn5250_macro_estate
 * NAME
 *    tn5250_macro_estate
 * SYNOPSIS
 *    tn5250_macro_estate (This);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Returns the current macro execution state
 *****/
char tn5250_macro_estate (Tn5250Display *This)
{
	if (This->macro != NULL)
		return (This->macro->EState) ;
	return 0;
}

/****f* lib5250/tn5250_macro_startexec
 * NAME
 *    tn5250_macro_startexec
 * SYNOPSIS
 *    tn5250_macro_startexec (This);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Starts a macro execution
 *****/
char tn5250_macro_startexec (Tn5250Display *This)
{
	if (This->macro != NULL)
	{
	    This->macro->EState = 1 ;
	    This->macro->FctnKey = 0 ;
	    return (1) ;
	}
	return (0) ;
}

/****f* lib5250/tn5250_macro_endexec
 * NAME
 *    tn5250_macro_endexec
 * SYNOPSIS
 *    tn5250_macro_endexec (This);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 * DESCRIPTION
 *    Ends a macro execution
 *****/
void tn5250_macro_endexec (Tn5250Display *This)
{
	if (This->macro != NULL)
	    This->macro->EState = 0 ;
}

/****f* lib5250/tn5250_macro_execfunct
 * NAME
 *    tn5250_macro_execfunct
 * SYNOPSIS
 *    tn5250_macro_execfunct (This,key);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 *    int 		   key     - function key received
 * DESCRIPTION
 *    Receives an execution function key.
 *****/
char tn5250_macro_execfunct (Tn5250Display *This, int key)
{
   int NumMacro ;

   if ((This->macro != NULL) && (This->macro->EState == 1))
   {
      This->macro->EState = 2 ;
      This->macro->FctnKey = key ;
      NumMacro = key - K_F1 ;
      if ((NumMacro >= 0) && (NumMacro < 24))
      {
         macro_clearmem (This->macro) ;
         macro_loadfile (This->macro) ;

	 This->macro->EState = 3 ;  /* Ok to run macro */
	 This->macro->TleBuff = 0 ;
	 return (1) ;
      }
   }
   return (0) ;
}

/****f* lib5250/tn5250_macro_getkey
 * NAME
 *    tn5250_macro_execfunct
 * SYNOPSIS
 *    tn5250_macro_execfunct (This,Last);
 * INPUTS
 *    Tn5250Display *      This       - Current display
 *    char  *		   Last     - to return a "toggle indicator off"
 * DESCRIPTION
 *    Sends a key to execute
 *****/
int tn5250_macro_getkey (Tn5250Display *This, char *Last)
{
   int NumMacro ;
   int key,nkey ;

   *Last = 0 ;
   if ((This->macro != NULL) && (This->macro->EState == 3))
   {
      NumMacro = This->macro->FctnKey  - K_F1 ;
      if (This->macro->BuffM[NumMacro] != NULL)
      {
	 key = This->macro->BuffM[NumMacro][This->macro->TleBuff] ;
	 if (key != 0)
	    nkey = This->macro->BuffM[NumMacro][++This->macro->TleBuff] ;
	 
	 if ((key == 0) || (nkey == 0))
	 {
	    *Last = 1 ;
	    This->macro->EState = 0 ;
	 }

	 return (key) ;
      }
      else
      {
	 This->macro->EState = 0 ;
	 *Last = 1 ;
	 return (0) ;
      }
   }
   return (0) ;
}
