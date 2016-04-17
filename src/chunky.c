#define _BSD_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "chunky.h"
#include "chunky_internal.h"


void
chunk_push(struct chunk_list_t *list, uint64_t chunk_id)
{
    assert(list != NULL);

    uint32_t capacity = list->capacity;
    uint32_t num = list->num;
    uint64_t *chunks = list->ptr;

    if (num == capacity)
    {
        uint32_t new_capacity = 1.5 * (double)capacity;
        
        if (capacity == 0)
        {
            new_capacity = 1 << 10;
        }
        
        chunks = realloc(chunks, new_capacity * sizeof(uint64_t));
        list->ptr = chunks;
        list->capacity = new_capacity;
    }

    chunks[num++] = chunk_id;
    list->num = num;
}

static int
chunk_id_predicate(const void *ptr_a, const void *ptr_b)
{
    const uint64_t *a = ptr_a;
    const uint64_t *b = ptr_b;

    return *a - *b;
}

int
chunk_name_to_id(uint64_t *id, const char *name)
{
    char *endptr;

    assert(id != NULL);
    *id = strtoul(name, &endptr, 16);

    if (endptr == name)
    {
        *id = 0;
        return 1;
    }

    if (*endptr != 0)
    {
        *id = 0;
        return 1;
    }

    return 0;
}

void
chunk_id_to_name(char *name, size_t name_size, uint64_t id)
{
    const char *data_directory;

    data_directory = get_data_directory();

    if (snprintf(name, name_size, "%s/%0lx", data_directory, id)
            >= (int)name_size)
    {
        fprintf(stderr, "Path length too long, string '%s' was truncated. Aborting.", name);
        abort();
    }
}

int
chunk_list_build(struct chunk_list_t *chunks_ptr)
{
    const char *data_directory;
    DIR *directory;
    struct dirent *entry;
    int i;

    data_directory = get_data_directory();
    directory = opendir(data_directory);

    VERIFY(directory != NULL);

    errno = 0;
    i = 0;

    while ((entry = readdir(directory)) != NULL)
    {
        uint64_t chunk_id;

        if (entry->d_type == DT_DIR)
        {
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            {
                continue;
            }
        }

        if (chunk_name_to_id(&chunk_id, entry->d_name))
        {
            fprintf(stderr, "File '%s' in chunk data directory is of an unexpected name format!", entry->d_name);
            return 1;
        }
       
        chunk_push(chunks_ptr, chunk_id);
    }

    qsort(chunks_ptr->ptr, i, sizeof(uint64_t), chunk_id_predicate);

    closedir(directory);

    return 0;
}


int
buffer_fill(struct buffer_t *buffer, const void *data, size_t bytes)
{
    int cursor = buffer->cursor;
    if (cursor + bytes >= sizeof buffer->buf)
    {
        return -1;
    }

    uint8_t *p = (uint8_t *)(&buffer->buf[cursor]);
    memcpy(p, data, bytes);
    buffer->cursor = cursor + bytes;

    return 0;
}

int
buffer_add_u8(struct buffer_t *buffer, uint8_t val)
{
    return buffer_fill(buffer, &val, sizeof val);
}

int
buffer_add_u32(struct buffer_t *buffer, uint32_t val)
{
    return buffer_fill(buffer, &val, sizeof val);
}

int
buffer_add_u64(struct buffer_t *buffer, uint64_t val)
{
    return buffer_fill(buffer, &val, sizeof val);
}

int
buffer_send(int fd, struct buffer_t *buffer)
{
    ssize_t bytes_sent;
    char *buf = buffer->buf;
    uint32_t cursor = buffer->cursor;
    uint32_t pos = buffer->pos;
    uint32_t to_send = cursor - pos;
   
    if (to_send > 0)
    {
        bytes_sent = write(fd, buf + pos, cursor - pos);

        if (bytes_sent != to_send)
        {
            if (bytes_sent > 0)
            {
                buffer->pos = pos + bytes_sent;
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }

    buffer->pos = 0;
    buffer->cursor = 0;

    return 0;
}


