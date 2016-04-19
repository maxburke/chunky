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
test_register(struct testcase_t *test)
{
    struct testcase_link_t *ptr;

    ptr = malloc(sizeof(struct testcase_link_t));
    ptr->testcase = *test;
    ptr->next = fixture.tests;
    fixture.tests = ptr;
}

void
test_run(const char *test_name, int verbose)
{
    struct testcase_link_t *p;

    fixture.verbose = verbose;

    for (p = fixture.tests; p != NULL; p = p->next)
    {
        int failed;
        
        failed = 0;

        if (test_name != NULL && strcmp(p->testcase.name, test_name) != 0)
        {
            continue;
        }

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
            if (p->testcase.teardown && p->testcase.teardown != 0)
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
        printf("%32s ... %s\n", p->testcase.name, failed ? "FAILED" : "passed");
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

