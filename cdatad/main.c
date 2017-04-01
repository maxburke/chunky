#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "chunky.h"
#include "chunky_internal.h"

#define LISTEN_PORT                 14240
#define MAX_EVENTS                  16

static char data_directory[256];
static struct chunk_list_t chunks;
static int epoll_fd;

const char *
get_data_directory(void)
{
    return data_directory;
}

uint32_t
get_chunk_num(void)
{
    return chunks.num;
}

const uint64_t *
get_chunks(void)
{
    return chunks.ptr;
}

int
get_epoll_fd(void)
{
    return epoll_fd;
}

void
set_epoll_fd(int fd)
{
    epoll_fd = fd;
}

static int
message_delete_chunk_handler(struct connection_t *connection)
{
    UNUSED(connection);
    return 0;
}

static int
message_shutdown_handler(struct connection_t *connection)
{
    UNUSED(connection);
    return 0;
}

message_handler_t message_handlers[] = {
    message_initial_handler,
    message_ping,
    message_get_chunk_list_handler,
    message_get_chunk_data_handler,
    message_put_chunk_data_handler,
    message_delete_chunk_handler,
    message_shutdown_handler,
    message_mirror_chunk_data_handler
};

static struct connection_t *active_connections;
static int num_active_connections;
static int max_active_connections;

static void
handle_connection(int index)
{
    struct connection_t *connection = &active_connections[index];
    int message = connection->message;

    if (message_handlers[message](connection) == 0)
    {
        if (num_active_connections > 1)
        {
            active_connections[index] = active_connections[num_active_connections - 1];
        }

        --num_active_connections;

        if (connection->fd != 0)
        {
            shutdown(connection->fd, SHUT_WR);
            close(connection->fd);
        }
    }

    return;
}

static void
find_and_handle_connection(int fd)
{
    int i;

    for (i = 0; i < num_active_connections; ++i)
    {
        if (active_connections[i].fd == fd)
        {
            handle_connection(i);
        }
    }

    if (num_active_connections == max_active_connections)
    {
        const int realloc_delta = 16;
        active_connections = realloc(active_connections, 
                (max_active_connections + realloc_delta) * sizeof(struct connection_t));
    }

    int slot = num_active_connections++;
    memset(&active_connections[slot], 0, sizeof(struct connection_t));
    active_connections[slot].fd = fd;

    handle_connection(slot);
}

static int
create_socket_listener(void)
{
    int bind_socket;
    struct sockaddr_in address;
    
    memset(&address, 0, sizeof address);
    
    bind_socket = socket(AF_INET, SOCK_STREAM, 0);
    VERIFY(bind_socket != -1);

    address.sin_family = AF_INET;
    address.sin_port = htons(LISTEN_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    VERIFY(bind(bind_socket, (struct sockaddr *)&address, sizeof address) != -1);
    VERIFY(listen(bind_socket, 128) != -1);

    return bind_socket;
}

static int
accept_add_to_epoll_list(int epoll_set, int listen_sock)
{
    int connection;
    struct epoll_event event = { 0, { 0 } };
    struct sockaddr_in address;
    socklen_t address_size = sizeof connection;

    connection = accept(listen_sock, (struct sockaddr *)&address, &address_size);
    VERIFY(connection != -1);

    fcntl(connection, F_SETFD, O_NONBLOCK);
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = connection;

    VERIFY(epoll_ctl(epoll_set, EPOLL_CTL_ADD, connection, &event) != -1);

    return 0;
}

static int
do_listen_loop(void)
{
    int socket_listener;
    const int MANDATORY_NON_ZERO_PARAMETER = 1;
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event ev;
    int epoll_fd;

    socket_listener = create_socket_listener();
    if (socket_listener == -1)
    {
        return -1;
    }

    epoll_fd = epoll_create(MANDATORY_NON_ZERO_PARAMETER);
    VERIFY(epoll_fd != -1);
    set_epoll_fd(epoll_fd);

    ev.events = EPOLLIN;
    ev.data.fd = socket_listener;
    VERIFY(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_listener, &ev) != -1);

    for (;;)
    {
        int num_fds;
        int i;

        num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        VERIFY(num_fds != -1);

        for (i = 0; i < num_fds; ++i)
        {
            int current_fd = events[i].data.fd;

            if (current_fd == socket_listener)
            {
                accept_add_to_epoll_list(epoll_fd, socket_listener);
            }
            else
            {
                find_and_handle_connection(current_fd);
            }
        }
    }

    return 0;
}

    
int
main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: chunky [datadir]\n");
        return -1;
    }

    strncpy(data_directory, argv[2], (sizeof data_directory) - 1);

    if (chunk_list_build(&chunks) != 0)
    {
        return -1;
    }

    if (do_listen_loop() != 0)
    {
        /*
         * Notify master that this chunk node is going down.
         */
        return -1;
    }

    return 0;
}

