#pragma once

#include <stdint.h>

enum blockmaster_message_t
{
    MESSAGE_PING = 1,
    /*
     * Not yet implemented
     */
    MESSAGE_VOTE,
    MESSAGE_LOG_ADD_FILE,
    MESSAGE_LOG_DELETE_FILE,
    
    MESSAGE_SHUTDOWN,

    /*
     * Not a message.
     */
    MESSAGE_MAX
};

enum log_entry_type_t
{
    FILE_ADD,
    FILE_DELETE,
    FILE_UPDATE
};

struct log_entry_t
{
    uint32_t log_entry_type;
    uint64_t block_id;
    uint32_t
};

enum response_code_t
{
    OK = 0,
    FAIL = 1
};

struct buffer_t
{
    char buf[2048];
    int cursor;
    int pos;
};

struct connection_t
{
    int fd;
    uint8_t message;
    struct buffer_t buffer;
    uint32_t state;
};

/*
 * Message handlers must be re-entrant.
 * Return value is zero if processing of the message is complete,
 * non-zero otherwise.
 */
typedef int (*message_handler_t)(struct connection_t *);


