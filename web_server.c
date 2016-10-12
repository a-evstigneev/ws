#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "servfunc.h"
#include "initconf.h"
#include "keyval.h"
#include "servconn.h"

enum {
	LISTENQ = 20,
	MAXSIZE = 8192
};

const char * client_address;
char *progname;
char *confpath;
extern struct set_keyval conf;

int
main(int argc, char *argv[])
{
	int fflag, hflag, errflag, gopt;
	int sockfd, connfd;
	socklen_t len;
	pid_t pid;
	struct sockaddr_in servaddr, cliaddr;
	char buf[MAXSIZE];
	char help[] = {
		"usage: webserv [OPTION]\n"
		"OPTION:\n"
		"-h, --help            - display this help screen\n"
		"-f, --file <filename> - full path to the configuration file\n"
	};
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"file", required_argument, NULL, 'f'},
		{NULL, 0, NULL, 0}
	};
	
	progname = argv[0];

	// Разбор аргументов командной строки
	fflag = hflag = errflag = 0;
	while ( (gopt = getopt_long(argc, argv, "+hf:", longopts, NULL)) != -1) {
		switch (gopt) {
		case 'h':
			hflag = 1;
			if (fflag == 1)
				errflag = 1;
			break;
		case 'f':
			fflag = 1;
			confpath = optarg;
			break;
		case '?':
			errflag = 1;
			break;
		default:
			errflag = 1;
			break;
		}
	}
	if ( (errflag == 1) || (argc - optind > 0)) {
		fprintf(stderr, "usage: %s --help\n", progname);
		exit(EXIT_FAILURE);
	}
	else if (hflag == 1) {
		fprintf(stdout, "%s", help);
		exit(EXIT_SUCCESS);
	}	
	
	// Инициализация настроек сервера
	if (set_default_conf(fflag) < 0)
		err_comm_quit("Default configuration hasn't been set");	
	
	if (fflag == 1)	
		if (set_new_conf(confpath) < 0)
			err_comm_quit("New configuration hasn't been reed");
	
	// Обработка сетевых подключений
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys_quit("Socket create error");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(get_value(conf, TAG_PORT)));
	
	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		err_sys_quit("Bind error");
	
	if (listen(sockfd, LISTENQ) < 0)
		err_sys_quit("Listen socket error");
	else 
		printf("Server listens to port %d\n", atoi(get_value(conf, TAG_PORT)));	
	
	signal(SIGCHLD, sig_chld);

	for (;;) {
		len = sizeof(cliaddr);
		
		if ( (connfd = accept(sockfd, (struct sockaddr *) &cliaddr, &len)) < 0)
			err_sys_quit("Connection accept error");
		
		client_address = inet_ntop(AF_INET, &cliaddr.sin_addr, buf, sizeof(buf));
		printf("\nConnection from %s, port %d\n", client_address, ntohs(cliaddr.sin_port));
		
		if ( (pid = fork()) == 0) {
			close(sockfd);
			service_connect(connfd); 
			fprintf(stdout, "TCP-connection is closed\n");
			exit(EXIT_SUCCESS);
		}
		
		if (close(connfd) < 0)
			err_sys_quit("Error closing connection socket in parent");	
	}
	return 0;
}

