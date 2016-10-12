#ifndef INITCONF_H
#define INITCONF_H

#define NAME_CONFIG 	"web_serv.conf"
#define PROTOCOL		"protocol"
#define DEF_MIME		"application/octet-stream"
#define TAG_ROOT_DIR	"root_dir"
#define TAG_ERROR_DIR	"err_dir"
#define TAG_PORT		"port"
#define TAG_CONNECTION	"Connection:"
#define TAG_TIME_CONN	"time_conn"
#define TAG_TIME_CGI	"time_cgi"
#define TAG_COUNT_RQST	"count_rqst"
#define TAG_SERVNAME	"Server:"
#define array_size(a) (sizeof(a)/sizeof((a)[0]))

int set_default_conf(int fflag);

int set_new_conf(const char *confpath);

#endif

