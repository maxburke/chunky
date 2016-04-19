#pragma once

struct testcase_t
{
    /*
     * Test case name. Should be unique among all test cases registered. Allows
     * for 32-char name plus null temrinator.
     */
    char name[33];

    /*
     * Test case setup is run prior to the test case. If this function
     * returns a non-zero value the harness will assume this failed and skip
     * the execution of the test case. In the event of a failure the teardown
     * function will still be run to clean up any resources the test has
     * allocated. In the event of a success, indicated by a zero-return value,
     * the test function will be run.
     *
     * This function is optional. If setup is not desired, set this to NULL.
     */
    int (*setup)(void);

    /*
     * The test case function. If this function returns a non-zero value the 
     * harness will assume this failed. In the event of a failure the teardown
     * function will still be run to clean up any resources the test has 
     * allocated.
     */
    int (*test)(void);

    /*
     * Please see the notes for the setup and test methods for the conditions
     * under which the teardown function is run. A non-zero return value will
     * be assumed to be a failure.
     *
     * This function is optional. If teardown is not desired, set this to NULL.
     */
    int (*teardown)(void);
};

void
test_abort(const char *format, ...);

void
test_register(struct testcase_t *test);

void
test_run(const char *test_name, int verbose);

void
test_print(const char *format, ...);

