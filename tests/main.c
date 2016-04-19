#include "mock.h"

int
main(void)
{
    struct mocks_t mocks;

    mock_create_default(&mocks);
    mock_set(&mocks);
}

