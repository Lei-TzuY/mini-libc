#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

static int write_all(const char *buffer, size_t length)
{
    while (length != 0) {
        long result = mini_sys_write(1, buffer, (unsigned long)length);

        if (result < 0) {
            errno = (int)-result;
            return EOF;
        }
        if (result == 0) {
            errno = EIO;
            return EOF;
        }
        buffer += (size_t)result;
        length -= (size_t)result;
    }
    return 0;
}

int putchar(int c)
{
    unsigned char byte = (unsigned char)c;

    if (write_all((const char *)&byte, 1) == EOF) {
        return EOF;
    }
    return (int)byte;
}

int puts(const char *s)
{
    size_t length = 0;

    while (s[length] != '\0') {
        ++length;
    }
    if (write_all(s, length) == EOF || write_all("\n", 1) == EOF) {
        return EOF;
    }
    return 0;
}
