#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include "servweb.h"
#include "keyval.h"
#include "readstr.h"

enum {
	MAXLINE				= 8192,
	MAX_REQUEST_LINE	= 8000,
	LEN_FILE_SIZE 		= 16
};

// request_parse() - функция разбирает стартовую строку на составляющие    
int
request_parse(int connfd, struct http_request *req)
{
	char buf_soc[MAXLINE] = {0}, *buf, *tempres, *clean_tempres;
	char *index = "/index.html";
	struct keyval temp;
	size_t len;
	int i;

	clearstrbuf();
	// Разбор стартовой строки  	
	if ( (len = readstr(connfd, buf_soc, MAXLINE)) != 0) {
			
		buf = buf_soc;
		if (len > MAX_REQUEST_LINE) {
			req->reqline.status_code = "414";
			return 0;
		}
		while ( (buf[len - 1] == '\n') || (buf[len - 1] == '\r') || (buf[len - 1] == ' ')) {
			buf[len - 1] = '\0';
			--len;
		}
		if ( (req->reqline.method = strdup(strsep(&buf, " "))) == NULL) {
			req->reqline.status_code = "500";
			return -1;
		}
		if (buf == NULL) {
			req->reqline.status_code = "400";
			return 0;
		}
		if ( (req->reqline.resourse = strdup(strsep(&buf, " "))) == NULL) {
			req->reqline.status_code = "500";
			return -1;
		}	
		if (buf == NULL) {
			req->reqline.status_code = "400";
			return 0;
		}	
		if ( (req->reqline.protocol = strdup(strsep(&buf, " "))) == NULL) {
			req->reqline.status_code = "500";
			return -1;
		}
		if (strcmp(req->reqline.resourse, "/") == 0)
			req->reqline.resourse = strdup(index);

		// Разбор строки ресурса request.reqline.resourse на имя файла, расширение и строку запроса (если таковые имеются) 
		if ( (tempres = strdup(req->reqline.resourse)) == NULL) {
			req->reqline.status_code = "500";
			return -1;
		}
		clean_tempres = tempres;
		if ( (req->reqline.file_path = strdup(strsep(&tempres, "?"))) == NULL) {
			req->reqline.status_code = "500";
			return -1;
		}
		if (tempres != NULL) {
			if ( (req->reqline.query_str = strdup(strsep(&tempres, " "))) == NULL) {
				req->reqline.status_code = "500";
				return -1;
			}
		}
		
		if (strrchr(req->reqline.file_path, '/') != NULL) {
			if ( (req->reqline.file_name = strdup(strrchr(req->reqline.file_path, '/') + 1)) == NULL) {
				req->reqline.status_code = "500";
				return -1;
			}
		}
		
		if (strrchr(req->reqline.file_name, '.') != NULL) {
			if ( (req->reqline.exten = strdup(strrchr(req->reqline.file_name, '.') + 1)) == NULL) {
				req->reqline.status_code = "500";
				return -1;
			}
			else if (*(req->reqline.exten) == '\0')
				req->reqline.exten = NULL;
		}
		
		free(clean_tempres);
	}

	if (read_headers(connfd, &(req->headers)) < 0) {
		req->reqline.status_code = "500";
		return -1;
	}
	return 0;
}

// read_header() - функция считывает заголовки http-запроса и присваивает их элементам структуры set_keyval
int
read_headers(int fd, struct set_keyval *ptr)
{
	char bufpipe[MAXLINE] = {0};
	char *buf;
	int len;
	struct keyval temp;
	
	while ( (len = readstr(fd, bufpipe, MAXLINE)) != 0) {
		buf = bufpipe;
		// Если прочитали строку отделяющую заголовки от тела запроса, то выходим из цикла
		if (strcmp(buf, "\r\n") == 0)
			break;
	
		while ( (buf[len - 1] == '\n') || (buf[len - 1] == '\r') || (buf[len - 1] == ' ')) {
			buf[len - 1] = '\0';
			--len;
		}
		
		temp.key = strsep(&buf, " ");
		temp.value = buf;

		if (add_record(ptr, temp.key, temp.value) < 0)
			return -1;
	}
	return 0;
}

// clear_request() - функция освобождает динамическую память выделенную для полей структуры http_request
int
clear_request(struct http_request *req)
{
	int i;
	
	free(req->reqline.method);
	req->reqline.method = NULL;
	
	free(req->reqline.resourse);
	req->reqline.resourse = NULL;
	
	free(req->reqline.protocol);
	req->reqline.protocol = NULL;
	
	free(req->reqline.file_path);
	req->reqline.file_path = NULL;
	
	free(req->reqline.file_name);
	req->reqline.file_name = NULL;
	
	if (req->reqline.exten != NULL) {
		free(req->reqline.exten);
		req->reqline.exten = NULL;
	}

	if (req->reqline.query_str != NULL) {
		free(req->reqline.query_str);
		req->reqline.query_str = NULL;
	}
	
	req->reqline.status_code = NULL;

	for (i =0; i < req->headers.ind_cur; ++i) {
		free(req->headers.element[i].key);
		req->headers.element[i].key = NULL;

		free(req->headers.element[i].value);
		req->headers.element[i].value = NULL;
	}
	
	free(req->headers.element);
	req->headers.element = NULL;

	req->headers.ind_cur = 0;
	req->headers.ind_max = 0;

	return 0;
}

// check_method () - функция проверяет соответствие метода в запросе, с доступным на сервере
int
check_method(char *ptr)
{
	int i;
	char *method[] = {"GET", "HEAD", "POST", NULL};
	
	i = 0;
	while(method[i] != NULL) {
		if (strcmp(method[i], ptr) != 0) {
			++i;
		}
		else 
			return 0;
	}
	return -1;
}

// filesise() - функция возвращает размер resourse:w
char * 
filesize(int resourse, char *ptr, int len)
{
	struct stat fname;
	
	fstat(resourse, &fname);
	snprintf(ptr, len, "%Ld", fname.st_size);
	return ptr;
}

// pass_file() - функция записывает в дескриптор connfd, файл resourse, используя системные вызов sendfile 
int 
pass_file(int connfd, int resourse)
{
	char fsize[LEN_FILE_SIZE];
	int resourse_size;

	resourse_size = atoi(filesize(resourse, fsize, LEN_FILE_SIZE));
	sendfile(connfd, resourse, NULL, resourse_size);
	
	return 0;
}

// get_date() - функция возвращает строку даты для часового пояса GMT(UTC+0)
char *
get_date(char *ptr, int len) 
{
	time_t timeval;
	struct tm *timeptr;
	
	timeval = time(NULL);
	timeptr = gmtime(&timeval);
	strftime(ptr, len, "%a, %d %b %Y %T GMT", timeptr);
	return ptr;
}

// send_respline() - функция записывает стартовую строку ответа в дескриптор fd
int 
send_respline(int fd, const char *protocol, const char *code, const char *def)
{
/*	char temp_respline[1024] = {0};
	int len;

	len = strlen(protocol) + strlen(code) + strlen(def) + 5;
	snprintf(temp_respline, len, "%s %s %s\r\n", protocol, code, def);
	write(fd, temp_respline, len - 1);*/
	dprintf(fd, "%s %s %s\r\n", protocol, code, def);
	return 0;
}

// send_header() - функция записывает http-заголовок в дескриптор fd
int
send_header(int fd, const char *key, const char *val)
{
/*	char temp_header[1024] = {0};
	int len;
	
	len = strlen(key) + strlen(val) + 4;
	snprintf(temp_header, len, "%s %s\r\n", key, val);
	write(fd, temp_header, len - 1);*/
	dprintf(fd, "%s %s\r\n", key, val);
	return 0;
}

// send_chunk() - функция записывает размер чанка len в дескриптор fd 
int
send_chunk(int fd, int len)
{
	dprintf(fd, "%X\r\n", len);
	return 0;
}
