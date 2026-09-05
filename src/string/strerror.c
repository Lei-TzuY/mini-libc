#include <errno.h>
#include <string.h>

char *strerror(int errnum)
{
    static char enoent[] = "No such file or directory";
    static char eio[] = "Input/output error";
    static char enomem[] = "Cannot allocate memory";
    static char eexist[] = "File exists";
    static char einval[] = "Invalid argument";
    static char erange[] = "Numerical result out of range";
    static char unknown[] = "Unknown error";

    switch (errnum) {
    case ENOENT:
        return enoent;
    case EIO:
        return eio;
    case ENOMEM:
        return enomem;
    case EEXIST:
        return eexist;
    case EINVAL:
        return einval;
    case ERANGE:
        return erange;
    default:
        return unknown;
    }
}
