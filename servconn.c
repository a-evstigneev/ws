#define _GNU_SOURCE
#include <stdio.h>
#include <poll.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "initconf.h"
#include "servfunc.h"
#include "servconn.h"
#include "keyval.h"
#include "servweb.h"
#include "initconf.h"
#include "servcgi.h"
#include "readstr.h"

extern struct set_keyval conf, state, mime;
enum {
	MAXLINE			= 8192,
	REQ_SUCC 		= 0,
	REQ_CGI			= 1,
	REQ_ERR			= 2,
	LEN_DATE		= 64,
	LEN_FILE_SIZE 	= 16
};

char resolve_path[MAXLINE];

int exec_response(int connfd, struct http_request *req);
int	pass_resp_headers(int connfd, int resourse, struct http_request *req);

// service_connect() - функция для обработки http-запроса и записи http-ответа в сокет
int 
service_connect(int connfd) 
{
	int i, rc, timeout, count_rqst = 0;
	struct http_request request = {NULL};
	time_t start_time, sig_time;  
	struct pollfd fds;
	
	fds.fd = connfd;
	fds.events = POLLIN;
	timeout = atoi(get_value(conf, TAG_TIME_CONN)) * 1000;
	time(&start_time);
	
	while (1) {
		rc = poll(&fds, 1, timeout);
		if (rc < 0) { 
			if (errno == EINTR) {
 				time(&sig_time);
 				timeout = (sig_time - start_time) * 1000;
 				continue;
 			}
 			else 	
 				err_sys_quit("Call error poll()");
		}
		else if (rc == 0) { 
			fprintf(stderr, "The socket isn't present data\n");
			goto closecon;
		}
		else {
			if (request_parse(connfd, &request) < 0) 
				perror("request_parse error");
			
			exec_response(connfd, &request);				
			
			clear_request(&request);
			
			if (++count_rqst == atoi(get_value(conf, TAG_COUNT_RQST)))
				goto closecon; 

			// В зависимости от значения ключа Connection, ждем новые запросы (keep-alive) или закрываем соединение (close)
			if (strcmp(get_value(conf, TAG_CONNECTION), "keep-alive") == 0) {
				timeout = atoi(get_value(conf, TAG_TIME_CONN)) * 1000;
				continue;
			}
			else if (strcmp(get_value(conf, TAG_CONNECTION), "close") == 0) 
				goto closecon;
		}
	}
	
	// Единая точка выхода
	closecon:
		fprintf(stdout, "The socket will be close\n");
		if (close(connfd) < 0)
			err_sys_quit("Error closing connection socket in child");
		
	return 0;
}

// exec_response() - функция для отправки ответa на запрос
int
exec_response(int connfd, struct http_request *req)
{
	int res_fd, err_fd;
	int req_state = 0, len;
	char *relative_path, error_path[MAXLINE] = {0};
	
	// Резолвим http-путь к запрошенному файлу в полный путь
	if ( (relative_path = merge_str(get_value(conf, TAG_ROOT_DIR), req->reqline.file_path)) != NULL)
		realpath(relative_path, resolve_path);
	else
		req->reqline.status_code = "500";

	if (req->reqline.status_code != NULL) {
		req_state = REQ_ERR; 
	}
	else if (check_method(req->reqline.method) < 0) {
		req->reqline.status_code = "501";
		req_state = REQ_ERR;
	}
	else if (strncmp(get_value(conf, TAG_ROOT_DIR), resolve_path, strlen(get_value(conf, TAG_ROOT_DIR))) != 0) {
		req->reqline.status_code = "403";
		req_state = REQ_ERR;
	}
	else if ( (req->reqline.exten != NULL) && (strcmp(req->reqline.exten, "cgi") == 0)) { 
		req_state = REQ_CGI;
	}
	else if ( (res_fd = open(resolve_path, O_RDONLY)) == -1) {
		if (errno == EACCES)
			req->reqline.status_code = "403";
		else if (errno == ENOENT)
			req->reqline.status_code = "404";
		req_state = REQ_ERR;
	}
	else { 
		req->reqline.status_code = "200";
		req_state = REQ_SUCC;
	}

	// В зависимости от значения req_state возвращаем запрошенный resource (REQ_SUCC), \
	html-страницу с кодом ошибки (REQ_ERR) или запускаем cgi-скрипт (REQ_CGI) 
	if (req_state == REQ_CGI) {
		if (service_cgi(connfd, req) < 0)
			req_state = REQ_ERR;
	}
	if (req_state == REQ_SUCC) {
		pass_resp_headers(connfd, res_fd, req);
		if (strcmp(req->reqline.method, "GET") == 0) 
			pass_file(connfd, res_fd);
		close(res_fd);
	}
	if (req_state == REQ_ERR) {
		len = strlen(get_value(conf, TAG_ERROR_DIR)) + strlen(req->reqline.status_code) + 7; 
		snprintf(error_path, len, "%s/%s.html", get_value(conf, TAG_ERROR_DIR), req->reqline.status_code); 
		err_fd = open(error_path, O_RDONLY);
		pass_resp_headers(connfd, err_fd, req);
		pass_file(connfd, err_fd);
		close(err_fd);
	}
	
	free(relative_path);
	return 0;
}

int
pass_resp_headers(int connfd, int resourse, struct http_request *req)
{
	char fsize[LEN_FILE_SIZE];
	char date[LEN_DATE];
	
	send_respline(connfd, get_value(conf, PROTOCOL), req->reqline.status_code, get_value(state, req->reqline.status_code));
	send_header(connfd, TAG_SERVNAME, get_value(conf, TAG_SERVNAME));	
	send_header(connfd, TAG_CONNECTION, get_value(conf, TAG_CONNECTION));
	send_header(connfd, "Accept-Ranges:", "bytes");	
	send_header(connfd, "Date:", get_date(date, LEN_DATE));	
	
	if (strcmp(req->reqline.status_code, "200") != 0) 
		send_header(connfd, "Content-Type:", get_value(mime, "html"));	
	else if (req->reqline.exten != NULL)
		send_header(connfd, "Content-Type:", get_value(mime, req->reqline.exten));	
	else
		send_header(connfd, "Content-Type:", DEF_MIME);	
	
	if (strcmp(req->reqline.method, "GET") == 0) 
		send_header(connfd, "Content-Length:", filesize(resourse, fsize, LEN_FILE_SIZE));	
	write(connfd, "\r\n", 2);
	
	return 0;
}
