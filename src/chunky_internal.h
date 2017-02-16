#pragma once

struct chunk_list_t
{
    uint32_t num;
    uint32_t capacity;
    uint64_t *ptr;
};

void
chunk_push(struct chunk_list_t *list, uint64_t chunk_id);

int
chunk_name_to_id(uint64_t *id, const char *name);

void
chunk_id_to_name(char *name, size_t name_size, uint64_t id);

int
chunk_list_build(struct chunk_list_t *chunks);

uint32_t
get_chunk_num(void);

const uint64_t *
get_chunks(void);

int
get_epoll_fd(void);

void
set_epoll_fd(int);

