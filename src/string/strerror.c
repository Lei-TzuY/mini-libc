#include <errno.h>
#include <string.h>

char *strerror(int errnum)
{
    static char eperm[] = "Operation not permitted";
    static char enoent[] = "No such file or directory";
    static char esrch[] = "No such process";
    static char eio[] = "Input/output error";
    static char echild[] = "No child processes";
    static char eagain[] = "Resource temporarily unavailable";
    static char enomem[] = "Cannot allocate memory";
    static char eexist[] = "File exists";
    static char enotdir[] = "Not a directory";
    static char eisdir[] = "Is a directory";
    static char einval[] = "Invalid argument";
    static char efbig[] = "File too large";
    static char epipe[] = "Broken pipe";
    static char edom[] = "Numerical argument out of domain";
    static char erange[] = "Numerical result out of range";
    static char eilseq[] = "Invalid or incomplete multibyte or wide character";
    static char unknown[] = "Unknown error";

    switch (errnum) {
    case EPERM:
        return eperm;
    case ENOENT:
        return enoent;
    case ESRCH:
        return esrch;
    case EIO:
        return eio;
    case ECHILD:
        return echild;
    case EAGAIN:
        return eagain;
    case ENOMEM:
        return enomem;
    case EEXIST:
        return eexist;
    case ENOTDIR:
        return enotdir;
    case EISDIR:
        return eisdir;
    case EINVAL:
        return einval;
    case EFBIG:
        return efbig;
    case EPIPE:
        return epipe;
    case EDOM:
        return edom;
    case ERANGE:
        return erange;
    case EILSEQ:
        return eilseq;
    default:
        return unknown;
    }
}
