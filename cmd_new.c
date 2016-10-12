#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 3000

int
main(int argc, char *argv[])
{
	extern char **environ;
	char *query_str = NULL;
	char *method = NULL;
	char *path;
	int i;
	char buf[65546];
	

	printf("Content-type: text/html\r\n\r\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<title>My first page!</title>\n");
	printf("</head>\n");
	printf("<body>\n");
	printf("<center><h1>Article about Unix</h1></center>\n");
	printf("<p>Unix (trademarked as UNIX) is a family of multitasking, multiuser computer operating systems that derive from the original AT&T Unix, developed in the 1970s at the Bell Labs research center by Ken Thompson, Dennis Ritchie, and others.</p>\n");
	printf("<p>Initially intended for use inside the Bell System, AT&T licensed Unix to outside parties from the late 1970s, leading to a variety of both academic and commercial variants of Unix from vendors such as the University of California, Berkeley (BSD), Microsoft (Xenix), IBM (AIX) and Sun Microsystems (Solaris). AT&T finally sold its rights in Unix to Novell in the early 1990s, which then sold its Unix business to the Santa Cruz Operation (SCO) in 1995,[4] but the UNIX trademark passed to the industry standards consortium The Open Group, which allows the use of the mark for certified operating systems compliant with the Single UNIX Specification (SUS). Among these is Apple's OS X,[5] which is the Unix version with the largest installed base as of 2014.</p>\n");
	printf("</body>\n");
	printf("</html>\n");
	
	return 0;
}

