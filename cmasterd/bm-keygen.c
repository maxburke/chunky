#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <nacl/crypto_box.h>
   
const char secret_key_filename[] = "blockmaster.private.key";
const char public_key_filename[] = "blockmaster.public.key";

static int
write_key_data(const char *filename, const unsigned char *key, size_t key_size)
{
    int fd;
    ssize_t bytes_written;
    
    fd = creat(filename, S_IRUSR | S_IWUSR);
    if (fd <= 0)
    {
        char buf[512];
        snprintf(buf, sizeof buf, "Unable to open key file '%s'. If the key files exist already, please delete them and re-try.", filename);
        perror(buf);
        return -1;
    }

    bytes_written = write(fd, key, key_size);

    if (bytes_written >= 0 && ((size_t)bytes_written) != key_size)
    {
        fprintf(stderr, "Unable to write key file '%s', please try again.\n", filename);
        
        close(fd);
        unlink(filename);

        return -1;
    }

    printf("Written key file '%s' (%lu bytes)\n", filename, key_size);
    close(fd);
    chmod(filename, S_IRUSR);

    return 0;
}

int
main(void)
{
    unsigned char secret_key[crypto_box_SECRETKEYBYTES];
    unsigned char public_key[crypto_box_PUBLICKEYBYTES];

    crypto_box_keypair(public_key, secret_key);
    if (write_key_data(secret_key_filename, secret_key, sizeof secret_key) < 0)
        return -1;

    if (write_key_data(public_key_filename, public_key, sizeof public_key) < 0)
        return -1;

    return 0;
}

