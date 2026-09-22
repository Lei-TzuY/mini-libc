#include <locale.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

#include "../src/locale/locale_internal.h"
#include "../src/wchar/wchar_internal.h"

int main(void)
{
    static const char mixed[] =
        "LC_CTYPE=C.UTF-8;LC_NUMERIC=C;LC_TIME=C;LC_COLLATE=C;LC_MONETARY=C";
    static const char euro[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};
    struct mini_locale_state process_shadow;
    struct mini_locale_state isolated;
    mbstate_t state = {0U, 0U};
    wchar_t wc = 0;

    if (setlocale(LC_ALL, "C") == (char *)0 || MB_CUR_MAX != 1) {
        return 1;
    }

    __mini_locale_state_init(&process_shadow);
    __mini_locale_state_init(&isolated);
    if (!__mini_locale_state_apply(&isolated, LC_CTYPE, "C.UTF-8") ||
        __mini_locale_state_mb_cur_max(&isolated) != 4U ||
        __mini_locale_state_mb_cur_max(&process_shadow) != 1U) {
        return 2;
    }
    if (__mini_mbrtowc_mode(&wc, euro, 3U, &state,
                            __mini_locale_state_is_utf8(&isolated)) != 3U ||
        wc != (wchar_t)0x20ac || MB_CUR_MAX != 1) {
        return 3;
    }

    __mini_locale_state_copy(&process_shadow, &isolated);
    if (__mini_locale_state_query(&process_shadow, LC_ALL) == (const char *)0 ||
        !__mini_locale_state_is_utf8(&process_shadow)) {
        return 4;
    }
    {
        const char *aggregate =
            __mini_locale_state_query(&process_shadow, LC_ALL);
        const char *expected = mixed;

        while (*aggregate != '\0' && *aggregate == *expected) {
            ++aggregate;
            ++expected;
        }
        if (*aggregate != *expected) {
            return 5;
        }
    }

    if (setlocale(LC_ALL, (const char *)0) == (char *)0 || MB_CUR_MAX != 1) {
        return 6;
    }

    if (mini_sys_write(1, "tiny-locale-state-ok\n", 21U) != 21L) {
        return 7;
    }
    return 0;
}
