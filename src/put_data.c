#define _GNU_SOURCE

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chunky.h"

enum put_data_states_t
{
    FLUSHING_BUFFER = 1 << 0,
    READ_REQUEST = 1 << 1,
    MAPPED = 1 << 2,
    WRITTEN = 1 << 3,
    VERIFIED = 1 << 4, 
    MIRRORED = 1 << 5
};

static int
read_put_data_request(struct connection_t *connection)
{
    char *buffer;
    uint32_t pos;
    ssize_t bytes_expected;
    ssize_t bytes_read;

    buffer = connection->buffer;
    pos = connection->pos;
    bytes_expected = sizeof(struct put_data_request_t) - pos;
    bytes_read = read(connection->fd, buffer + pos, bytes_expected);

    if (bytes_read == -1)
    {
        return -1;
    }

    if (bytes_read != bytes_expected)
    {
        connection->pos = pos + bytes_read;
        return 1;
    }

    memmove(&connection->put_data_context.request, buffer, sizeof(struct put_data_request_t));

    connection->state |= READ_REQUEST;
    connection->pos = 0;
    memset(buffer, 0, sizeof(struct put_data_request_t));

    mlb_sha1_hash_init(&connection->hash_context);

    return 0;
}

static int
map_file_for_write(struct connection_t *connection)
{
    struct put_data_context_t *context;
    uint64_t id;
    int fd;
    const char *data_directory;
    uint64_t file_size;
    void *base;
    
    context = &connection->put_data_context;
    id = context->request.id;
    chunk_id_to_name(context->filename, sizeof context->filename, id);

    /*
     * If the chunk already exists, return an error.
     */
    if (access(context->filename, F_OK) == -1)
    {
        /*
         * TODO: Log error. Send error code back to client.
         */
        uint8_t response;

        response = FAIL;
        write(connection->fd, &response, sizeof response);

        return -1;
    }

    data_directory = get_data_directory();
    fd = open(data_directory, O_RDWR | O_TMPFILE, S_IRUSR | S_IWUSR);

    /*
     * Hash is written to the front of the file so that it does not need to be
     * re-calculated when sending to clients or replicating to other chunk
     * servers.
     */
    file_size = context->request.size + sizeof(struct mlb_sha1_hash_t);
    if (ftruncate(fd, context->request.size) == 0)
    {
        /*
         * TODO: Log error. Send error code back to client.
         */
        uint8_t response;

        response = FAIL;
        write(connection->fd, &response, sizeof response);
        close(fd);

        return -1;
    }

    base = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE /* | MAP_HUGETLB */, fd, 0);

    if (base == MAP_FAILED)
    {
        /*
         * TODO: Log error. Send error code back to client.
         */
        uint8_t response;

        response = FAIL;
        write(connection->fd, &response, sizeof response);
        close(fd);

        return -1;
    }

    context->fd = fd;
    context->base = base;
    context->file_size = file_size;

    return 0;
}

static int
initialize_mirror(struct connection_t *connection)
{
    struct put_data_request_t *request;
    int sock;
    struct sockaddr_in addr;
    struct epoll_event event = { 0, { 0 } };

    request = &connection->put_data_context.request;

    if (request->n == 0)
    {
        return 0;
    }

    assert(request->n <= MAX_REPLICATION_FACTOR);

    /*
     * shutdown is used to ensure a FIN packet is sent after the chunk
     * server sends the OK packet.
     */
    shutdown(connection->fd, SHUT_WR);

    /*
     * TODO: This should probably poll in a recv()/read() loop until
     * it returns 0 before closing.
     */
    close(connection->fd);

    /*
     * In case we can't connect to the mirror, this ensures we do
     * not accidentally re-close the fd when the normal socket 
     * clean-up happens.
     */
    connection->fd = 0;
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = request->replicants[0].address;
    addr.sin_port        = request->replicants[0].port;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, &addr, sizeof addr) != 0)
    {
        /*
         * TODO: report error!
         */
        assert(0 && "report error!");
        return -1;
    }

    fcntl(sock, F_SETFD, O_NONBLOCK);

    connection->fd      = sock;
    connection->message = MESSAGE_CHUNK_MIRROR;
    memset(connection->buffer, 0, sizeof connection->buffer);
    connection->cursor  = 0;
    connection->pos     = 0;
    connection->state   = 0;
    memset(&connection->hash_context, 0, sizeof connection->hash_context);

    int epoll_fd = get_epoll_fd();
    VERIFY(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection->fd, &event) != -1);

    /*
     * At this point we've re-appropriated the connection object for a mirror 
     * operation. We can leave the put data handler and let the mirror handler
     * resume.
     */

    return 0;
}

int
message_put_chunk_data_handler(struct connection_t *connection)
{
    if ((connection->state & READ_REQUEST) != 0)
    {
        int rv;

        rv = read_put_data_request(connection);

        if (rv == 1)
        {
            /*
             * Insufficient data read, please try again later.
             */
            return 1;
        }
        else if (rv == -1)
        {
            /*
             * Something went wrong, abort connection.
             * TODO: Log failure
             */

            return 0;
        }
    }
    assert((connection->state & READ_REQUEST) != 0);

    if ((connection->state & MAPPED) == 0)
    {
        struct put_data_context_t *context = &connection->put_data_context;

        if (context->fd == 0 && map_file_for_write(connection) != 0)
        {
            return 0;
        }

        connection->state |= MAPPED;
    }
    assert((connection->state & MAPPED) != 0);

    if ((connection->state & WRITTEN) == 0)
    {
        struct put_data_context_t *context;
        char *ptr;
        size_t bytes_remaining;
        ssize_t bytes_read;

        context = &connection->put_data_context;
        ptr = (char *)context->base + context->i;
        bytes_remaining = context->request.size - context->i;
        bytes_read = read(connection->fd, ptr, bytes_remaining);

        if (bytes_read == -1)
        {
            /*
             * TODO: Log error. Send error code back to client.
             */
            uint8_t response;

            response = FAIL;
            write(connection->fd, &response, sizeof response);

            goto abort_fail;
        }

        mlb_sha1_hash_update(&connection->hash_context, ptr, (size_t)bytes_read);

        context->i += bytes_read;
        if ((size_t)bytes_read != bytes_remaining)
        {
            return 1;
        }

        connection->state |= WRITTEN;
    }
    assert((connection->state & WRITTEN) != 0);

    if ((connection->state & VERIFIED) == 0)
    {
        struct mlb_sha1_hash_t hash;
        char tmp_file[64];

        hash = mlb_sha1_hash_finalize(&connection->hash_context);
        if (memcmp(&hash, connection->put_data_context.base, sizeof(struct mlb_sha1_hash_t)) != 0)
        {
            /*
             * TODO: Log error. Send error code back to client.
             */
            uint8_t response;

            response = FAIL;
            write(connection->fd, &response, sizeof response);

            goto abort_fail;
        }

        snprintf(tmp_file, sizeof tmp_file, "/proc/self/fd/%d", connection->put_data_context.fd);
        if (linkat(AT_FDCWD, tmp_file, AT_FDCWD, connection->put_data_context.filename, AT_SYMLINK_FOLLOW) != 0)
        {
            /*
             * TODO: Log error. Send error code back to client.
             */
            uint8_t response;

            response = FAIL;
            write(connection->fd, &response, sizeof response);

            goto abort_fail;
        }

        munmap(connection->put_data_context.base, connection->put_data_context.file_size);
        close(connection->put_data_context.fd);
        
        uint8_t response;
        response = OK;
        write(connection->fd, &response, sizeof response);
    }
    assert((connection->state & VERIFIED) != 0);

    if ((connection->state & MIRRORED) == 0)
    {
        uint32_t i;
        uint32_t e = connection->put_data_context.request.n;

        for (i = 0; i < e; ++i)
        {
            if (initialize_mirror(connection) == 0)
            {
                goto done;
            }
        }

        /*
         * TODO: At this point, all our mirrors have been exhausted and we have
         * been unable to connect to any of them. Perhaps we just just give up.
         */
        assert(0 && "Life sucks and then you die.");
    }

    return 0;

done:
    /*
     * We return 1 here because we've hijacked the connection with a new one to
     * our mirror so there is still work to be done, however this work will be
     * done by the mirror data handler.
     */
    return 1;

abort_fail:
    munmap(connection->put_data_context.base, connection->put_data_context.file_size);
    close(connection->put_data_context.fd);

    return 0;

}

