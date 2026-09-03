#include <errno.h>
#include <string.h>

char *strerror(int errnum)
{
    static char eio[] = "Input/output error";
    static char enomem[] = "Cannot allocate memory";
    static char einval[] = "Invalid argument";
    static char erange[] = "Numerical result out of range";
    static char unknown[] = "Unknown error";

    switch (errnum) {
    case EIO:
        return eio;
    case ENOMEM:
        return enomem;
    case EINVAL:
        return einval;
    case ERANGE:
        return erange;
    default:
        return unknown;
    }
}
