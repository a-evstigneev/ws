#ifndef READSTR_H
#define READSTR_H

ssize_t readn(int fd, void *vptr, size_t n);

ssize_t writen(int fd, const void *vptr, size_t n);

ssize_t readstr(int fd, void *vptr, size_t maxlen);

ssize_t readstrbuf(char **vptrptr);

void clearstrbuf(void);

#endif
