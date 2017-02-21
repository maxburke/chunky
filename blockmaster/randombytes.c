#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <linux/random.h>
#include <sys/syscall.h>

void randombytes(unsigned char *buf, unsigned long long len)
{
    unsigned long long bytes_remaining;

    bytes_remaining = len;

    while (bytes_remaining != 0)
    {
        int rv;

        rv = syscall(SYS_getrandom, buf, (size_t)bytes_remaining, 0);

        if (rv > 0)
        {
            bytes_remaining -= (unsigned long long)rv;
            buf += rv;
        }
        else
        {
            perror("Unable to generate random bytes!");
            abort();
        }
    }
}
