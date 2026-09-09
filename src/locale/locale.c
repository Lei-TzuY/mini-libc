#include <locale.h>

#define MINI_CHAR_MAX 127

static char mini_locale_name[] = "C";
static char mini_decimal_point[] = ".";
static char mini_empty[] = "";

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

char *setlocale(int category, const char *locale)
{
    if (!valid_category(category)) {
        return (char *)0;
    }
    if (locale == (const char *)0) {
        return mini_locale_name;
    }
    if (*locale == '\0' || same_string(locale, "C")) {
        return mini_locale_name;
    }
    return (char *)0;
}

struct lconv *localeconv(void)
{
    return &mini_c_locale;
}
