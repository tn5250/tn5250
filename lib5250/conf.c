/* tn5250 -- an implentation of the 5250 telnet protocol.
 * Copyright (C) 2000-2008 Jason M. Felice
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

#define MAX_PREFIX_STACK 50

static void tn5250_config_str_destroy (Tn5250ConfigStr * This);
static Tn5250ConfigStr *tn5250_config_str_new (const char *name,
					       const char *value);
static Tn5250ConfigStr *tn5250_config_get_str (Tn5250Config * This,
					       const char *name);
static void tn5250_config_replace_vars (char *buf, int maxlen);
static void tn5250_config_replacedata (const char *from, const char *to,
				       char *line, int maxlen);
#ifdef WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

/*** Tn5250ConfigStr ***/

static void
tn5250_config_str_destroy (Tn5250ConfigStr * This)
{
  if (This->name != NULL)
    free (This->name);
  if (This->value != NULL)
    free (This->value);
  free (This);
  return;
}

static Tn5250ConfigStr *
tn5250_config_str_new (const char *name, const char *value)
{
  Tn5250ConfigStr *This = tn5250_new (Tn5250ConfigStr, 1);
  if (This == NULL)
    return NULL;

  This->name = (char *) malloc (strlen (name) + 1);
  if (This->name == NULL)
    {
      free (This);
      return NULL;
    }
  strcpy (This->name, name);

  This->value = (char *) malloc (strlen (value) + 1);
  if (This->value == NULL)
    {
      free (This->name);
      free (This);
      return NULL;
    }
  strcpy (This->value, value);

  return This;
}

/*** Tn5250Config ***/

Tn5250Config *
tn5250_config_new ()
{
  Tn5250Config *This = tn5250_new (Tn5250Config, 1);
  if (This == NULL)
    return NULL;

  This->ref = 1;
  This->vars = NULL;

  return This;
}

Tn5250Config *
tn5250_config_ref (Tn5250Config * This)
{
  This->ref++;
  return This;
}

void
tn5250_config_unref (Tn5250Config * This)
{
  if (--This->ref == 0)
    {
      Tn5250ConfigStr *iter, *next;

      /* Destroy all vars. */
      if ((iter = This->vars) != NULL)
	{
	  do
	    {
	      next = iter->next;
	      tn5250_config_str_destroy (iter);
	      iter = next;
	    }
	  while (iter != This->vars);
	}
      free (This);
    }
  return;
}

int
tn5250_config_load (Tn5250Config * This, const char *filename)
{
  FILE *f;
  char buf[128];
  char *scan;
  int len;
  int prefix_stack_size = 0;
  char *prefix_stack[MAX_PREFIX_STACK];
  char *list[100];
  int done;
  int curitem;
  int loop;
  int name_len, i;
  char *name;

  /* It is not an error for a config file not to exist. */
  if ((f = fopen (filename, "r")) == NULL)
    return errno == ENOENT ? 0 : -1;

  while (fgets (buf, sizeof (buf) - 1, f))
    {
      buf[sizeof (buf) - 1] = '\0';
      if (strchr (buf, '\n'))
	*strchr (buf, '\n') = '\0';

      tn5250_config_replace_vars (buf, sizeof (buf));

      scan = buf;
      while (*scan && isspace (*scan))
	scan++;

      if (*scan == '#' || *scan == '\0')
	continue;

      if (*scan == '+')
	{
	  scan++;
	  while (*scan && isspace (*scan))
	    scan++;
	  len = strlen (scan) - 1;
	  while (len > 0 && isspace (scan[len]))
	    scan[len--] = '\0';

	  for (i = 0, name_len = len + 3; i < prefix_stack_size; i++)
	    {
	      name_len += strlen (prefix_stack[i]) + 1;
	    }
	  if ((name = (char *) malloc (name_len)) == NULL)
	    goto config_error;
	  name[0] = '\0';
	  for (i = 0; i < prefix_stack_size; i++)
	    {
	      strcat (name, prefix_stack[i]);
	      strcat (name, ".");
	    }
	  strcat (name, scan);
	  tn5250_config_set (This, name, "1");
	  free (name);

	}
      else if (*scan == '-')
	{
	  scan++;
	  while (*scan && isspace (*scan))
	    scan++;
	  len = strlen (scan) - 1;
	  while (len > 0 && isspace (scan[len]))
	    scan[len--] = '\0';
	  for (i = 0, name_len = len + 3; i < prefix_stack_size; i++)
	    {
	      name_len += strlen (prefix_stack[i]) + 1;
	    }
	  if ((name = (char *) malloc (name_len)) == NULL)
	    goto config_error;
	  name[0] = '\0';
	  for (i = 0; i < prefix_stack_size; i++)
	    {
	      strcat (name, prefix_stack[i]);
	      strcat (name, ".");
	    }
	  strcat (name, scan);
	  tn5250_config_set (This, name, "0");
	  free (name);

	}
      else if (strchr (scan, '='))
	{

	  /* Set item. */

	  len = 0;
	  while (scan[len] && !isspace (scan[len]) && scan[len] != '=')
	    len++;
	  if (len == 0)
	    goto config_error;	/* Missing variable name. */

	  for (i = 0, name_len = len + 3; i < prefix_stack_size; i++)
	    {
	      name_len += strlen (prefix_stack[i]) + 1;
	    }

	  if ((name = (char *) malloc (name_len)) == NULL)
	    goto config_error;

	  name[0] = '\0';
	  for (i = 0; i < prefix_stack_size; i++)
	    {
	      strcat (name, prefix_stack[i]);
	      strcat (name, ".");
	    }
	  name_len = strlen (name) + len;
	  memcpy (name + strlen (name), scan, len);
	  name[name_len] = '\0';

	  scan += len;
	  while (*scan && isspace (*scan))
	    scan++;

	  if (*scan != '=')
	    {
	      free (name);
	      goto config_error;
	    }
	  scan++;

	  while (*scan && isspace (*scan))
	    scan++;

	  if (*scan == '[')
	    {
	      done = 0;
	      curitem = 0;
	      while (!done)
		{
		  fgets (buf, sizeof (buf) - 1, f);
		  buf[sizeof (buf) - 1] = '\0';
		  if (strchr (buf, '\n'))
		    *strchr (buf, '\n') = '\0';

		  scan = buf;

		  while (*scan && isspace (*scan))
		    scan++;

		  if (*scan == ']')
		    {
		      done = 1;
		    }
		  else
		    {
		      list[curitem] = malloc (strlen (scan) + 1);
		      strcpy (list[curitem], scan);
		      curitem++;
		    }

		}
	      for (loop = 0; loop < curitem; loop++)
		{
		  printf ("%s\n", list[loop]);
		}


	    }
	  else
	    {

	      len = strlen (scan) - 1;
	      while (len > 0 && isspace (scan[len]))
		scan[len--] = '\0';

	      tn5250_config_set (This, name, scan);
	      free (name);
	    }
	}
      else if (strchr (scan, '{'))
	{

	  /* Push level. */

	  len = 0;
	  while (scan[len] && !isspace (scan[len]) && scan[len] != '{')
	    len++;
	  if (len == 0)
	    goto config_error;	/* Missing section name. */

	  TN5250_ASSERT (prefix_stack_size < MAX_PREFIX_STACK);
	  prefix_stack[prefix_stack_size] = (char *) malloc (len + 1);
	  TN5250_ASSERT (prefix_stack[prefix_stack_size] != NULL);

	  memcpy (prefix_stack[prefix_stack_size], scan, len);
	  prefix_stack[prefix_stack_size][len] = '\0';

	  prefix_stack_size++;

	}
      else if (*scan == '}')
	{

	  /* Pop level. */

	  scan++;
	  while (*scan && isspace (*scan))
	    scan++;
	  if (*scan != '#' && *scan != '\0')
	    goto config_error;	/* Garbage after '}' */

	  TN5250_ASSERT (prefix_stack_size > 0);
	  prefix_stack_size--;

	  if (prefix_stack[prefix_stack_size] != NULL)
	    free (prefix_stack[prefix_stack_size]);

	}
      else
	goto config_error;	/* Garbage line. */
    }

  if (prefix_stack_size != 0)
    goto config_error;

  fclose (f);
  return 0;

config_error:

  while (prefix_stack_size > 0)
    {
      prefix_stack_size--;
      if (prefix_stack[prefix_stack_size] != NULL)
	free (prefix_stack[prefix_stack_size]);
    }
  fclose (f);
  return -1;
}

#ifdef __WIN32__
/*
 *    Load default configuration files.
 *    Win32 version -- Looks for a file in the same dir as the .exe
 *            file called "tn5250rc".
 */
int
tn5250_config_load_default (Tn5250Config * This)
{
#define PATHSIZE 1024
  LPTSTR apppath;
  LPTSTR dir;
  DWORD rc;
  DWORD len;
  LPTSTR lpMsgBuf;

  apppath = malloc (PATHSIZE + 1);
  TN5250_ASSERT (apppath != NULL);

  if (GetModuleFileName (NULL, apppath, PATHSIZE) < 1)
    {
      FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
		     FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError (),
		     MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), lpMsgBuf, 0,
		     NULL);
      TN5250_LOG (("GetModuleFileName Error: %s\n", lpMsgBuf));
      MessageBox (NULL, lpMsgBuf, "TN5250", MB_OK);
      LocalFree (lpMsgBuf);
      return -1;
    }

  if (strrchr (apppath, '\\'))
    {
      len = strrchr (apppath, '\\') - apppath;
      apppath[len + 1] = '\0';
    }

  dir = malloc (strlen (apppath) + 15);
  TN5250_ASSERT (apppath != NULL);

  strcpy (dir, apppath);
  strcat (dir, "tn5250rc");
  free (apppath);

  TN5250_LOG (("Config File = %s\n", dir));

  rc = tn5250_config_load (This, dir);
  free (dir);
  return rc;
}

#else

/*
 *    Load default configuration files.
 */
int
tn5250_config_load_default (Tn5250Config * This)
{
  struct passwd *pwent;
  char *dir;
  int ec;

  if (tn5250_config_load (This, SYSCONFDIR "/tn5250rc") == -1)
    {
      perror (SYSCONFDIR "/tn5250rc");
      return -1;
    }

  pwent = getpwuid (getuid ());
  if (pwent == NULL)
    {
      perror ("getpwuid");
      return -1;
    }

  dir = (char *) malloc (strlen (pwent->pw_dir) + 12);
  if (dir == NULL)
    {
      perror ("malloc");
      return -1;
    }

  strcpy (dir, pwent->pw_dir);
  strcat (dir, "/.tn5250rc");
  if ((ec = tn5250_config_load (This, dir)) == -1)
    perror (dir);
  free (dir);
  return ec;
}
#endif

int
tn5250_config_parse_argv (Tn5250Config * This, int argc, char **argv)
{
  int argn = 1;

  /* FIXME: Scan and promote entries first, then parse individual
   * settings.   This is so that we can use a particular session but
   * override one of it's settings. */

  while (argn < argc)
    {
      if (argv[argn][0] == '+')
	{
	  /* Set boolean option. */
	  char *opt = argv[argn] + 1;
	  tn5250_config_set (This, opt, "1");
	}
      else if (argv[argn][0] == '-')
	{
	  /* Clear boolean option. */
	  char *opt = argv[argn] + 1;
	  tn5250_config_set (This, opt, "0");
	}
      else if (strchr (argv[argn], '='))
	{
	  /* Set string option. */
	  char *opt;
	  char *val = strchr (argv[argn], '=') + 1;
	  opt = (char *) malloc (strchr (argv[argn], '=') - argv[argn] + 3);
	  if (opt == NULL)
	    return -1;		/* FIXME: Produce error message. */
	  memcpy (opt, argv[argn], strchr (argv[argn], '=') - argv[argn] + 1);
	  *strchr (opt, '=') = '\0';
	  tn5250_config_set (This, opt, val);
	}
      else
	{
	  /* Should be profile name/connect URL. */
	  tn5250_config_set (This, "host", argv[argn]);
	  tn5250_config_promote (This, argv[argn]);
	}
      argn++;
    }

  return 0;
}

const char *
tn5250_config_get (Tn5250Config * This, const char *name)
{
  Tn5250ConfigStr *str = tn5250_config_get_str (This, name);
  return (str == NULL ? NULL : str->value);
}

int
tn5250_config_get_bool (Tn5250Config * This, const char *name)
{
  const char *v = tn5250_config_get (This, name);
  return (v == NULL ? 0 :
	  !(!strcmp (v, "off")
	    || !strcmp (v, "no")
	    || !strcmp (v, "0") || !strcmp (v, "false")));
}

int
tn5250_config_get_int (Tn5250Config * This, const char *name)
{
  const char *v = tn5250_config_get (This, name);

  if (v == NULL)
    {
      return 0;
    }
  else
    {
      return (atoi (v));
    }

}

void
tn5250_config_set (Tn5250Config * This, const char *name, const char *value)
{
  Tn5250ConfigStr *str = tn5250_config_get_str (This, name);

  if (str != NULL)
    {
      if (str->value != NULL)
	free (str->value);
      str->value = (char *) malloc (strlen (value) + 1);
      TN5250_ASSERT (str->value != NULL);
      strcpy (str->value, value);
      return;
    }

  str = tn5250_config_str_new (name, value);
  if (This->vars == NULL)
    This->vars = str->next = str->prev = str;
  else
    {
      str->next = This->vars;
      str->prev = This->vars->prev;
      str->next->prev = str;
      str->prev->next = str;
    }
}

void
tn5250_config_unset (Tn5250Config * This, const char *name)
{
  Tn5250ConfigStr *str;

  if ((str = tn5250_config_get_str (This, name)) == NULL)
    return;			/* Not found */

  if (This->vars == str)
    This->vars = This->vars->next;
  if (This->vars == str)
    This->vars = NULL;
  else
    {
      str->next->prev = str->prev;
      str->prev->next = str->next;
    }
  tn5250_config_str_destroy (str);
}

/*
 *    Copy variables prefixed with `prefix' to variables without `prefix'.
 */
void
tn5250_config_promote (Tn5250Config * This, const char *prefix)
{
  Tn5250ConfigStr *iter;
  if ((iter = This->vars) == NULL)
    return;
  do
    {
      if (strlen (prefix) <= strlen (iter->name) + 2
	  && !memcmp (iter->name, prefix, strlen (prefix))
	  && iter->name[strlen (prefix)] == '.')
	{
	  tn5250_config_set (This, iter->name + strlen (prefix) + 1,
			     iter->value);
	}
      iter = iter->next;
    }
  while (iter != This->vars);
}

static Tn5250ConfigStr *
tn5250_config_get_str (Tn5250Config * This, const char *name)
{
  Tn5250ConfigStr *iter;

  if ((iter = This->vars) == NULL)
    return NULL;		/* No vars */

  do
    {
      if (!strcmp (iter->name, name))
	return iter;
      iter = iter->next;
    }
  while (iter != This->vars);

  return NULL;			/* Not found */
}


/****f* lib5250/tn5250_config_replace_vars
 * NAME
 *    tn5250_config_replace_vars
 * SYNOPSIS
 *    tn5250_config_replace_vars (buf, 128);
 * INPUTS
 *    char        *      buf          -
 *    int                maxlen       -
 * DESCRIPTION
 *    This searches for special "replacement variables" in the
 *    config file entries and converts them to their "real values".
 *    (At this time, the only var is "$loginname$".)
 *****/
#define USER_NAME_MAX 50
static void
tn5250_config_replace_vars (char *buf, int maxlen)
{

#ifdef WIN32
  {
    DWORD len;
    char usrnam[USER_NAME_MAX + 1];
    len = USER_NAME_MAX;
    if (GetUserName (usrnam, &len) != 0)
      {
	tn5250_config_replacedata ("$loginname$", usrnam, buf, maxlen);
      }
  }
#else
  {
    struct passwd *pwent;
    pwent = getpwuid (getuid ());
    if (pwent != NULL)
      {
	tn5250_config_replacedata ("$loginname$", pwent->pw_name, buf,
				   maxlen);
      }
  }
#endif

  return;
}


/****f* lib5250/tn5250_config_replacedata
 * NAME
 *    tn5250_config_replacedata
 * SYNOPSIS
 *    tn5250_config_replacedata ("$loginname$", "fred", line, sizeof(line));
 * INPUTS
 *    const char  *      from         -
 *    const char  *      to           -
 *    char        *      line         -
 *    int                maxlen       -
 * DESCRIPTION
 *    This will replace the first occurrance of the "from" string with 
 *    the "to" string in a line of text.  The from and to do not have to 
 *    be the same length. 
 *****/
static void
tn5250_config_replacedata (const char *from, const char *to,
			   char *line, int maxlen)
{

  char *p;
  int len;
  char *before, *after;

  if ((p = strstr (line, from)) != NULL)
    {
      if (p <= line)
	{
	  before = malloc (1);
	  *before = '\0';
	}
      else
	{
	  len = p - line;
	  before = malloc (len + 1);
	  memcpy (before, line, len);
	  before[len] = '\0';
	}
      p += strlen (from);
      if (strlen (p) < 1)
	{
	  after = malloc (1);
	  *after = '\0';
	}
      else
	{
	  len = strlen (p);
	  after = malloc (len + 1);
	  memcpy (after, p, len);
	  after[len] = '\0';
	}
      snprintf (line, maxlen - 1, "%s%s%s", before, to, after);
      free (before);
      free (after);
    }

}

/* vi:set sts=3 sw=3: */
