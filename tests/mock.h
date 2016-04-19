#pragma once

#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>

struct epoll_event;
struct sockaddr;

struct mocks_t
{
    int (*accept)(int, struct sockaddr *, socklen_t *);
    int (*access)(const char *, int);
    int (*bind)(int, const struct sockaddr *, socklen_t);
    int (*close)(int);
    int (*closedir)(DIR *);
    int (*connect)(int, const struct sockaddr *, socklen_t);
    int (*epoll_ctl)(int, int, int, struct epoll_event *);
    int (*epoll_create)(int);
    int (*epoll_wait)(int, struct epoll_event *, int, int);
    int (*fcntl)(int, int, ...);
    int (*ftruncate)(int, off_t);
    int (*linkat)(int, const char *, int, const char *, int);
    int (*listen)(int, int);
    void *(*mmap)(void *, size_t, int, int, int, off_t);
    int (*munmap)(void *, size_t);
    int (*open)(const char *, int);
    DIR *(*opendir)(const char *);
    ssize_t (*read)(int, void *, size_t);
    struct dirent *(*readdir)(DIR *);
    int (*socket)(int, int, int);
    int (*shutdown)(int, int);
    ssize_t (*write)(int, const void *, size_t);
};

void
mock_create_default(struct mocks_t *);

void
mock_set(struct mocks_t *);
