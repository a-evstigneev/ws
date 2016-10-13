#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

enum {
	MAXLINE = 8192	
};

int
main()
{
	extern char **environ;
	char *query_str = NULL;
	char *method = NULL;
	int contlen;
	char *conttype = NULL;
	char *host = NULL;
	char *user_agent = NULL;
	char *pathinfo = NULL;
	char *remaddr = NULL;
	char *ptrreq = NULL;
	int i, len = 0, recv = 0, tran = 0;
	char buf[MAXLINE] = {0};
	char *resp_stat = NULL;
	int err = 0;
	int fd;

	
	method = getenv("REQUEST_METHOD");
	fprintf(stderr, "%s\n", method);
	if (strcmp(method, "GET") == 0) 
		query_str = getenv("QUERY_STRING");
	else if (strcmp(method, "POST") == 0) {
		conttype = getenv("CONTENT_TYPE");
		if (strcmp(conttype, "application/x-www-form-urlencoded") != 0)
			resp_stat = "400 Bad Request";
		contlen = atoi(getenv("CONTENT_LENGTH"));
		ptrreq = realloc(NULL, contlen);
		if (ptrreq != NULL)
			recv = read(0, ptrreq, contlen);
		else
			resp_stat = "500 Internal Server Error";
	}
	else
		resp_stat = "501 Not Implemented";
	
	fprintf(stderr, "%s %s %d\n", resp_stat, conttype, contlen);
	
	if (resp_stat != NULL) {
		printf("Content-type: text/html;charset=utf-8\r\n");
		printf("Status: %s\r\n", resp_stat);
		printf("\r\n");
		printf("<html>\n");
		printf("<head>\n");
		printf("<title>Error!</title>\n");
		printf("</head>\n");
		printf("<body>\n");
		printf("<H1>Server Error %s</H1>", resp_stat);
		printf("</body>\n");
		printf("</html>\n");
		fflush(stdout);	
	}
	else { 
		host = getenv("HTTP_HOST");
		user_agent = getenv("HTTP_USER_AGENT");
		pathinfo = getenv("PATH_INFO");
		remaddr = getenv("REMOTE_ADDR");
		
	fprintf(stderr, "%s\n", host);
	fprintf(stderr, "%s\n", user_agent);
	fprintf(stderr, "%s\n", pathinfo);
	fprintf(stderr, "%s\n", remaddr);
	fprintf(stderr, "%s\n", ptrreq);

		printf("Content-Type: text/html;charset=utf-8\r\n");
		printf("\r\n");
		printf("<html>\n");
		printf("<head>\n");
		printf("<title>CGI page!</title>\n");
		printf("</head>\n");
		printf("<body>\n");
		printf("<p>REQUEST_METHOD = %s</p>\n", method);
		printf("<p>QUERY_STRING = %s</p>\n", query_str);
		printf("<p>CONTENT_LENGTH = %s</p>\n", contlen);
		printf("<p>CONTENT_TYPE = %s</p>\n", conttype);
		printf("<p>HTTP_HOST = %s</p>\n", host);
		printf("<p>HTTP_USER_AGENT = %s</p>\n", user_agent);
		printf("<p>PATH_INFO = %s</p>\n", pathinfo);
		printf("<p>REMOTE_ADDR = %s</p>\n", remaddr);
		printf("<br>");
	//	fflush(stdout);	
//		write(1, ptrreq, contlen);
		printf("</body>\n");
		printf("</html>\n");
		fflush(stdout);	
	
	}
	return 0;
}
