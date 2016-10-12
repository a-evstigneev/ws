#ifndef SERVCGI_H
#define SERVCGI_H

#include "servweb.h" 

int service_cgi(int connfd, struct http_request *req);

#endif
