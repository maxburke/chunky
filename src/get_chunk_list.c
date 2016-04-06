#include "chunky.h"
    
int
message_get_chunk_list_handler(struct connection_t *connection)
{
    uint32_t state;
    uint32_t num_chunks;

    enum states_t
    {
        FLUSHING_BUFFER = 1 << 0,
        SENT_OK = 1 << 1,
        SENT_NUM_CHUNKS = 1 << 2,
        SENT_CHUNKS = 1 << 3,
        SENT_HASH = 1 << 4,
    };

    state = connection->state;

    FLUSH_BUFFER(state, connection);

    if ((state & SENT_OK) == 0)
    {
        if (buffer_add_u8(connection, OK) != 0)
        {
            goto chunk_list_flush;
        }

        state |= SENT_OK;
    }

    num_chunks = get_chunk_num();

    if ((state & SENT_NUM_CHUNKS) == 0)
    {
        if (buffer_add_u32(connection, num_chunks) != 0)
        {
            goto chunk_list_flush;
        }

        state |= SENT_NUM_CHUNKS;
    }

    if ((state & SENT_CHUNKS) == 0)
    {
        uint32_t i, e;
        const uint64_t *chunks;

        chunks = get_chunks();
        for (i = connection->chunk_list_context.i, e = num_chunks; i < e; ++i)
        {
            uint64_t chunk;

            chunk = chunks[i];
            if (buffer_add_u64(connection, chunk) != 0)
            {
                connection->chunk_list_context.i = i;
                goto chunk_list_flush;
            }
            mlb_sha1_hash_update(&connection->hash_context, &chunk, sizeof chunk);
        }

        state |= SENT_CHUNKS;
    }

    if ((state & SENT_HASH) == 0)
    {
        struct mlb_sha1_hash_t hash;

        hash = mlb_sha1_hash_finalize(&connection->hash_context);
        if (buffer_fill(connection, &hash, sizeof hash) != 0)
        {
            goto chunk_list_flush;
        }

        state |= SENT_HASH;
    }

    return 0;

chunk_list_flush:
    connection->state = state | FLUSHING_BUFFER;
    return 1;
}

