/* tn5250 -- an implentation of the 5250 telnet protocol.
 * Copyright (C) 2000 Jason M. Felice
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "tn5250-private.h"

#define MAX_PREFIX_STACK 50

static void tn5250_config_str_destroy (Tn5250ConfigStr *This);
static Tn5250ConfigStr *tn5250_config_str_new (const char *name, const char *value);
static Tn5250ConfigStr *tn5250_config_get_str (Tn5250Config *This, const char *name);

/*** Tn5250ConfigStr ***/

static void tn5250_config_str_destroy (Tn5250ConfigStr *This)
{
   if (This->name != NULL)
      free (This->name);
   if (This->value != NULL)
      free (This->value);
   free (This);
}

static Tn5250ConfigStr *tn5250_config_str_new (const char *name, const char *value)
{
   Tn5250ConfigStr *This = tn5250_new (Tn5250ConfigStr, 1);
   if (This == NULL)
      return NULL;

   This->name = (char *)malloc (strlen (name)+1);
   if (This->name == NULL) {
      free (This);
      return NULL;
   }
   strcpy (This->name, name);

   This->value = (char *)malloc (strlen (value)+1);
   if (This->value == NULL) {
      free (This->name);
      free (This);
      return NULL;
   }
   strcpy (This->value, value);

   return This;
}

/*** Tn5250Config ***/

Tn5250Config *tn5250_config_new ()
{
   Tn5250Config *This = tn5250_new (Tn5250Config, 1);
   if (This == NULL)
      return NULL;

   This->ref = 1;
   This->vars = NULL;

   return This;
}

Tn5250Config *tn5250_config_ref (Tn5250Config *This)
{
   This->ref++;
   return This;
}

void tn5250_config_unref (Tn5250Config *This)
{
   if (-- This->ref == 0) {
      Tn5250ConfigStr *iter, *next;

      /* Destroy all vars. */
      if ((iter = This->vars) != NULL) {
	 do {
	    next = iter->next;
	    tn5250_config_str_destroy (iter);
	    iter = next;
	 } while (iter != This->vars);
      }
      free (This);
   }
}

int tn5250_config_load (Tn5250Config *This, const char *filename)
{
   FILE *f;
   char buf[128];
   char *scan;
   int len;
   int prefix_stack_size = 0;
   char *prefix_stack[MAX_PREFIX_STACK];
   char * list[100]; 
   int done;
   int curitem;
   int loop;
   int name_len, i;
   char *name;

   /* It is not an error for a config file not to exist. */
   if ((f = fopen (filename, "r")) == NULL)
      return errno == ENOENT ? 0 : -1;

   while (fgets (buf, sizeof(buf)-1, f)) {
      buf[sizeof (buf)-1] = '\0';
      if (strchr (buf, '\n'))
	 *strchr (buf, '\n') = '\0';

      scan = buf;
      while (*scan && isspace (*scan))
	 scan++;

      if (*scan == '#' || *scan == '\0')
	 continue;

      if (*scan == '+') {
	 scan++;
	 while (*scan && isspace (*scan))
	    scan++;
	 len = strlen (scan) - 1;
	 while (len > 0 && isspace (scan[len]))
	    scan[len--] = '\0';

	 for (i = 0, name_len = len + 3; i < prefix_stack_size; i++) {
	    name_len += strlen (prefix_stack[i]) + 1;
	 }
	 if ((name = (char*)malloc (name_len)) == NULL)
	    goto config_error;
	 name[0] = '\0';
	 for (i = 0; i < prefix_stack_size; i++) {
	    strcat (name, prefix_stack[i]);
	    strcat (name, ".");
	 }
         strcat(name, scan);
	 tn5250_config_set (This, name, "1");
	 free (name);

      } else if (*scan == '-') {
	 scan++;
	 while (*scan && isspace (*scan))
	    scan++;
	 len = strlen (scan) - 1;
	 while (len > 0 && isspace (scan[len]))
	    scan[len--] = '\0';
	 for (i = 0, name_len = len + 3; i < prefix_stack_size; i++) {
	    name_len += strlen (prefix_stack[i]) + 1;
	 }
	 if ((name = (char*)malloc (name_len)) == NULL)
	    goto config_error;
	 name[0] = '\0';
	 for (i = 0; i < prefix_stack_size; i++) {
	    strcat (name, prefix_stack[i]);
	    strcat (name, ".");
	 }
         strcat(name, scan);
	 tn5250_config_set (This, name, "0");
	 free (name);

      } else if (strchr (scan, '=')) {

	 /* Set item. */

	 len = 0;
	 while (scan[len] && !isspace (scan[len]) && scan[len] != '=')
	    len++;
	 if (len == 0)
	    goto config_error; /* Missing variable name. */

	 for (i = 0, name_len = len + 3; i < prefix_stack_size; i++) {
	    name_len += strlen (prefix_stack[i]) + 1;
	 }

	 if ((name = (char*)malloc (name_len)) == NULL)
	    goto config_error;

	 name[0] = '\0';
	 for (i = 0; i < prefix_stack_size; i++) {
	    strcat (name, prefix_stack[i]);
	    strcat (name, ".");
	 }
	 name_len = strlen(name) + len;
	 memcpy (name + strlen (name), scan, len);
	 name[name_len] = '\0';

	 scan += len;
	 while (*scan && isspace (*scan))
	    scan++;

	 if (*scan != '=') {
	    free (name);
	    goto config_error;
	 }
	 scan++;

	 while (*scan && isspace (*scan))
	   scan++;

	 if(*scan == '[') {
	   done = 0;
	   curitem = 0;
	   while(!done) {
	     fgets (buf, sizeof(buf)-1, f);
	     buf[sizeof (buf)-1] = '\0';
	     if (strchr (buf, '\n'))
	       *strchr (buf, '\n') = '\0';
	     
	     scan = buf;

	     while(*scan && isspace(*scan))
	       scan++;

	     if(*scan == ']') {
	       done = 1;
	     } else {
		list[curitem] = malloc(strlen(scan)+1);
		strcpy(list[curitem],scan);
		curitem++;
	     }
	
	   }
	   for(loop=0;loop < curitem; loop++) {
	     printf("%s\n", list[loop]);
	   }


	 } else {

	   len = strlen (scan) - 1;
	   while (len > 0 && isspace (scan[len]))
	     scan[len--] = '\0';

	   tn5250_config_set (This, name, scan);
	   free (name);
	 }
      } else if (strchr (scan, '{')) {

	 /* Push level. */

	 len = 0;
	 while (scan[len] && !isspace (scan[len]) && scan[len] != '{')
	    len++;
	 if (len == 0)
	    goto config_error; /* Missing section name. */

	 TN5250_ASSERT (prefix_stack_size < MAX_PREFIX_STACK);
	 prefix_stack[prefix_stack_size] = (char*)malloc (len+1);
	 TN5250_ASSERT (prefix_stack[prefix_stack_size] != NULL);

	 memcpy (prefix_stack[prefix_stack_size], scan, len);
	 prefix_stack[prefix_stack_size][len] = '\0';

	 prefix_stack_size++;

      } else if (*scan == '}') {

	 /* Pop level. */

	 scan++;
	 while (*scan && isspace (*scan))
	    scan++;
	 if (*scan != '#' && *scan != '\0')
	    goto config_error; /* Garbage after '}' */

	 TN5250_ASSERT (prefix_stack_size > 0);
	 prefix_stack_size--;

	 if (prefix_stack[prefix_stack_size] != NULL)
	    free (prefix_stack[prefix_stack_size]);

      } else
	 goto config_error; /* Garbage line. */
   }

   if (prefix_stack_size != 0)
      goto config_error;

   fclose (f);
   return 0;

config_error:

   while (prefix_stack_size > 0) {
      prefix_stack_size--;
      if (prefix_stack[prefix_stack_size] != NULL)
	 free (prefix_stack[prefix_stack_size]);
   }
   fclose (f);
   return -1;
}

/*
 *    Load default configuration files.
 */
int tn5250_config_load_default (Tn5250Config *This)
{
   struct passwd *pwent;
   char *dir;
   int ec;

   if (tn5250_config_load (This, SYSCONFDIR "/tn5250rc") == -1) {
      perror (SYSCONFDIR "/tn5250rc");
      return -1;
   }
   
   pwent = getpwuid (getuid ());
   if (pwent == NULL) {
      perror ("getpwuid");
      return -1;
   }

   dir = (char *)malloc (strlen (pwent->pw_dir) + 12);
   if (dir == NULL) {
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

int tn5250_config_parse_argv (Tn5250Config *This, int argc, char **argv)
{
   int argn = 1, pos = -1;

   /* FIXME: Scan and promote entries first, then parse individual
    * settings.   This is so that we can use a particular session but
    * override one of it's settings. */

   while (argn < argc) {
      if (argv[argn][0] == '+') {
	 /* Set boolean option. */
	 char *opt = argv[argn] + 1;
	 tn5250_config_set (This, opt, "1");
      } else if (argv[argn][0] == '-') {
	 /* Clear boolean option. */
	 char *opt = argv[argn] + 1;
	 tn5250_config_set (This, opt, "0");
      } else if (strchr (argv[argn], '=')) {
	 /* Set string option. */
	 char *opt;
	 char *val = strchr (argv[argn], '=') + 1;
	 opt = (char*)malloc (strchr(argv[argn], '=') - argv[argn] + 3);
	 if (opt == NULL)
	    return -1; /* FIXME: Produce error message. */
	 memcpy (opt, argv[argn], strchr(argv[argn], '=') - argv[argn] + 1);
	 *strchr (opt, '=') = '\0';
	 tn5250_config_set (This, opt, val);
      } else {
	 /* Should be profile name/connect URL. */
	 tn5250_config_set(This, "host", argv[argn]);
	 tn5250_config_promote(This, argv[argn]);
      }
      argn++;
   }

   return 0;
}

const char *tn5250_config_get (Tn5250Config *This, const char *name)
{
   Tn5250ConfigStr *str = tn5250_config_get_str (This, name);
   return (str == NULL ? NULL : str->value);
}

int tn5250_config_get_bool (Tn5250Config *This, const char *name)
{
   const char *v = tn5250_config_get (This, name);
   return (v == NULL ? 0 :
	 !(!strcmp (v,"off")
	    || !strcmp (v, "no")
	    || !strcmp (v,"0")
	    || !strcmp (v,"false")));
}

int
tn5250_config_get_int (Tn5250Config *This, const char *name)
{
   const char *v = tn5250_config_get (This, name);
  
   if(v == NULL) {
     return 0;
   } else {
     return(atoi(v));
   }

}

void tn5250_config_set (Tn5250Config *This, const char *name, const char *value)
{
   Tn5250ConfigStr *str = tn5250_config_get_str (This, name);

   if (str != NULL) {
      if (str->value != NULL)
	 free (str->value);
      str->value = (char*)malloc (strlen (value) + 1);
      TN5250_ASSERT (str->value != NULL);
      strcpy (str->value, value);
      return;
   }

   str = tn5250_config_str_new (name,value);
   if (This->vars == NULL)
      This->vars = str->next = str->prev = str;
   else {
      str->next = This->vars;
      str->prev = This->vars->prev;
      str->next->prev = str;
      str->prev->next = str;
   }
}

void tn5250_config_unset (Tn5250Config *This, const char *name)
{
   Tn5250ConfigStr *str;

   if ((str = tn5250_config_get_str (This, name)) == NULL)
      return; /* Not found */

   if (This->vars == str)
      This->vars = This->vars->next;
   if (This->vars == str)
      This->vars = NULL;
   else {
      str->next->prev = str->prev;
      str->prev->next = str->next;
   }
   tn5250_config_str_destroy (str);
}

/*
 *    Copy variables prefixed with `prefix' to variables without `prefix'.
 */
void tn5250_config_promote (Tn5250Config *This, const char *prefix)
{
   Tn5250ConfigStr *iter;
   if ((iter = This->vars) == NULL)
      return;
   do {
      if (strlen(prefix) <= strlen(iter->name) + 2
	    && !memcmp (iter->name, prefix, strlen(prefix))
	    && iter->name[strlen(prefix)] == '.') {
	 tn5250_config_set (This, iter->name + strlen(prefix) + 1, iter->value);
      }
      iter = iter->next;
   } while (iter != This->vars);
}

static Tn5250ConfigStr *tn5250_config_get_str (Tn5250Config *This, const char *name)
{
   Tn5250ConfigStr *iter;

   if ((iter = This->vars) == NULL)
      return NULL; /* No vars */

   do {
      if (!strcmp (iter->name, name))
	 return iter;
      iter = iter->next;
   } while (iter != This->vars);

   return NULL; /* Not found */
}

/* vi:set sts=3 sw=3: */

