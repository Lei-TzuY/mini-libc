#include <locale.h>
#include <stddef.h>

#include "locale_internal.h"

#define MINI_CHAR_MAX 127

static char mini_locale_c[] = "C";
static char mini_locale_utf8[] = "C.UTF-8";
static char mini_decimal_point[] = ".";
static char mini_empty[] = "";
static int mini_ctype_utf8;

static struct lconv mini_c_locale = {
    mini_decimal_point,
    mini_empty,
    mini_empty,
    mini_empty,
    mini_empty,
    mini_empty,
    mini_empty,
    mini_empty,
    mini_empty,
    mini_empty,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX,
    MINI_CHAR_MAX
};

static int valid_category(int category)
{
    return category == LC_CTYPE || category == LC_NUMERIC ||
           category == LC_TIME || category == LC_COLLATE ||
           category == LC_MONETARY || category == LC_ALL;
}

static int same_string(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }
        ++left;
        ++right;
    }
    return *left == *right;
}

static int utf8_name(const char *locale)
{
    return same_string(locale, "C.UTF-8") || same_string(locale, "C.utf8");
}

int __mini_locale_is_utf8(void)
{
    return mini_ctype_utf8;
}

size_t __mini_mb_cur_max(void)
{
    return mini_ctype_utf8 ? 4U : 1U;
}

char *setlocale(int category, const char *locale)
{
    if (!valid_category(category)) {
        return (char *)0;
    }

    if (locale == (const char *)0) {
        if (category == LC_CTYPE || category == LC_ALL) {
            return mini_ctype_utf8 ? mini_locale_utf8 : mini_locale_c;
        }
        return mini_locale_c;
    }

    if (category == LC_CTYPE || category == LC_ALL) {
        if (*locale == '\0' || same_string(locale, "C")) {
            mini_ctype_utf8 = 0;
            return mini_locale_c;
        }
        if (utf8_name(locale)) {
            mini_ctype_utf8 = 1;
            return mini_locale_utf8;
        }
        return (char *)0;
    }

    if (*locale == '\0' || same_string(locale, "C")) {
        return mini_locale_c;
    }
    return (char *)0;
}

struct lconv *localeconv(void)
{
    return &mini_c_locale;
}
