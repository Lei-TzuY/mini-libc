#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>
#include <wchar.h>
#include <wctype.h>

#include "../src/locale/locale_internal.h"

static const char euro[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};

static int decode_euro(void)
{
    mbstate_t state = {0U, 0U};
    wchar_t wc = 0;

    errno = EIO;
    return mbrtowc(&wc, euro, 3U, &state) == 3U &&
           wc == (wchar_t)0x20ac && errno == EIO;
}

static int reject_euro(void)
{
    mbstate_t state = {0U, 0U};
    wchar_t wc = 999;

    errno = EIO;
    return mbrtowc(&wc, euro, 3U, &state) == (size_t)-1 &&
           wc == 999 && errno == EILSEQ;
}

static int inherited_override_worker(void *opaque)
{
    (void)opaque;

    if (!__mini_locale_thread_override_active() || MB_CUR_MAX != 4 ||
        !decode_euro() || !iswalpha((wint_t)0x03b1)) {
        return 11;
    }

    __mini_locale_thread_use_global();
    if (__mini_locale_thread_override_active() || MB_CUR_MAX != 1 ||
        !reject_euro() || iswalpha((wint_t)0x03b1)) {
        return 12;
    }
    return 0;
}

static int global_worker(void *opaque)
{
    int expect_utf8 = *(int *)opaque;

    if (__mini_locale_thread_override_active()) {
        return 21;
    }
    if (expect_utf8) {
        return MB_CUR_MAX == 4 && decode_euro() &&
                       iswalpha((wint_t)0x03b1)
                   ? 0
                   : 22;
    }
    return MB_CUR_MAX == 1 && reject_euro() &&
                   !iswalpha((wint_t)0x03b1)
               ? 0
               : 23;
}

int main(void)
{
    static const char marker[] = "thread-locale-ok\n";
    struct mini_locale_state utf8_state;
    thrd_t thread;
    int result;
    int expect_utf8;

    if (setlocale(LC_ALL, "C") == (char *)0 ||
        __mini_locale_thread_override_active() || MB_CUR_MAX != 1) {
        return 1;
    }

    __mini_locale_state_init(&utf8_state);
    if (!__mini_locale_state_apply(&utf8_state, LC_CTYPE, "C.UTF-8") ||
        !__mini_locale_thread_set_current(&utf8_state) ||
        !__mini_locale_thread_override_active() || MB_CUR_MAX != 4 ||
        !decode_euro() || !iswalpha((wint_t)0x03b1)) {
        return 2;
    }

    if (thrd_create(&thread, inherited_override_worker, (void *)0) !=
            thrd_success ||
        thrd_join(thread, &result) != thrd_success || result != 0 ||
        !__mini_locale_thread_override_active() || MB_CUR_MAX != 4) {
        return 3;
    }

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 || MB_CUR_MAX != 4) {
        return 4;
    }
    __mini_locale_thread_use_global();
    if (__mini_locale_thread_override_active() || MB_CUR_MAX != 4 ||
        !decode_euro()) {
        return 5;
    }

    expect_utf8 = 1;
    if (thrd_create(&thread, global_worker, &expect_utf8) != thrd_success ||
        thrd_join(thread, &result) != thrd_success || result != 0) {
        return 6;
    }

    if (setlocale(LC_CTYPE, "C") == (char *)0 || MB_CUR_MAX != 1 ||
        !reject_euro()) {
        return 7;
    }
    expect_utf8 = 0;
    if (thrd_create(&thread, global_worker, &expect_utf8) != thrd_success ||
        thrd_join(thread, &result) != thrd_success || result != 0 ||
        MB_CUR_MAX != 1) {
        return 8;
    }

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 9;
    }
    return 0;
}
