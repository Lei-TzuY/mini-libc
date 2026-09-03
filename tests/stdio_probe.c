#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

int main(void)
{
    int result;

    errno = ERANGE;
    result = putchar('A');
    if (result != 'A' || errno != ERANGE) {
        return 1;
    }

    result = putchar(0x141);
    if (result != 'A' || errno != ERANGE) {
        return 2;
    }

    result = puts("BC");
    if (result < 0 || errno != ERANGE) {
        return 3;
    }

    result = puts("");
    if (result < 0 || errno != ERANGE) {
        return 4;
    }

    {
        static const char marker[] = "stdio-ok";
        long written = mini_sys_write(1, marker, sizeof(marker) - 1);

        if (written != (long)(sizeof(marker) - 1)) {
            return 5;
        }
    }

    return 0;
}
