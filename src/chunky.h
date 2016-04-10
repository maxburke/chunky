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
    MESSAGE_CHUNK_MIRROR = 7,
    MESSAGE_MAX
};

enum flags_t
{
    CHUNK_NAME_TEMP = 1
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
 *   u64        : size (including hash size of 20 bytes)
 *   u32        : replication factor
 *   u32,u16*n  : replication list
 *   u8*20      : hash
 *   u8*n       : data
 * Response:
 *   u8    : OK / FAIL
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
    char filename[256];
    void *base;
    uint64_t i;
    uint64_t file_size;
    int fd;
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
    union
    {
        struct chunk_list_context_t chunk_list_context;
        struct get_data_context_t get_data_context;
        struct put_data_context_t put_data_context;
    };
    struct buffer_t buffer;
    uint32_t state;
    struct mlb_sha1_hash_context_t hash_context;
};

int 
message_initial_handler(struct connection_t *connection);

int 
message_ping(struct connection_t *connection);

int
message_get_chunk_list_handler(struct connection_t *connection);

int
message_get_chunk_data_handler(struct connection_t *connection);

int
message_put_chunk_data_handler(struct connection_t *connection);

int
message_mirror_chunk_data_handler(struct connection_t *connection);

const char *
get_data_directory(void);

uint32_t
get_chunk_num(void);

const uint64_t *
get_chunks(void);

int
get_epoll_fd(void);

int
add_to_epoll_list(int fd);

void
chunk_id_to_name(char *name, size_t name_size, uint64_t id);

int
buffer_fill(struct buffer_t *buffer, const void *data, size_t bytes);

int
buffer_add_u8(struct buffer_t *buffer, uint8_t val);

int
buffer_add_u32(struct buffer_t *buffer, uint32_t val);

int
buffer_add_u64(struct buffer_t *buffer, uint64_t val);

int
buffer_send(int fd, struct buffer_t *buffer);

#define FLUSH_BUFFER(state, fd, buffer)     \
    do {                                    \
        if (state & FLUSHING_BUFFER)        \
        {                                   \
            int rv;                         \
                                            \
            rv = buffer_send(fd, buffer);   \
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

#define PERROR() do { \
        char buf[128]; \
        snprintf(buf, sizeof buf, "%s(%d)", __FILE__, __LINE__); \
        perror(buf); \
    } while(0)

#define VERIFY(x) if (!(x)) { PERROR(); return -1; }

