#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "chunky.h"

enum put_data_states_t
{
    FLUSHING_BUFFER = 1 << 0,
    READ_REQUEST = 1 << 1,
};

static int
read_put_data_request(struct active_connection_t *connection)
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

    return 0;
}

int
message_put_chunk_data_handler(struct active_connection_t *connection)
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

    return 0;
}

