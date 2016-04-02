#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "chunky.h"

enum get_data_states_t
{
    FLUSHING_BUFFER = 1 << 0,
    READ_REQUEST = 1 << 1,
    MAPPED_FILE = 1 << 2,
    SENT_SIZE = 1 << 3,
    SENT_DATA = 1 << 4
};

static int
read_get_data_request(struct active_connection_t *connection)
{
    char *buffer;
    uint32_t pos;
    ssize_t bytes_expected;
    ssize_t bytes_read;

    buffer = connection->buffer;
    pos = connection->pos;
    bytes_expected = sizeof(struct get_data_request_t) - pos;
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

    memmove(&connection->get_data_context.request, buffer, sizeof(struct get_data_request_t));

    connection->state |= READ_REQUEST;
    connection->pos = 0;
    memset(buffer, 0, sizeof(struct get_data_request_t));


    return 0;
}

static int
map_file_for_read(struct active_connection_t *connection)
{
    uint64_t id;
    char chunk_file[256];
    int fd;
    struct stat stat_buf;
    void *base;
    uint64_t size;

    id = connection->get_data_context.request.id;
    chunk_id_to_name(chunk_file, sizeof chunk_file, id);
    fd = open(chunk_file, O_RDONLY);

    if (fd == -1)
    {
        /*
         * TODO: Log error. Send error code back to client.
         */

        uint8_t response;

        response = FAIL;
        write(connection->fd, &response, sizeof response);

        return -1;
    }

    if (fstat(fd, &stat_buf) == -1)
    {
        /*
         * TODO: Again, log, send failure and return.
         */

        uint8_t response;

        response = FAIL;
        write(connection->fd, &response, sizeof response);
        close(fd);

        return -1;
    }

    size = stat_buf.st_size;
    base = mmap(NULL, size, PROT_READ, MAP_PRIVATE /* | MAP_HUGETLB */, fd, 0);

    if (base == MAP_FAILED)
    {
        /*
         * TODO: Yet again, log, send failure and return.
         */

        uint8_t response;

        response = FAIL;
        write(connection->fd, &response, sizeof response);
        close(fd);

        return -1;
    }

    connection->get_data_context.fd = fd;
    connection->get_data_context.base = base;
    connection->get_data_context.size = size;

    connection->state |= MAPPED_FILE;

    return 0;
}

int
message_get_chunk_data_handler(struct active_connection_t *connection)
{
    if ((connection->state & READ_REQUEST) != 0)
    {
        int rv;

        rv = read_get_data_request(connection);

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

    if ((connection->state & MAPPED_FILE) == 0)
    {
        if (map_file_for_read(connection) != 0)
        {
            /*
             * If map fails, the FAIL packet will have been sent and so
             * we can kill the connection.
             */
            return 0;
        }

        buffer_add_u8(connection, OK);
    }
    assert((connection->state & MAPPED_FILE) != 0);

    FLUSH_BUFFER(connection->state, connection);

    if ((connection->state & SENT_SIZE) == 0)
    {
        buffer_add_u64(connection, connection->get_data_context.size);

        connection->state |= SENT_SIZE;
    }
    assert((connection->state & SENT_SIZE) != 0);

    FLUSH_BUFFER(connection->state, connection);

    /*
     * The data format includes a leading 20-bytes in the file with the content
     * hash. This leads to a discrepancy with what is sent below versus what is
     * described in the wire format in chunky.h.
     */
    if ((connection->state & SENT_DATA) == 0)
    {
        struct get_data_context_t *context;
        uint64_t size;
        uint64_t sent;
        uint64_t remaining;
        char *ptr;
        ssize_t bytes_sent;

        context = &connection->get_data_context;
        size = context->size;
        sent = context->sent;

        assert(size >= sent);
        remaining = size - sent;

        ptr = (char *)context->base + sent;
        bytes_sent = write(context->fd, ptr, remaining);

        if (bytes_sent < 0)
        {
            /*
             * TODO: Log and handle error.
             */
            goto abort_fail;
        }

        if (bytes_sent == 0 && remaining != 0)
        {
            /*
             * TODO: Log and handle error. Remote dropped connection?
             */

            goto abort_fail;
        }

        context->sent = sent + bytes_sent;

        if ((size_t)bytes_sent == remaining)
        {
            /*
             * All data we expected to send has been sent. Onto the next
             * phase!
             */

            connection->state |= SENT_DATA;
            return 0;
        }
    }

    return 1;

abort_fail:
    munmap(connection->get_data_context.base, connection->get_data_context.size);
    close(connection->get_data_context.fd);

    return 0;
}

