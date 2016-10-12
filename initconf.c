#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "servfunc.h"
#include "initconf.h"
#include "keyval.h"
#include "readstr.h"

extern char *progname;
extern char *confpath;
struct set_keyval conf, state, mime;

struct keyval defconf[] = {
	"root_dir",		"www",
	"err_dir",		"http_error",
	"port",			"8080",
	"Connection:",	"close",
	"time_conn",	"5",
	"time_cgi",		"5",
	"Server:",		"edu_http_server",
	"protocol",		"HTTP/1.1",
	"count_rqst",	"5"
};

struct keyval defmime[] = {
	"ico",	"image/x-icon",
	"jpg",	"image/jpeg",
	"jpeg",	"image/jpeg",
	"html",	"text/html",
	"txt",	"text/plain",
	"pdf",	"application/pdf"
};

struct keyval defstate[] = {
	"200", "OK",
	"403", "Forbidden",
	"404", "Not Found",
	"414", "Request-URI Too Large",
	"500", "Internal Server Error",
	"501", "Not Implemented"
};

enum {
	MAXLINE = 8192
};

int read_conf(int fd, struct set_keyval *ptr);

int
set_default_conf(int fflag)
{
	int cfg;
	int i;
	
	// Заполняем динамические массивы conf, state, mime 
	for (i = 0; i < array_size(defconf); ++i) { 
		if (add_record(&conf, defconf[i].key, defconf[i].value) < 0) {
			perror("It wasn't succeeded to add new entry in conf\n");
			return -1;
		}
	}	
	
	for (i = 0; i < array_size(defmime); ++i) { 
		if (add_record(&mime, defmime[i].key, defmime[i].value) < 0) {
			perror("It wasn't succeeded to add new entry in mime\n");
			return -1;
		}
	}
	
	for (i = 0; i < array_size(defstate); ++i) { 
		if (add_record(&state, defstate[i].key, defstate[i].value) < 0) {
			perror("It wasn't succeeded to add new entry in state\n");
			return -1;
		}
	}
	
	// Если новый конфиг не задан, пробуем прочитать конфиг по умолчанию из текущего каталога
	if (fflag == 0) {
		if ( (cfg = open(NAME_CONFIG, O_RDONLY)) < 0 ) {
			perror("It wasn't succeeded to open a default config file");
			fprintf(stderr, "Default static settings will be used\n");
		}
		else if (read_conf(cfg, &conf) < 0) {
			perror("Error read default config");
			close(cfg);
			return -1;
		}
		else {
			close(cfg);	
			fprintf(stdout, "%s: will use default configuration file %s\n", progname, NAME_CONFIG);
		}
	}	
	
	return 0;
}

int
set_new_conf(const char *confpath)
{
	int cfg;
	
	if (confpath != NULL) {
		if ( (cfg = open(confpath, O_RDONLY)) < 0) {
			perror("It wasn't succeeded to open a new config file");
			fprintf(stderr, "Static default settings will be used\n");
		}
		else if (read_conf(cfg, &conf) < 0) { 
			perror("Error read new config");
			close(cfg);
			return -1;
		}
		else {
			close(cfg);	
			fprintf(stdout, "The new configuration file is successfully read\n");
		}
	}		
	
	return 0;
}

int
read_conf(int fd, struct set_keyval *ptr)
{
    struct keyval temp;
	char buf[MAXLINE], *line_conf = NULL;
	int i, len;
	
	clearstrbuf();
	while ( (len = readstr(fd, buf, MAXLINE)) != 0) {
		line_conf = buf;
		while ( (*line_conf == ' ') || (*line_conf == '\t'))
			++line_conf;
		if ( (*line_conf == '#') || (*line_conf == '\n'))
			continue;
		line_conf[strlen(line_conf) - 1] = '\0';
	
		temp.key = strsep(&line_conf, " \t");
        if (line_conf == NULL)
            temp.value = NULL;
        else { 
            while ( (*line_conf == ' ') || (*line_conf == '\t'))
                ++line_conf;
            if (*line_conf == '\0')
                temp.value = NULL;
            else
                temp.value = strsep(&line_conf, " \t");   
        }
		
		// Сравниваем прочитанный ключ из потока file с ключами из ptr
		for (i = 0; i < ptr->ind_cur; ++i) {
			if (strcmp(ptr->element[i].key, temp.key) == 0) {
				free(ptr->element[i].value);
				if ( (ptr->element[i].value = strdup(temp.value)) == NULL)
					return -1;
				break;
			}	
		}
	
		// Если просмотрели массив conf и совпадения по ключам не было, добавляем новые key и value  
		if (i == ptr->ind_cur) {  
			if (add_record(ptr, temp.key, temp.value) < 0) 
				return -1;
		}
	}
	return 0;
}
