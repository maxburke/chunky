#include <stdint.h>
#include <unistd.h>

#include "chunky.h"

int 
message_ping(struct active_connection_t *connection)
{
    uint8_t response;

    response = OK;
    write(connection->fd, &response, sizeof response);

    return 0;
}


