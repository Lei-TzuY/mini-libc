#include <locale.h>
#include <stddef.h>
#include <stdlib.h>

#include "locale_internal.h"

#define MINI_CHAR_MAX 127

static char mini_locale_c[] = "C";
static char mini_locale_utf8[] = "C.UTF-8";
static char mini_locale_mixed[] =
    "LC_CTYPE=C.UTF-8;LC_NUMERIC=C;LC_TIME=C;LC_COLLATE=C;LC_MONETARY=C";
static char mini_decimal_point[] = ".";
static char mini_empty[] = "";
static struct mini_locale_state mini_process_locale = {
    {MINI_LOCALE_MODE_C, MINI_LOCALE_MODE_C, MINI_LOCALE_MODE_C,
     MINI_LOCALE_MODE_C, MINI_LOCALE_MODE_C}
};
static mini_locale_state_provider_t mini_locale_state_provider;

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

static int c_name(const char *locale)
{
    return same_string(locale, "C") || same_string(locale, "POSIX");
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

void __mini_locale_state_init(struct mini_locale_state *state)
{
    int category;

    if (state == (struct mini_locale_state *)0) {
        return;
    }
    for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
        state->category[category] = MINI_LOCALE_MODE_C;
    }
}

void __mini_locale_state_copy(struct mini_locale_state *dst,
                              const struct mini_locale_state *src)
{
    int category;

    if (dst == (struct mini_locale_state *)0 ||
        src == (const struct mini_locale_state *)0) {
        return;
    }
    for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
        dst->category[category] = src->category[category];
    }
}

static int all_categories_c(const struct mini_locale_state *state)
{
    int category;

    for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
        if (state->category[category] != MINI_LOCALE_MODE_C) {
            return 0;
        }
    }
    return 1;
}

static const char *current_lc_all(const struct mini_locale_state *state)
{
    if (all_categories_c(state)) {
        return mini_locale_c;
    }

    if (state->category[LC_CTYPE] == MINI_LOCALE_MODE_C_UTF8 &&
        state->category[LC_NUMERIC] == MINI_LOCALE_MODE_C &&
        state->category[LC_TIME] == MINI_LOCALE_MODE_C &&
        state->category[LC_COLLATE] == MINI_LOCALE_MODE_C &&
        state->category[LC_MONETARY] == MINI_LOCALE_MODE_C) {
        return mini_locale_mixed;
    }
    return (const char *)0;
}

const char *__mini_locale_state_query(const struct mini_locale_state *state,
                                      int category)
{
    if (state == (const struct mini_locale_state *)0 ||
        !valid_category(category)) {
        return (const char *)0;
    }

    if (category == LC_ALL) {
        return current_lc_all(state);
    }
    return state->category[category] == MINI_LOCALE_MODE_C_UTF8
               ? mini_locale_utf8
               : mini_locale_c;
}

static int apply_category_unchecked(struct mini_locale_state *state,
                                    int category, const char *locale)
{
    if (!state_category(category)) {
        return 0;
    }

    if (c_name(locale)) {
        state->category[category] = MINI_LOCALE_MODE_C;
        return 1;
    }

    if (category == LC_CTYPE && utf8_name(locale)) {
        state->category[category] = MINI_LOCALE_MODE_C_UTF8;
        return 1;
    }

    return 0;
}

static int apply_lc_all_unchecked(struct mini_locale_state *state,
                                  const char *locale)
{
    if (c_name(locale)) {
        __mini_locale_state_init(state);
        return 1;
    }

    if (utf8_name(locale) || same_string(locale, mini_locale_mixed)) {
        __mini_locale_state_init(state);
        state->category[LC_CTYPE] = MINI_LOCALE_MODE_C_UTF8;
        return 1;
    }

    return 0;
}

int __mini_locale_state_apply(struct mini_locale_state *state, int category,
                              const char *locale)
{
    struct mini_locale_state candidate;
    int applied;

    if (state == (struct mini_locale_state *)0 || locale == (const char *)0 ||
        !valid_category(category)) {
        return 0;
    }

    __mini_locale_state_copy(&candidate, state);
    if (category == LC_ALL) {
        applied = apply_lc_all_unchecked(&candidate, locale);
    } else {
        applied = apply_category_unchecked(&candidate, category, locale);
    }
    if (!applied) {
        return 0;
    }

    __mini_locale_state_copy(state, &candidate);
    return 1;
}

int __mini_locale_state_apply_name(struct mini_locale_state *state,
                                   int category, const char *locale)
{
    const char *selected;

    if (state == (struct mini_locale_state *)0 ||
        locale == (const char *)0 || !valid_category(category)) {
        return 0;
    }

    selected = *locale == '\0' ? environment_locale(category) : locale;
    return __mini_locale_state_apply(state, category, selected);
}

int __mini_locale_state_is_utf8(const struct mini_locale_state *state)
{
    return state != (const struct mini_locale_state *)0 &&
           state->category[LC_CTYPE] == MINI_LOCALE_MODE_C_UTF8;
}

size_t __mini_locale_state_mb_cur_max(const struct mini_locale_state *state)
{
    return __mini_locale_state_is_utf8(state) ? 4U : 1U;
}

struct mini_locale_state *__mini_locale_process_state(void)
{
    return &mini_process_locale;
}

struct mini_locale_state *__mini_locale_current_state(void)
{
    struct mini_locale_state *state;

    if (mini_locale_state_provider != (mini_locale_state_provider_t)0) {
        state = mini_locale_state_provider();
        if (state != (struct mini_locale_state *)0) {
            return state;
        }
    }
    return &mini_process_locale;
}

void __mini_locale_set_state_provider(mini_locale_state_provider_t provider)
{
    mini_locale_state_provider = provider;
}

int __mini_locale_is_utf8(void)
{
    return __mini_locale_state_is_utf8(__mini_locale_current_state());
}

size_t __mini_mb_cur_max(void)
{
    return __mini_locale_state_mb_cur_max(__mini_locale_current_state());
}

char *setlocale(int category, const char *locale)
{
    if (!valid_category(category)) {
        return (char *)0;
    }

    if (locale == (const char *)0) {
        return (char *)__mini_locale_state_query(&mini_process_locale, category);
    }

    if (!__mini_locale_state_apply_name(&mini_process_locale, category,
                                        locale)) {
        return (char *)0;
    }
    return (char *)__mini_locale_state_query(&mini_process_locale, category);
}

struct lconv *localeconv(void)
{
    return &mini_c_locale;
}
