#include "mock.h"

struct mocks_t mocks;

void
mock_set(struct mocks_t *new_mocks)
{
    mocks = *new_mocks;
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

