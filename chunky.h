#pragma once

#include <stdint.h>

#include "mlb_sha1.h"

enum message_t
{
    MESSAGE_PING = 1,
    MESSAGE_GET_CHUNK_LIST = 2,
    MESSAGE_GET_CHUNK_DATA = 3,
    MESSAGE_PUT_CHUNK_DATA = 4,
    MESSAGE_DELETE_CHUNK = 5,
    MESSAGE_SHUTDOWN = 6,
    MESSAGE_MAX
};

#define MAX_REPLICATION_FACTOR 32

/*
 * PING
 *   u8 : type
 * Response:
 *   u8 : OK
 */

/*
 * GET_CHUNK_LIST
 *   u8        : type
 * Response:
 *   u8        : OK / FAIL
 *   u32       : num_chunks
 *   u64,u32*n : chunk list
 *   u8*20     : hash
 */

/*
 * GET_CHUNK_DATA
 *   u8    : type
 *   u64   : chunk_id
 *   u64   : offset
 *   u64   : size
 * Response:
 *   u8    : OK / FAIL
 *   u64   : size (not incl. hash)
 *   u8*20 : hash
 *   u8*n  : data
 */

/*
 * PUT_CHUNK_DATA
 *   u8         : type
 *   u64        : chunk_id
 *   u64        : size
 *   u32        : replication factor
 *   u32,u16*n  : replication list
 *   u8*n       : data
 *   u8*20      : hash
 * Response:
 *   u8    : OK / FAIL
 *   u8*20 : full chunk hash, if OK
 */

/*
 * DELETE_CHUNK
 *   u8  : type
 *   u64 : chunk_id
 * Response:
 *   u8  : OK / FAIL
 */

/*
 * SHUTDOWN
 */

enum response_code_t
{
    OK = 0,
    FAIL = 1
};

struct chunk_list_context_t
{
    uint32_t i;
};

struct get_data_request_t
{
    uint64_t id;
    uint64_t offset;
    uint64_t size;
} __attribute__((packed));

struct get_data_context_t
{
    struct get_data_request_t request;
    int fd;
    void *base;
    uint64_t size;
    uint64_t sent;
};

struct replication_list_t
{
    uint32_t address;
    uint16_t port;
} __attribute__((packed));

struct put_data_request_t
{
    uint64_t id;
    uint64_t size;
    uint32_t n;
    struct replication_list_t replicants[MAX_REPLICATION_FACTOR];
} __attribute__((packed));

struct put_data_context_t
{
    struct put_data_request_t request;
};

struct active_connection_t
{
    int fd;
    uint8_t message;
    char buffer[2048];
    union
    {
        struct chunk_list_context_t chunk_list_context;
        struct get_data_context_t get_data_context;
        struct put_data_context_t put_data_context;
    };
    int cursor;
    int pos;
    uint32_t state;
    struct mlb_sha1_hash_context_t hash_context;
};

int
message_get_chunk_data_handler(struct active_connection_t *connection);

int
message_put_chunk_data_handler(struct active_connection_t *connection);

void
chunk_id_to_name(char *name, size_t name_size, uint64_t id);

int
buffer_add_u8(struct active_connection_t *connection, uint8_t val);

int
buffer_add_u32(struct active_connection_t *connection, uint32_t val);

int
buffer_add_u64(struct active_connection_t *connection, uint64_t val);

int
buffer_send(struct active_connection_t *connection);

#define FLUSH_BUFFER(state, connection)     \
    do {                                    \
        if (state & FLUSHING_BUFFER)        \
        {                                   \
            int rv;                         \
                                            \
            rv = buffer_send(connection);   \
            if (rv == 1)                    \
            {                               \
                return 1;                   \
            }                               \
                                            \
            if (rv == -1)                   \
            {                               \
                return 0;                   \
            }                               \
                                            \
            state = state & ~FLUSHING_BUFFER;\
        }                                   \
    }                                       \
    while (0)


