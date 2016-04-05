#define _BSD_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "chunky.h"
#include "mlb_sha1.h"

#define UNUSED(x) (void)(sizeof(x))

#define TOKEN_PASTE_HELPER(x, y)    x ## y
#define TOKEN_PASTE(x, y)           TOKEN_PASTE_HELPER(x, y)
#define COMPILETIME_ASSERT(x)       typedef char TOKEN_PASTE(compileTimeAssert, __LINE__)[x ? 1 : -1]

#define ARRAY_COUNT(x)              (sizeof(x) / sizeof(x[0]))

#define LISTEN_PORT                 14240
#define MAX_EVENTS                  16

static char data_directory[256];
static uint32_t chunk_num;
static uint32_t chunk_capacity;
static uint64_t *chunks;
static int epoll_fd;

uint32_t
get_chunk_num(void)
{
    return chunk_num;
}

const uint64_t *
get_chunks(void)
{
    return chunks;
}

static void
chunk_push(uint64_t chunk_id)
{
    if (chunk_num == chunk_capacity)
    {
        uint32_t chunk_capacity_new = 1.5 * (double)chunk_capacity;
        
        if (chunk_capacity == 0)
        {
            chunk_capacity_new = 1 << 10;
        }
        
        chunks = realloc(chunks, chunk_capacity_new * sizeof(uint64_t));
    }

    chunks[chunk_num++] = chunk_id;
}

static int
chunk_id_predicate(const void *ptr_a, const void *ptr_b)
{
    const uint64_t *a = ptr_a;
    const uint64_t *b = ptr_b;

    return *a - *b;
}

static int
chunk_name_to_id(uint64_t *id, const char *name)
{
    char *endptr;

    assert(id != NULL);
    *id = strtoul(name, &endptr, 16);

    if (endptr == name)
    {
        *id = 0;
        return 1;
    }

    if (*endptr != 0)
    {
        *id = 0;
        return 1;
    }

    return 0;
}

const char *
get_data_directory(void)
{
    return data_directory;
}

void
chunk_id_to_name(char *name, size_t name_size, uint64_t id)
{
    if (snprintf(name, name_size, "%s/%0lx", data_directory, id)
            >= (int)name_size)
    {
        fprintf(stderr, "Path length too long, string '%s' was truncated. Aborting.", name);
        abort();
    }
}

static int
chunk_list_build(void)
{
    DIR *directory;
    struct dirent *entry;
    int i;

    directory = opendir(data_directory);

    VERIFY(directory != NULL);

    errno = 0;
    i = 0;

    while ((entry = readdir(directory)) != NULL)
    {
        uint64_t chunk_id;

        if (entry->d_type == DT_DIR)
        {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            {
                continue;
            }
        }

        if (chunk_name_to_id(&chunk_id, entry->d_name))
        {
            fprintf(stderr, "File '%s' in chunk data directory is of an unexpected name format!", entry->d_name);
            return 1;
        }
       
        chunk_push(chunk_id);
    }

    qsort(chunks, i, sizeof(uint64_t), chunk_id_predicate);

    closedir(directory);

    return 0;
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

int
buffer_fill(struct active_connection_t *connection, const void *data, size_t bytes)
{
    int cursor = connection->cursor;
    if (cursor + bytes >= sizeof connection->buffer)
    {
        return -1;
    }

    uint8_t *p = (uint8_t *)(&connection->buffer[cursor]);
    memcpy(p, data, bytes);
    connection->cursor = cursor + bytes;

    return 0;
}

int
buffer_add_u8(struct active_connection_t *connection, uint8_t val)
{
    return buffer_fill(connection, &val, sizeof val);
}

int
buffer_add_u32(struct active_connection_t *connection, uint32_t val)
{
    return buffer_fill(connection, &val, sizeof val);
}

int
buffer_add_u64(struct active_connection_t *connection, uint64_t val)
{
    return buffer_fill(connection, &val, sizeof val);
}

/*
 * Message handlers must be re-entrant.
 * Return value is zero if processing of the message is complete,
 * non-zero otherwise.
 */
typedef int (*message_handler_t)(struct active_connection_t *);

int
buffer_send(struct active_connection_t *connection)
{
    ssize_t bytes_sent;
    char *buffer = connection->buffer;
    uint32_t cursor = connection->cursor;
    uint32_t pos = connection->pos;
    uint32_t to_send = cursor - pos;
   
    if (to_send > 0)
    {
        bytes_sent = write(connection->fd, buffer + pos, cursor - pos);

        if (bytes_sent != to_send)
        {
            if (bytes_sent > 0)
            {
                connection->pos = pos + bytes_sent;
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }

    connection->pos = 0;
    connection->cursor = 0;

    return 0;
}

static int
message_delete_chunk_handler(struct active_connection_t *connection)
{
    UNUSED(connection);
    return 0;
}

static int
message_shutdown_handler(struct active_connection_t *connection)
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

static struct active_connection_t *active_connections;
static int num_active_connections;
static int max_active_connections;

static void
handle_connection(int index)
{
    struct active_connection_t *connection = &active_connections[index];
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
                (max_active_connections + realloc_delta) * sizeof(struct active_connection_t));
    }

    int slot = num_active_connections++;
    memset(&active_connections[slot], 0, sizeof(struct active_connection_t));
    active_connections[slot].fd = fd;

    handle_connection(slot);
}

static int
do_listen_loop(void)
{
    int socket_listener;
    const int MANDATORY_NON_ZERO_PARAMETER = 1;
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event ev;

    socket_listener = create_socket_listener();
    if (socket_listener == -1)
    {
        return -1;
    }

    epoll_fd = epoll_create(MANDATORY_NON_ZERO_PARAMETER);
    VERIFY(epoll_fd != -1);

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
    if (argc < 3)
    {
        fprintf(stderr, "usage: chunky [datadir]");
        return -1;
    }

    strncpy(data_directory, argv[2], (sizeof data_directory) - 1);

    if (chunk_list_build() != 0)
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

