#include <ctype.h>

#include "../locale/locale_internal.h"

static int isalpha_byte(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int isblank_byte(int c)
{
    return c == ' ' || c == '\t';
}

static int isdigit_byte(int c)
{
    return c >= '0' && c <= '9';
}

static int isalnum_byte(int c)
{
    return isalpha_byte(c) || isdigit_byte(c);
}

static int iscntrl_byte(int c)
{
    return (c >= 0x00 && c <= 0x1f) || c == 0x7f;
}

static int isgraph_byte(int c)
{
    return c >= 0x21 && c <= 0x7e;
}

static int islower_byte(int c)
{
    return c >= 'a' && c <= 'z';
}

static int isprint_byte(int c)
{
    return c >= 0x20 && c <= 0x7e;
}

static int ispunct_byte(int c)
{
    return isgraph_byte(c) && !isalnum_byte(c);
}

static int isspace_byte(int c)
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

static int isupper_byte(int c)
{
    return c >= 'A' && c <= 'Z';
}

static int isxdigit_byte(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

static int tolower_byte(int c)
{
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static int toupper_byte(int c)
{
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

static int locale_object_valid(locale_t locale)
{
    struct mini_locale_state state;

    return __mini_locale_object_copy_state(locale, &state);
}

int isalpha(int c)
{
    return isalpha_byte(c);
}

int isblank(int c)
{
    return isblank_byte(c);
}

int isdigit(int c)
{
    return isdigit_byte(c);
}

int isalnum(int c)
{
    return isalnum_byte(c);
}

int iscntrl(int c)
{
    return iscntrl_byte(c);
}

int isgraph(int c)
{
    return isgraph_byte(c);
}

int islower(int c)
{
    return islower_byte(c);
}

int isprint(int c)
{
    return isprint_byte(c);
}

int ispunct(int c)
{
    return ispunct_byte(c);
}

int isspace(int c)
{
    return isspace_byte(c);
}

int isupper(int c)
{
    return isupper_byte(c);
}

int isxdigit(int c)
{
    return isxdigit_byte(c);
}

int tolower(int c)
{
    return tolower_byte(c);
}

int toupper(int c)
{
    return toupper_byte(c);
}

#define MINI_DEFINE_CTYPE_L(name, mode_fn)                                   \
    int name##_l(int c, locale_t locale)                                     \
    {                                                                         \
        if (!locale_object_valid(locale)) {                                  \
            return 0;                                                         \
        }                                                                     \
        return mode_fn(c);                                                    \
    }

MINI_DEFINE_CTYPE_L(isalnum, isalnum_byte)
MINI_DEFINE_CTYPE_L(isalpha, isalpha_byte)
MINI_DEFINE_CTYPE_L(isblank, isblank_byte)
MINI_DEFINE_CTYPE_L(iscntrl, iscntrl_byte)
MINI_DEFINE_CTYPE_L(isdigit, isdigit_byte)
MINI_DEFINE_CTYPE_L(isgraph, isgraph_byte)
MINI_DEFINE_CTYPE_L(islower, islower_byte)
MINI_DEFINE_CTYPE_L(isprint, isprint_byte)
MINI_DEFINE_CTYPE_L(ispunct, ispunct_byte)
MINI_DEFINE_CTYPE_L(isspace, isspace_byte)
MINI_DEFINE_CTYPE_L(isupper, isupper_byte)
MINI_DEFINE_CTYPE_L(isxdigit, isxdigit_byte)

int tolower_l(int c, locale_t locale)
{
    if (!locale_object_valid(locale)) {
        return c;
    }
    return tolower_byte(c);
}

int toupper_l(int c, locale_t locale)
{
    if (!locale_object_valid(locale)) {
        return c;
    }
    return toupper_byte(c);
}
