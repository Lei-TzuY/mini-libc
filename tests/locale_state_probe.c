#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

#include "../src/locale/locale_internal.h"
#include "../src/wchar/wchar_internal.h"

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

int main(void)
{
    static const char ok[] = "locale-state-ok\n";
    static const char mixed[] =
        "LC_CTYPE=C.UTF-8;LC_NUMERIC=C;LC_TIME=C;LC_COLLATE=C;LC_MONETARY=C";
    static const char euro[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};
    struct mini_locale_state c_state;
    struct mini_locale_state utf8_state;
    struct mini_locale_state saved_state;
    mbstate_t conversion_state = {0U, 0U};
    wchar_t wc = 999;
    char bytes[4] = {'?', '?', '?', '?'};

    if (setlocale(LC_ALL, "C") == (char *)0 || MB_CUR_MAX != 1) {
        return 1;
    }

    __mini_locale_state_init(&c_state);
    __mini_locale_state_init(&utf8_state);
    if (!same_string(__mini_locale_state_query(&c_state, LC_ALL), "C") ||
        __mini_locale_state_mb_cur_max(&c_state) != 1U ||
        __mini_locale_state_is_utf8(&c_state)) {
        return 2;
    }

    if (!__mini_locale_state_apply(&utf8_state, LC_CTYPE, "C.UTF-8") ||
        !same_string(__mini_locale_state_query(&utf8_state, LC_CTYPE),
                     "C.UTF-8") ||
        !same_string(__mini_locale_state_query(&utf8_state, LC_NUMERIC), "C") ||
        !same_string(__mini_locale_state_query(&utf8_state, LC_ALL), mixed) ||
        !__mini_locale_state_is_utf8(&utf8_state) ||
        __mini_locale_state_mb_cur_max(&utf8_state) != 4U) {
        return 3;
    }

    if (__mini_locale_state_apply(&utf8_state, LC_NUMERIC, "C.UTF-8") ||
        !same_string(__mini_locale_state_query(&utf8_state, LC_ALL), mixed)) {
        return 4;
    }

    __mini_locale_state_copy(&saved_state, &utf8_state);
    if (!__mini_locale_state_apply(&utf8_state, LC_ALL, "C") ||
        !same_string(__mini_locale_state_query(&utf8_state, LC_ALL), "C") ||
        !same_string(__mini_locale_state_query(&saved_state, LC_ALL), mixed)) {
        return 5;
    }

    if (__mini_mbrtowc_mode(&wc, euro, 3U, &conversion_state,
                            __mini_locale_state_is_utf8(&saved_state)) != 3U ||
        wc != (wchar_t)0x20ac) {
        return 6;
    }
    conversion_state.__count = 0U;
    conversion_state.__value = 0U;
    if (__mini_wcrtomb_mode(bytes, wc, &conversion_state,
                            __mini_locale_state_is_utf8(&saved_state)) != 3U ||
        (unsigned char)bytes[0] != 0xe2U ||
        (unsigned char)bytes[1] != 0x82U ||
        (unsigned char)bytes[2] != 0xacU) {
        return 7;
    }

    errno = EIO;
    wc = 999;
    if (mbtowc(&wc, euro, 3U) != -1 || wc != 999 || errno != EILSEQ ||
        MB_CUR_MAX != 1 ||
        !same_string(setlocale(LC_ALL, (const char *)0), "C")) {
        return 8;
    }

    if (!__mini_locale_state_apply(&utf8_state, LC_ALL, mixed) ||
        !same_string(__mini_locale_state_query(&utf8_state, LC_ALL), mixed) ||
        !__mini_locale_state_is_utf8(&utf8_state)) {
        return 9;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 10;
    }
    return 0;
}
