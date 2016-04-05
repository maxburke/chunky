#include <unistd.h>

#include "chunky.h"
    
int 
message_initial_handler(struct active_connection_t *connection)
{
    uint8_t message;
    int rv;
   
    message = 0;
    rv = read(connection->fd, &message, sizeof message);

    if (rv <= 0)
    {
        return 0;
    }

    connection->message = message;

    return 1;
}


