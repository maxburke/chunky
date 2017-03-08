#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

struct testcase_link_t
{
    struct testcase_link_t *next;
    struct testcase_t testcase;
};

struct fixture_t
{
    int passed;
    int failed;
    int verbose;
    struct testcase_link_t *tests;
    jmp_buf context;
};

struct fixture_t fixture;

void
test_abort(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    printf("ABORT: ");
    vprintf(format, args);
    va_end(args);

    longjmp(fixture.context, 1);
}

void
test_list(void)
{
    struct testcase_link_t *p;

    puts("Registered test cases:");

    for (p = fixture.tests; p != NULL; p = p->next)
    {
        printf("  %s\n", p->testcase.name);
    }
}

void
test_register(struct testcase_t *test)
{
    struct testcase_link_t *ptr;

    ptr = calloc(sizeof(struct testcase_link_t), 1);
    ptr->testcase = *test;
    ptr->next = fixture.tests;
    fixture.tests = ptr;
}

static void
testcase_run(struct testcase_link_t *p, const char *test_name)
{
    int failed;
    
    failed = 0;

    if (test_name != NULL && strcmp(p->testcase.name, test_name) != 0)
    {
        return;
    }

    printf("%32s ", p->testcase.name);

    /*
     * The test fixture uses setjmp and longjmp to allow cases to perform
     * non-local exits should the need to immediately abort arise.
     */
    if (setjmp(fixture.context) == 0)
    {
        if (p->testcase.setup && p->testcase.setup() != 0)
        {
            failed = 1;
            goto teardown;
        }
    }
    else
    {
        failed = 1;
        goto teardown;
    }

    if (setjmp(fixture.context) == 0)
    {
        if (!p->testcase.test || p->testcase.test() != 0)
        {
            failed = 1;
            goto teardown;
        }
    }
    else
    {
        failed = 1;
        goto teardown;
    }

teardown:
    if (setjmp(fixture.context) == 0)
    {
        if (p->testcase.teardown && p->testcase.teardown() != 0)
        {
            failed = 1;
        }
    }
    else
    {
        failed = 1;
    }

    fixture.passed += (1 - failed);
    fixture.failed +=      failed;
    printf(" ... %s\n", failed ? "FAILED" : "passed");
}

static void
test_run_recursive(struct testcase_link_t *p, const char *test_name)
{
    if (p->next != NULL)
    {
        test_run_recursive(p->next, test_name);
    }

    testcase_run(p, test_name);
}

void
test_run(const char *test_name, int verbose)
{
    fixture.verbose = verbose;

    test_run_recursive(fixture.tests, test_name);
}

void
test_shutdown(void)
{
    struct testcase_link_t *p;

    for (p = fixture.tests; p != NULL;)
    {
        struct testcase_link_t *next;

        next = p->next;
        free(p);
        p = next;
    }
}

void
test_print(const char *format, ...)
{
    va_list args;

    if (!fixture.verbose)
    {
        return;
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void
test_print_location(const char *file, int line, const char *format, ...)
{
    va_list args;

    if (!fixture.verbose)
    {
        return;
    }

    printf("%s(%d): ", file, line);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

