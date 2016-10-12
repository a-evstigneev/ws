#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "readstr.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

enum {
	MAXLINE = 8192	
};

int
main(int argc, char *argv[])
{
	extern char **environ;
	char *query_str = NULL;
	char *method = NULL;
	char *contlen = NULL;
	char *conttype = NULL;
	char *host = NULL;
	char *user_agent = NULL;
	char *path;
	int i, len = 0, recv = 0, tran = 0;
	char buf[MAXLINE] = {0};
	int fd;

	if ( (method = getenv("REQUEST_METHOD")) == NULL)
		fprintf(stderr, "Клиент: метод не определён\n");
	if ( (query_str = getenv("QUERY_STRING")) == NULL)
		fprintf(stderr, "Клиент: переменная QUERY_STRING не определена\n");
	if ( (contlen = getenv("CONTENT_LENGTH")) == NULL)
		fprintf(stderr, "Клиент: переменная CONTENT_LENGTH не определена\n");
	if ( (conttype = getenv("CONTENT_TYPE")) == NULL)
		fprintf(stderr, "Клиент: переменная CONTENT_TYPE не определена\n");
	if ( (host = getenv("HTTP_HOST")) == NULL)
		fprintf(stderr, "Клиент: переменная HTTP_HOST не определена\n");
	if ( (user_agent = getenv("HTTP_USER_AGENT")) == NULL)
		fprintf(stderr, "Клиент: переменная HTTP_USER_AGENT не определена\n");
	
	
	printf("Content-type: text/html\r\n");
	printf("\r\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<title>My first page!</title>\n");
	printf("</head>\n");
	printf("<body>\n");
	
	printf("<p>REQUEST_METHOD = %s</p>\n", method);
	printf("<p>QUERY_STRING = %s</p>\n", query_str);
	printf("<p>CONTENT_LENGTH = %s</p>\n", contlen);
	printf("<p>CONTENT_TYPE = %s</p>\n", conttype);
	printf("<p>HTTP_HOST = %s</p>\n", host);
	printf("<p>HTTP_USER_AGENT = %s</p>\n", user_agent);
	
	fflush(stdout);	
	if ( (fd = open("www/abbott_king.txt", O_RDONLY)) > 0) {
		while ( (recv = read(fd, buf, sizeof(buf))) > 0)
			write(1, buf, recv);
	}
	else
		fprintf(stderr, "Не удалось открыть файл abbot_king.txt\n");

	printf("</body>\n");
	printf("</html>\n");
	return 0;
}
