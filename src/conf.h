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
#ifndef CONF_H
#define CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_STRING 1
#define CONFIG_LIST   2

struct _Tn5250ConfigStr {
  char * name;
  int    type;
  gpointer value;
};

typedef struct _Tn5250ConfigStr Tn5250ConfigStr;

struct _Tn5250Config {
  int	   ref;
  GSList * vars;
};

typedef struct _Tn5250Config Tn5250Config;

extern Tn5250Config *         tn5250_config_new		 (void);
extern Tn5250Config *	      tn5250_config_ref		 (Tn5250Config *This);
extern void		      tn5250_config_unref  	 (Tn5250Config *This);

extern int		      tn5250_config_load	 (Tn5250Config *This,
							  const char *filen);
extern int		      tn5250_config_load_default (Tn5250Config *This);
extern int		      tn5250_config_parse_argv	 (Tn5250Config *This,
							  int argc,
							  char **argv);

extern const char *	      tn5250_config_get		 (Tn5250Config *This,
							  const char *name);
extern int		      tn5250_config_get_bool	 (Tn5250Config *This,
						          const char *name);
extern int                    tn5250_config_get_int      (Tn5250Config *This,
							  const char *name);
extern void		      tn5250_config_set		 (Tn5250Config *This,
							  const char *name,
							  const int  type,
							  const gpointer value);
extern void		      tn5250_config_unset	 (Tn5250Config *This,
							  const char *name);

extern void		      tn5250_config_promote	 (Tn5250Config *This,
							  const char *prefix);

#ifdef __cplusplus
}
#endif

#endif				/* CONF_H */

/* vi:set sts=3 sw=3: */
