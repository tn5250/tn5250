%module _tn5250
%{
#include "tn5250-private.h"
%}

%inline %{
char **tn5250_argv_new (int len) {
	return (char**)malloc (sizeof (char**)*len);
}
void tn5250_argv_free (char **argv) {
	free (argv);
}
void tn5250_argv_set (char **argv, int i, char *str) {
	argv[i] = str;
}
int tn5250_config_exists (Tn5250Config *cfg, const char *name) {
	return !!tn5250_config_get (cfg, name);
}
void tn5250_terminal_set_conn_fd (Tn5250Terminal *term, int fd) {
	term->conn_fd = fd;
}
int tn5250_stream_env_exists (Tn5250Stream *This, const char *name) {
    return !!tn5250_stream_getenv (This, name);
}
%}

