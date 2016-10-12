#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include "servcgi.h"
#include "servweb.h"
#include "keyval.h"
#include "servfunc.h"
#include "initconf.h"
#include "readstr.h"

enum { 
	MAXLINE = 8192
};

extern struct set_keyval conf; 
extern char *client_address;
extern char resolve_path[]; 

int 
service_cgi(int connfd, struct http_request *req)
{
	struct pollfd fds;
	time_t start_time, sig_time;
	pid_t cgipid;
	int pfd1[2], pfd2[2], timeout, len, recv, i, rp;
	char buf1[MAXLINE] = {0};
	char buf2[MAXLINE] = {0};
	char *temp_ptr;
	int temp_len = 0, err = 0;
	char *argv[] = {req->reqline.file_name, NULL};
	char *envp[21] = {NULL};
	struct set_keyval cgi_headers;
	
	// Определяем переменные окружения для cgi-скрипта
	envp[0] = merge_str("AUTH_TYPE=", NULL);
	envp[1] = merge_str("CONTENT_LENGTH=", get_value(req->headers, "Content-Length:"));
	envp[2] = merge_str("CONTENT_TYPE=", get_value(req->headers, "Content-Type:"));
	envp[3] = merge_str("GATEWAY_INTERFACE=", "CGI/1.1");
	envp[4] = merge_str("PATH_INFO=", req->reqline.file_path);
	envp[5] = merge_str("PATH_TRANSLATED=", resolve_path);
	envp[6] = merge_str("QUERY_STRING=", req->reqline.query_str);
	envp[7] = merge_str("REMOTE_ADDR=", client_address);
	envp[8] = merge_str("REMOTE_HOST=", NULL);
	envp[9] = merge_str("REMOTE_IDENT=", NULL);
	envp[10] = merge_str("REMOTE_USER=", NULL);
	envp[11] = merge_str("REQUEST_METHOD=", req->reqline.method);
	envp[12] = merge_str("SCRIPT_NAME=", req->reqline.file_name);
	envp[13] = merge_str("SERVER_NAME=", NULL);
	envp[14] = merge_str("SERVER_PORT=", get_value(conf, TAG_PORT));
	envp[15] = merge_str("SERVER_PROTOCOL=", get_value(conf, PROTOCOL));
	envp[16] = merge_str("SERVER_SOFTWARE=", get_value(conf, TAG_SERVNAME));
	envp[17] = merge_str("HTTP_CONNECTION=", get_value(req->headers, "Connection:"));
	envp[18] = merge_str("HTTP_USER_AGENT=", get_value(req->headers, "User-Agent:"));
	envp[19] = merge_str("HTTP_ACCEPT=", get_value(req->headers, "Accept:"));
	envp[20] = merge_str("HTTP_HOST=", get_value(req->headers, "Host:"));
	
	pipe(pfd1);
	pipe(pfd2);

	if ( (cgipid = fork()) < 0) {
		perror("Error fork cgi_script");
		err = 1;
		goto exitcgi;
	}
	else if (cgipid == 0) {
		// Заменяем в потомке стандартный ввод на pfd1[0]
		close(pfd1[1]);
		close(0);
		dup(pfd1[0]);
		close(pfd1[0]);
		
		// Заменяем в потомке стандартный вывод на pfd2[1]
		close(pfd2[0]);
		close(1);
		dup(pfd2[1]);
		close(pfd2[1]);
		if ( (err = execve(resolve_path, argv, envp)) < 0)
			err_sys_quit("Error execve cgi_script");
	}
	else {
		close(pfd1[0]);
		close(pfd2[1]);

		if (strcmp(req->reqline.method, "POST") == 0) { // Надо ли проверять на наличие ошибки EPIPE при записи в трубу?
			len = atoi(get_value(req->headers, "Content-Length:"));
			// Проверяем буфер функции readstr, если имеются данные читаем и записываем в pipe
			if ( (temp_len = readstrbuf(&temp_ptr)) > 0) {
				write(pfd1[1], temp_ptr, temp_len);	
				clearstrbuf(); // Обнуляем буфер функции readstr	
			}
			// Если в сокете остались данные, тоже считываем и записываем в pipe
			if ( (len -= temp_len) > 0) {
				while ( (len - MAXLINE) > 0) {
					recv = readn(connfd, buf1, MAXLINE);
					writen(pfd1[1], buf1, recv);
					len = len - MAXLINE;
				}
				recv = read(connfd, buf1, len);
				write(pfd1[1], buf1, recv);
			}
			close(pfd1[1]);	
		}
		else if (strcmp(req->reqline.method, "GET") == 0) {
			close(pfd1[1]);
		}

		fds.fd = pfd2[0];
		fds.events = POLLIN;
		timeout = atoi(get_value(conf, TAG_TIME_CGI)) * 1000;
		time(&start_time);	
		
		while (1) {	
			rp = poll(&fds, 1, timeout);
			if (rp < 0) {
				if (errno == EINTR) {
					time(&sig_time);
					timeout = (sig_time - start_time) * 1000;
					continue;
				}
				else {
					// Здесь скорее всего надо принудительно завершать потомка?
					err = 1;
					goto exitcgi;
				}
			}
			else if (rp == 0) {
				fprintf(stderr, "CGI-server: child process has not given data\n");
				if (kill(cgipid, SIGTERM) < 0)
					perror("Error kill proc child process sgi");
				else
					fprintf(stderr, "CGI-server: child process was completed forcibly\n");
				goto exitcgi;
			}
			else {
				if (fds.revents & POLLIN) {
					fprintf(stderr, "CGI-server: pipe is ready to reading\n");
					
					// Читаем из трубы заголовки, которые записал cgi-скрипт. 
					// Необходимо также научиться рассматривать случаи когда скрипт шлет заголовки Status и Location (пока только Content-type)
					if (read_headers(pfd2[0], &cgi_headers) < 0) {
						err = 1;
						goto exitcgi;					
					}

					fprintf(stderr, "CGI-server: cgi-headers have been read\n");					
					// Записываем в сокет стартовую строку и заголовки ответа 
					send_respline(connfd, get_value(conf, PROTOCOL), "200", "OK");
					send_header(connfd, TAG_SERVNAME, get_value(conf, TAG_SERVNAME));
					send_header(connfd, TAG_CONNECTION, get_value(conf, TAG_CONNECTION));
					send_header(connfd, "Accept-Ranges:", "bytes");
					send_header(connfd, "Transfer-Encoding:", "chunked");	
					for (i = 0; i < cgi_headers.ind_cur; ++i)
						send_header(connfd, cgi_headers.element[i].key, cgi_headers.element[i].value);
					write(connfd, "\r\n", 2);
					
					// Проверяем буфер функции readstr на наличие оставшихся байт. Если есть записываем в pipe
					if ( (temp_len = readstrbuf(&temp_ptr)) > 0) {
						send_chunk(connfd, temp_len);	
						write(connfd, temp_ptr, temp_len);	
						write(connfd, "\r\n", 2);	
						clearstrbuf(); 	
					}	
				
				/*
					Вариант с ioctl(pfd2[0], FIONREAD, &len) и вызовом splice работает некорректно (ioctl при повторном вызовы иногда записывает в len 0)
					// Проверяем наличие данных непосредственно в буфере pipe
					if (ioctl(pfd2[0], FIONREAD, &len) == 0) 
						fprintf(stderr, "Сервер-cgi: количество байт доступных в трубе до первого чтения = %d\n", len);	
					
					while (len > 0) {
						send_chunk(connfd, len);
						if ( (recv = splice(pfd2[0], NULL, connfd, NULL, len, SPLICE_F_MORE | SPLICE_F_NONBLOCK)) > 0) 
							fprintf(stderr, "CGI-сервер: в сокет через splice записан chunk размером = %d\n", recv);
						write(connfd, "\r\n", 2);	
						if (ioctl(pfd2[0], FIONREAD, &len) == 0) 
							fprintf(stderr, "Сервер-cgi: количество байт доступных в трубе = %d\n", len);
					}
				*/
					// Пока в трубе есть данные записываем их в сокет определенными порциями
					while ( (recv = readn(pfd2[0], buf2, MAXLINE)) > 0) {
						send_chunk(connfd, recv);
						fprintf(stderr, "CGI-server: chunk size of %d writen to the socket\n", writen(connfd, buf2, recv));
						write(connfd, "\r\n", 2);	
					}
					
					// Завершающий кусок нулевой длины 
					send_chunk(connfd, 0);
					write(connfd, "\r\n", 2);
					
					goto exitcgi;
				}
			}
		}		
	}	
	exitcgi:
		for (i = 0; i < 21; ++i) {
			free(envp[i]);
			envp[i] = NULL;
		}
		if (err == 1) {
			req->reqline.status_code = "500"; // При ошибках системных вызовов или функций, будем возвращать клиенту ошибку "500" 
			return -1;
		}
		return 0;
}
