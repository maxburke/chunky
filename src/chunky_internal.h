#pragma once

struct chunk_list_t
{
    uint32_t num;
    uint32_t capacity;
    uint64_t *ptr;
};

uint32_t
get_chunk_num(void);

const uint64_t *
get_chunks(void);

int
get_epoll_fd(void);

