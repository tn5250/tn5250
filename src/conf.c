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
static Tn5250ConfigStr *tn5250_config_str_new (const char *name, 
					       const int type,
					       const gpointer value);
static Tn5250ConfigStr *tn5250_config_get_str (Tn5250Config *This, const char *name);

/*** Tn5250ConfigStr ***/

static void tn5250_config_str_destroy (Tn5250ConfigStr *This)
{
  GSList * iter;

  g_free (This->name);
  
  if(This->type == CONFIG_STRING) {
    g_free (This->value);
  } else {
    iter = This->value;
    while(iter != NULL)
      {
	g_free(iter->data);
	iter = g_slist_next(iter);
      }
    g_slist_free(This->value);
  }
  free (This);
}

static Tn5250ConfigStr *tn5250_config_str_new (const char *name, 
					       const int type,
					       const gpointer value)
{
   Tn5250ConfigStr *This = g_new (Tn5250ConfigStr, 1);

   This->name = (char *)g_malloc (strlen (name)+1);

   strcpy (This->name, name);

   This->type = type;

   if(This->type == CONFIG_STRING) {
     This->value = (char *)g_malloc (strlen (value)+1);

     strcpy (This->value, value);
   } else {
     This->value = value;
   }

   return This;
}

/*** Tn5250Config ***/

Tn5250Config *tn5250_config_new ()
{
   Tn5250Config *This = g_new (Tn5250Config, 1);

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
  GSList * next;
  
  if(-- This->ref == 0) {
    next = This->vars;

    while(next != NULL)
      {
	tn5250_config_str_destroy(next->data);
	next = g_slist_next(next);
      }
    g_slist_free(This->vars);
    g_free(This);
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
   GSList * list; 
   int done;
   char * curitem;
   GSList * temp;

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
	 tn5250_config_set (This, scan, CONFIG_STRING, "1");

      } else if (*scan == '-') {
	 scan++;
	 while (*scan && isspace (*scan))
	    scan++;
	 len = strlen (scan) - 1;
	 while (len > 0 && isspace (scan[len]))
	    scan[len--] = '\0';
	 tn5250_config_set (This, scan, CONFIG_STRING, "0");

      } else if (strchr (scan, '=')) {
	 int name_len, i;
	 char *name;

	 /* Set item. */

	 len = 0;
	 while (scan[len] && !isspace (scan[len]) && scan[len] != '=')
	    len++;
	 if (len == 0)
	    goto config_error; /* Missing variable name. */

	 for (i = 0, name_len = len + 3; i < prefix_stack_size; i++) {
	    name_len += strlen (prefix_stack[i]) + 1;
	 }

	 name = (char*)g_malloc (name_len);

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
	    g_free (name);
	    goto config_error;
	 }
	 scan++;

	 while (*scan && isspace (*scan))
	   scan++;

	 if(*scan == '[') {
	   done = 0;
	   list = NULL;
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
		curitem = g_malloc(strlen(scan)+1);
		strcpy(curitem, scan);
		curitem[strlen(curitem)] = '\0';
		list = g_slist_append(list, curitem);
	     }
	   }
	   tn5250_config_set(This, name, CONFIG_LIST, (gpointer)list); 
	     
	 } else {

	   len = strlen (scan) - 1;
	   while (len > 0 && isspace (scan[len]))
	     scan[len--] = '\0';

	   tn5250_config_set (This, name, CONFIG_STRING, scan);
	   g_free (name);
	 }
      } else if (strchr (scan, '{')) {

	 /* Push level. */

	 len = 0;
	 while (scan[len] && !isspace (scan[len]) && scan[len] != '{')
	    len++;
	 if (len == 0)
	    goto config_error; /* Missing section name. */

	 TN5250_ASSERT (prefix_stack_size < MAX_PREFIX_STACK);
	 prefix_stack[prefix_stack_size] = (char*)g_malloc (len+1);

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

	 g_free (prefix_stack[prefix_stack_size]);

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
      g_free (prefix_stack[prefix_stack_size]);
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

   dir = (char *)g_malloc (strlen (pwent->pw_dir) + 12);

   strcpy (dir, pwent->pw_dir);
   strcat (dir, "/.tn5250rc");
   if ((ec = tn5250_config_load (This, dir)) == -1)
      perror (dir);
   g_free (dir);

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
	 tn5250_config_set (This, opt, CONFIG_STRING, "1");
      } else if (argv[argn][0] == '-') {
	 /* Clear boolean option. */
	 char *opt = argv[argn] + 1;
	 tn5250_config_set (This, opt, CONFIG_STRING, "0");
      } else if (strchr (argv[argn], '=')) {
	 /* Set string option. */
	 char *opt;
	 char *val = strchr (argv[argn], '=') + 1;
	 opt = (char*)g_malloc (strchr(argv[argn], '=') - argv[argn] + 3);

	 memcpy (opt, argv[argn], strchr(argv[argn], '=') - argv[argn] + 1);
	 *strchr (opt, '=') = '\0';
	 tn5250_config_set (This, opt, CONFIG_STRING, val);
      } else {
	 /* Should be profile name/connect URL. */
	 tn5250_config_set(This, "host", CONFIG_STRING, argv[argn]);
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

void tn5250_config_set (Tn5250Config *This, const char *name, 
			const int type, const  gpointer value)
{
   Tn5250ConfigStr *str = tn5250_config_get_str (This, name);

   if (str != NULL) {
     if(str->type == CONFIG_STRING) {
       g_free (str->value);
       str->value = (char*)g_malloc (strlen (value) + 1);

       strcpy (str->value, value);
       return;
     }
   }

   str = tn5250_config_str_new (name,type, value);

   This->vars = g_slist_append(This->vars, str);
}

void tn5250_config_unset (Tn5250Config *This, const char *name)
{
   Tn5250ConfigStr *str;

   if ((str = tn5250_config_get_str (This, name)) == NULL)
      return; /* Not found */

   This->vars = g_slist_remove(This->vars, str);

   tn5250_config_str_destroy (str);
}

/*
 *    Copy variables prefixed with `prefix' to variables without `prefix'.
 */
void tn5250_config_promote (Tn5250Config *This, const char *prefix)
{
   GSList * iter;
   Tn5250ConfigStr * data;

   if ((iter = This->vars) == NULL)
      return;

   do {
     data = (Tn5250ConfigStr *)iter->data;
     if (strlen(prefix) <= strlen( data->name ) + 2
	 && !memcmp(data->name, prefix, strlen(prefix))
	 && data->name[strlen(prefix)] == '.') {
       tn5250_config_set(This, data->name 
			 + strlen(prefix) + 1,
			 data->type,
			 data->value);
     } 
     iter = g_slist_next(iter);
   } while(iter != NULL);

}

static Tn5250ConfigStr *tn5250_config_get_str (Tn5250Config *This, const char *name)
{
  GSList * node;
  Tn5250ConfigStr * result;

  node = This->vars;

  if ( node == NULL)
    {
      return(NULL);
    }

  do {
    result = (Tn5250ConfigStr *)node->data;

    if( !strcmp(result->name, name) )
      {
	return(result);
      }
    node = g_slist_next(node);
  } while(node != NULL);

  return(NULL);
	  

}

/* vi:set sts=3 sw=3: */

