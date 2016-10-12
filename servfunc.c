#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "servfunc.h"

extern char *progname;
enum {
	INITMP = 1,
	GROWMP = 2,
	MAX_LINE_SIZE = 2048
};

// Функции для сигнализации ошибок
void 
err_sys_quit(const char *str)
{
	perror(str);
    exit(EXIT_FAILURE);
}

void 
err_comm_quit(const char *str)
{
	fprintf(stderr, "%s: %s\n", progname, str);
    exit(EXIT_FAILURE);
}

// sig_chld() - функция обработчик для сигнала SIGCHLD
void
sig_chld(int signo)
{
	pid_t pid;
	int stat;
	
	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		;
	return;
}

// merge_str() - функция объединяет две строки, с выделением памяти
char *
merge_str(const char *name, const char *val)
{
	char *tmp_str = NULL;
	int len = 0;
	
	if ( (name == NULL) && (val == NULL))
		tmp_str = NULL;
	else if ( (name != NULL) && (val == NULL))
		tmp_str = strdup(name);
	else if ( (name == NULL) && (val != NULL))
		tmp_str = strdup(val);
	else { 	
		len = strlen(name) + strlen(val) + 1;
		tmp_str = realloc(NULL, len);		
		if (tmp_str != NULL)
			snprintf(tmp_str, len, "%s%s", name, val);
	}
	return tmp_str;
}
