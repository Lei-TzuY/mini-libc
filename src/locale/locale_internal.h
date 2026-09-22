#ifndef MINI_LIBC_LOCALE_INTERNAL_H
#define MINI_LIBC_LOCALE_INTERNAL_H

#include <locale.h>
#include <stddef.h>

#define MINI_LOCALE_CATEGORY_COUNT 5
#define MINI_LOCALE_MODE_C 0U
#define MINI_LOCALE_MODE_C_UTF8 1U

struct mini_locale_state {
    unsigned char category[MINI_LOCALE_CATEGORY_COUNT];
};

void __mini_locale_state_init(struct mini_locale_state *state);
void __mini_locale_state_copy(struct mini_locale_state *dst,
                              const struct mini_locale_state *src);
const char *__mini_locale_state_query(const struct mini_locale_state *state,
                                      int category);
int __mini_locale_state_apply(struct mini_locale_state *state, int category,
                              const char *locale);
int __mini_locale_state_is_utf8(const struct mini_locale_state *state);
size_t __mini_locale_state_mb_cur_max(const struct mini_locale_state *state);

int __mini_locale_is_utf8(void);

#endif
