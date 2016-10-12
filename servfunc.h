#ifndef SERVFUNC_H
#define SERVFUNC_H

void err_sys_quit(const char *str);
void err_comm_quit(const char *str);

void sig_chld(int signo);

char *merge_str(const char *name, const char *val);

#endif
