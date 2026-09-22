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

typedef struct mini_locale_state *(*mini_locale_state_provider_t)(void);

void __mini_locale_state_init(struct mini_locale_state *state);
void __mini_locale_state_copy(struct mini_locale_state *dst,
                              const struct mini_locale_state *src);
const char *__mini_locale_state_query(const struct mini_locale_state *state,
                                      int category);
int __mini_locale_state_apply(struct mini_locale_state *state, int category,
                              const char *locale);
int __mini_locale_state_apply_name(struct mini_locale_state *state,
                                   int category, const char *locale);
int __mini_locale_state_is_utf8(const struct mini_locale_state *state);
size_t __mini_locale_state_mb_cur_max(const struct mini_locale_state *state);
struct mini_locale_state *__mini_locale_process_state(void);
struct mini_locale_state *__mini_locale_current_state(void);
void __mini_locale_set_state_provider(mini_locale_state_provider_t provider);

int __mini_locale_thread_install(locale_t handle,
                                 const struct mini_locale_state *state);
locale_t __mini_locale_thread_current_handle(void);
void __mini_locale_thread_use_global(void);
int __mini_locale_thread_override_active(void);

int __mini_locale_is_utf8(void);

#endif
