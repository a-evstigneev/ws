#ifndef SERVWEB_H
#define SERVWEB_H

#include "keyval.h"

struct request_line {
	char *method;
	char *resourse;
	char *protocol;
	char *file_name;
	char *file_path;
	char *exten;
	char *query_str;
	char *status_code;
};

struct http_request {
	struct request_line reqline;
	struct set_keyval headers;
}; 

int request_parse(int connfd, struct http_request *req);

int read_headers(int fd, struct set_keyval *ptr);

int clear_request(struct http_request *req);

int check_method(char *ptr);

char *filesize(int resourse, char *ptr, int len);

int pass_file(int connfd, int resourse);

char *get_date(char *ptr, int len);

int send_respline(int fd, const char *protocol, const char *code, const char *def);

int send_header(int fd, const char *key, const char *val);

int send_chunk(int fd, int len);	

#endif

