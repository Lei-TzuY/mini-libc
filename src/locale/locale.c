#include <locale.h>
#include <stddef.h>
#include <stdlib.h>

#include "locale_internal.h"

#define MINI_CHAR_MAX 127
#define MINI_CATEGORY_COUNT 5

enum mini_locale_mode {
    MINI_LOCALE_C = 0,
    MINI_LOCALE_C_UTF8 = 1
};

struct mini_locale_state {
    enum mini_locale_mode category[MINI_CATEGORY_COUNT];
};

static char mini_locale_c[] = "C";
static char mini_locale_utf8[] = "C.UTF-8";
static char mini_locale_mixed[] =
    "LC_CTYPE=C.UTF-8;LC_NUMERIC=C;LC_TIME=C;LC_COLLATE=C;LC_MONETARY=C";
static char mini_decimal_point[] = ".";
static char mini_empty[] = "";
static struct mini_locale_state mini_process_locale = {
    {MINI_LOCALE_C, MINI_LOCALE_C, MINI_LOCALE_C, MINI_LOCALE_C, MINI_LOCALE_C}
};

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

static int state_category(int category)
{
    return category >= LC_CTYPE && category <= LC_MONETARY;
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

static const char *category_environment_name(int category)
{
    switch (category) {
    case LC_CTYPE:
        return "LC_CTYPE";
    case LC_NUMERIC:
        return "LC_NUMERIC";
    case LC_TIME:
        return "LC_TIME";
    case LC_COLLATE:
        return "LC_COLLATE";
    case LC_MONETARY:
        return "LC_MONETARY";
    default:
        return (const char *)0;
    }
}

static const char *nonempty_environment(const char *name)
{
    char *value = getenv(name);

    if (value == (char *)0 || *value == '\0') {
        return (const char *)0;
    }
    return value;
}

static const char *environment_locale(int category)
{
    const char *value;
    const char *category_name;

    value = nonempty_environment("LC_ALL");
    if (value != (const char *)0) {
        return value;
    }

    category_name = category_environment_name(category);
    if (category_name != (const char *)0) {
        value = nonempty_environment(category_name);
        if (value != (const char *)0) {
            return value;
        }
    }

    value = nonempty_environment("LANG");
    return value != (const char *)0 ? value : mini_locale_c;
}

static int all_categories_c(void)
{
    int category;

    for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
        if (mini_process_locale.category[category] != MINI_LOCALE_C) {
            return 0;
        }
    }
    return 1;
}

static char *current_category_locale(int category)
{
    return mini_process_locale.category[category] == MINI_LOCALE_C_UTF8
               ? mini_locale_utf8
               : mini_locale_c;
}

static char *current_lc_all(void)
{
    if (all_categories_c()) {
        return mini_locale_c;
    }

    /*
     * The bounded runtime currently permits only LC_CTYPE to differ from C.
     * Keep the aggregate name opaque but restorable instead of pretending that
     * the process is wholly in C.UTF-8.
     */
    if (mini_process_locale.category[LC_CTYPE] == MINI_LOCALE_C_UTF8) {
        return mini_locale_mixed;
    }
    return (char *)0;
}

static char *current_locale(int category)
{
    if (category == LC_ALL) {
        return current_lc_all();
    }
    return current_category_locale(category);
}

static char *apply_category_locale(int category, const char *locale)
{
    if (!state_category(category)) {
        return (char *)0;
    }

    if (same_string(locale, "C")) {
        mini_process_locale.category[category] = MINI_LOCALE_C;
        return mini_locale_c;
    }

    if (category == LC_CTYPE && utf8_name(locale)) {
        mini_process_locale.category[category] = MINI_LOCALE_C_UTF8;
        return mini_locale_utf8;
    }

    return (char *)0;
}

static char *apply_lc_all(const char *locale)
{
    int category;

    if (same_string(locale, "C")) {
        for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
            mini_process_locale.category[category] = MINI_LOCALE_C;
        }
        return mini_locale_c;
    }

    if (utf8_name(locale) || same_string(locale, mini_locale_mixed)) {
        for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
            mini_process_locale.category[category] = MINI_LOCALE_C;
        }
        mini_process_locale.category[LC_CTYPE] = MINI_LOCALE_C_UTF8;
        return mini_locale_mixed;
    }

    return (char *)0;
}

int __mini_locale_is_utf8(void)
{
    return mini_process_locale.category[LC_CTYPE] == MINI_LOCALE_C_UTF8;
}

size_t __mini_mb_cur_max(void)
{
    return __mini_locale_is_utf8() ? 4U : 1U;
}

char *setlocale(int category, const char *locale)
{
    if (!valid_category(category)) {
        return (char *)0;
    }

    if (locale == (const char *)0) {
        return current_locale(category);
    }

    if (*locale == '\0') {
        locale = environment_locale(category);
    }

    if (category == LC_ALL) {
        return apply_lc_all(locale);
    }
    return apply_category_locale(category, locale);
}

struct lconv *localeconv(void)
{
    return &mini_c_locale;
}
