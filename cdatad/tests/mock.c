#define _GNU_SOURCE
#include <string.h>

#include <dlfcn.h>
#include <unistd.h>

#include "mock.h"

struct mocks_t mocks;

void
mock_set(struct mocks_t *new_mocks)
{
    mocks = *new_mocks;
}

void
mock_get(struct mocks_t *m)
{
    memcpy(m, &mocks, sizeof mocks);
}

int
accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    return mocks.accept(sockfd, addr, addrlen);
}

int
access(const char *pathname, int mode)
{
    return mocks.access(pathname, mode);
}

int
bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return mocks.bind(sockfd, addr, addrlen);
}

int
close(int fd)
{
    return mocks.close(fd);
}

int
closedir(DIR *dirp)
{
    return mocks.closedir(dirp);
}

int
connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return mocks.connect(sockfd, addr, addrlen);
}

int
epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    return mocks.epoll_ctl(epfd, op, fd, event);
}

int
epoll_create(int size)
{
    return mocks.epoll_create(size);
}

int
epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    return mocks.epoll_wait(epfd, events, maxevents, timeout);
}

int
fcntl(int fd, int cmd, ...)
{
    return mocks.fcntl(fd, cmd);
}

int
ftruncate(int fd, off_t length)
{
    return mocks.ftruncate(fd, length);
}

int
linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    return mocks.linkat(olddirfd, oldpath, newdirfd, newpath, flags);
}

int
listen(int sockfd, int backlog)
{
    return mocks.listen(sockfd, backlog);
}

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    return mocks.mmap(addr, length, prot, flags, fd, offset);
}

int
munmap(void *addr, size_t length)
{
    return mocks.munmap(addr, length);
}

int
open(const char *pathname, int flags)
{
    return mocks.open(pathname, flags);
}

DIR *
opendir(const char *name)
{
    return mocks.opendir(name);
}

ssize_t
read(int fd, void *buf, size_t count)
{
    return mocks.read(fd, buf, count);
}

struct dirent *
readdir(DIR *dirp)
{
    return mocks.readdir(dirp);
}

int
socket(int domain, int type, int protocol)
{
    return mocks.socket(domain, type, protocol);
}

int
shutdown(int sockfd, int how)
{
    return mocks.shutdown(sockfd, how);
}

ssize_t
write(int fd, const void *buf, size_t count)
{
    return mocks.write(fd, buf, count);
}

const char *
get_data_directory(void)
{
    return mocks.get_data_directory();
}

int
get_epoll_fd(void)
{
    return mocks.get_epoll_fd();
}

const char *
default_get_data_directory(void)
{
    #define CURRENT_WORKING_DIRECTORY_SIZE 256
    static char current_working_directory[CURRENT_WORKING_DIRECTORY_SIZE];
    static int initialized;

    if (!initialized)
    {
        size_t remaining;

        getcwd(current_working_directory, sizeof current_working_directory);
        remaining = strlen(current_working_directory) - CURRENT_WORKING_DIRECTORY_SIZE - 1;
        strncat(current_working_directory, "/data", remaining);
        current_working_directory[CURRENT_WORKING_DIRECTORY_SIZE - 1] = 0;
    }

    return current_working_directory;
}

int
default_get_epoll_fd(void)
{
    return 0;
}

void
mock_create_default(struct mocks_t *mocks)
{
    static struct mocks_t default_mocks;
    static int initialized;

    if (!initialized)
    {
        initialized = 1;

        default_mocks.accept = dlsym(RTLD_NEXT, "accept");
        default_mocks.access = dlsym(RTLD_NEXT, "access");
        default_mocks.bind = dlsym(RTLD_NEXT, "bind");
        default_mocks.close = dlsym(RTLD_NEXT, "close");
        default_mocks.closedir = dlsym(RTLD_NEXT, "closedir");
        default_mocks.connect = dlsym(RTLD_NEXT, "connect");
        default_mocks.epoll_ctl = dlsym(RTLD_NEXT, "epoll_ctl");
        default_mocks.epoll_create = dlsym(RTLD_NEXT, "epoll_create");
        default_mocks.epoll_wait = dlsym(RTLD_NEXT, "epoll_wait");
        default_mocks.fcntl = dlsym(RTLD_NEXT, "fcntl");
        default_mocks.ftruncate = dlsym(RTLD_NEXT, "ftruncate");
        default_mocks.linkat = dlsym(RTLD_NEXT, "linkat");
        default_mocks.listen = dlsym(RTLD_NEXT, "listen");
        default_mocks.mmap = dlsym(RTLD_NEXT, "mmap");
        default_mocks.munmap = dlsym(RTLD_NEXT, "munmap");
        default_mocks.open = dlsym(RTLD_NEXT, "open");
        default_mocks.opendir = dlsym(RTLD_NEXT, "opendir");
        default_mocks.read = dlsym(RTLD_NEXT, "read");
        default_mocks.readdir = dlsym(RTLD_NEXT, "readdir");
        default_mocks.socket = dlsym(RTLD_NEXT, "socket");
        default_mocks.shutdown = dlsym(RTLD_NEXT, "shutdown");
        default_mocks.write = dlsym(RTLD_NEXT, "write");
        default_mocks.get_data_directory = default_get_data_directory;
        default_mocks.get_epoll_fd = default_get_epoll_fd;
    }
    memcpy(mocks, &default_mocks, sizeof(struct mocks_t));
    return;
}

/*
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    int access(const char *pathname, int mode);
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int close(int fd);
    int closedir(DIR *dirp);
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    int epoll_create(int size);
    int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    int fcntl(int fd, int cmd, ...);
    int ftruncate(int fd, off_t length);
    int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
    int listen(int sockfd, int backlog);
    void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    int munmap(void *addr, size_t length);
    int open(const char *pathname, int flags);
    DIR *opendir(const char *name);
    ssize_t read(int fd, void *buf, size_t count);
    struct dirent *readdir(DIR *dirp);
    int socket(int domain, int type, int protocol);
    int shutdown(int sockfd, int how);
    ssize_t write(int fd, const void *buf, size_t count);
*/

