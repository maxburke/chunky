#include <stdio.h>
#include <string.h>

#include "mock.h"
#include "test.h"

void
register_unit_tests(void);

static void
print_help(void)
{
    puts("usage:");
    puts("  -v        verbose mode");
    puts("  -h        print this help message");
    puts("  -l        list all test cases and exit");
    puts("  -t [test] run a specific test case");
}

int
main(int argc, char **argv)
{
    struct mocks_t mocks;
    int verbose;
    int i;
    int run_specific_test;

    mock_create_default(&mocks);
    mock_set(&mocks);

    register_unit_tests();

    verbose = 0;
    for (i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            verbose = 1;
        }

        if (strcmp(argv[i], "-h") == 0)
        {
            print_help();
            return 1;
        }

        if (strcmp(argv[i], "-l") == 0)
        {
            test_list();
            return 1;
        }
    }

    run_specific_test = 0;
    for (i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-t") == 0 && (i + 1 < argc))
        {
            run_specific_test = 1;

            test_run(argv[i + 1], verbose);
            ++i;
        }
    }

    if (!run_specific_test)
    {
        test_run(NULL, verbose);
    }
}

