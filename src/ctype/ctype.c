#include <ctype.h>

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isspace(int c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
        return 1;
    default:
        return 0;
    }
}
