#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>
#include <wchar.h>
#include <wctype.h>

static const char euro[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};

struct worker_arg {
    locale_t utf8_locale;
    locale_t c_locale;
    int expect_global_utf8;
};

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

static int worker(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;
    locale_t previous;

    if (uselocale((locale_t)0) != LC_GLOBAL_LOCALE) {
        return 11;
    }
    if (arg->expect_global_utf8) {
        if (MB_CUR_MAX != 4 || !decode_euro() ||
            !iswalpha((wint_t)0x03b1)) {
            return 12;
        }
    } else if (MB_CUR_MAX != 1 || !reject_euro() ||
               iswalpha((wint_t)0x03b1)) {
        return 13;
    }

    previous = uselocale(arg->utf8_locale);
    if (previous != LC_GLOBAL_LOCALE ||
        uselocale((locale_t)0) != arg->utf8_locale ||
        MB_CUR_MAX != 4 || !decode_euro() ||
        !iswalpha((wint_t)0x03b1)) {
        return 14;
    }

    previous = uselocale(arg->c_locale);
    if (previous != arg->utf8_locale ||
        uselocale((locale_t)0) != arg->c_locale ||
        MB_CUR_MAX != 1 || !reject_euro() ||
        iswalpha((wint_t)0x03b1)) {
        return 15;
    }

    previous = uselocale(LC_GLOBAL_LOCALE);
    if (previous != arg->c_locale ||
        uselocale((locale_t)0) != LC_GLOBAL_LOCALE) {
        return 16;
    }
    return 0;
}

int main(void)
{
    static const char marker[] = "thread-locale-ok\n";
    locale_t c_locale;
    locale_t utf8_locale;
    locale_t utf8_copy;
    locale_t previous;
    struct worker_arg arg;
    thrd_t thread;
    int result;

    if (setlocale(LC_ALL, "C") == (char *)0 ||
        uselocale((locale_t)0) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 1) {
        return 1;
    }

    c_locale = duplocale(LC_GLOBAL_LOCALE);
    if (c_locale == (locale_t)0) {
        return 2;
    }

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 || MB_CUR_MAX != 4) {
        return 3;
    }
    utf8_locale = duplocale(LC_GLOBAL_LOCALE);
    if (utf8_locale == (locale_t)0 || utf8_locale == c_locale) {
        return 4;
    }
    utf8_copy = duplocale(utf8_locale);
    if (utf8_copy == (locale_t)0 || utf8_copy == utf8_locale) {
        return 5;
    }

    if (setlocale(LC_CTYPE, "C") == (char *)0 || MB_CUR_MAX != 1) {
        return 6;
    }
    previous = uselocale(utf8_locale);
    if (previous != LC_GLOBAL_LOCALE ||
        uselocale((locale_t)0) != utf8_locale ||
        MB_CUR_MAX != 4 || !decode_euro() ||
        !iswalpha((wint_t)0x03b1)) {
        return 7;
    }

    arg.utf8_locale = utf8_locale;
    arg.c_locale = c_locale;
    arg.expect_global_utf8 = 0;
    if (thrd_create(&thread, worker, &arg) != thrd_success ||
        thrd_join(thread, &result) != thrd_success || result != 0 ||
        uselocale((locale_t)0) != utf8_locale || MB_CUR_MAX != 4) {
        return 8;
    }

    previous = uselocale(c_locale);
    if (previous != utf8_locale || MB_CUR_MAX != 1 || !reject_euro()) {
        return 9;
    }
    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 ||
        MB_CUR_MAX != 1 || uselocale((locale_t)0) != c_locale) {
        return 10;
    }

    arg.expect_global_utf8 = 1;
    if (thrd_create(&thread, worker, &arg) != thrd_success ||
        thrd_join(thread, &result) != thrd_success || result != 0 ||
        uselocale((locale_t)0) != c_locale || MB_CUR_MAX != 1) {
        return 17;
    }

    errno = EIO;
    if (uselocale((locale_t)0x1234UL) != (locale_t)0 ||
        errno != EINVAL || uselocale((locale_t)0) != c_locale) {
        return 18;
    }

    previous = uselocale(LC_GLOBAL_LOCALE);
    if (previous != c_locale ||
        uselocale((locale_t)0) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4 || !decode_euro()) {
        return 19;
    }

    previous = uselocale(utf8_copy);
    if (previous != LC_GLOBAL_LOCALE || MB_CUR_MAX != 4) {
        return 20;
    }
    if (setlocale(LC_CTYPE, "C") == (char *)0 || MB_CUR_MAX != 4) {
        return 21;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != utf8_copy || MB_CUR_MAX != 1) {
        return 22;
    }

    freelocale(utf8_copy);
    freelocale(utf8_locale);
    freelocale(c_locale);

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 23;
    }
    return 0;
}
