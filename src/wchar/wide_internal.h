#ifndef MINI_LIBC_WIDE_INTERNAL_H
#define MINI_LIBC_WIDE_INTERNAL_H

#include <stdio.h>
#include <wchar.h>

wint_t __mini_wide_read_character_unlocked(FILE *stream);
wint_t __mini_wide_unget_character_unlocked(wint_t wc, FILE *stream);

#endif
