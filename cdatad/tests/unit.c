#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "unit.h"
#include "test.h"

#include "../src/chunky_internal.h"

#define REGISTER_UNIT_TEST(x) \
    do { \
        strcpy(testcase.name, #x); \
        testcase.test = x; \
        test_register(&testcase); \
    } while(0)

static int
test_chunk_id_to_name(void)
{
    char buf[256];

    chunk_id_to_name(buf, sizeof buf, 0x7f);
    TEST_ASSERT(strstr(buf, "7f") != NULL);

    return 0;
}

static int
test_chunk_name_to_id(void)
{
    uint64_t id;

    TEST_ASSERT(!chunk_name_to_id(&id, "0") && id == 0);
    TEST_ASSERT(!chunk_name_to_id(&id, "7f") && id == 0x7f);
    TEST_ASSERT(chunk_name_to_id(&id, "hello"));
    TEST_ASSERT(chunk_name_to_id(&id, "7fhello"));

    return 0;
}

int unit_epoll_fd;

static int
test_get_epoll_fd(void)
{
    int fd;

    fd = get_epoll_fd();

    TEST_ASSERT(fd == 0);

    return 0;
}

void
register_unit_tests(void)
{
    struct testcase_t testcase;

    memset(&testcase, 0, sizeof testcase);

    REGISTER_UNIT_TEST(test_chunk_id_to_name);
    REGISTER_UNIT_TEST(test_chunk_name_to_id);
    REGISTER_UNIT_TEST(test_get_epoll_fd);
}

