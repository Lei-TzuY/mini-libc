#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

static const char euro[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};

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

static int environment_mode(int argc, char **argv)
{
    static const char ok[] = "newlocale-env-ok\n";
    locale_t object;
    locale_t previous;
    int expect_utf8;

    if (argc != 3 || !same_string(argv[1], "env")) {
        return -1;
    }

    expect_utf8 = same_string(argv[2], "C.UTF-8");
    if (!expect_utf8 && !same_string(argv[2], "C")) {
        return 40;
    }

    object = newlocale(LC_CTYPE_MASK, "", (locale_t)0);
    if (object == (locale_t)0) {
        return 41;
    }
    previous = uselocale(object);
    if (previous != LC_GLOBAL_LOCALE ||
        (expect_utf8 && (MB_CUR_MAX != 4 || !decode_euro() ||
                         !iswalpha((wint_t)0x03b1))) ||
        (!expect_utf8 && (MB_CUR_MAX != 1 || !reject_euro() ||
                          iswalpha((wint_t)0x03b1)))) {
        return 42;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 43;
    }
    freelocale(object);

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 44;
    }
    return 0;
}

int main(int argc, char **argv)
{
    static const char ok[] = "newlocale-ok\n";
    locale_t object;
    locale_t result;
    locale_t previous;
    int env_result;

    env_result = environment_mode(argc, argv);
    if (env_result >= 0) {
        return env_result;
    }
    if (argc != 1) {
        return 1;
    }

    if (setlocale(LC_ALL, "C") == (char *)0 ||
        uselocale((locale_t)0) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 1) {
        return 2;
    }

    errno = EIO;
    object = newlocale(0, "not-a-locale", (locale_t)0);
    if (object == (locale_t)0 || errno != EIO) {
        return 3;
    }
    previous = uselocale(object);
    if (previous != LC_GLOBAL_LOCALE || MB_CUR_MAX != 1 || !reject_euro()) {
        return 4;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 5;
    }

    result = newlocale(LC_CTYPE_MASK, "C.UTF-8", object);
    if (result != object) {
        return 6;
    }
    if (uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4 || !decode_euro() ||
        !iswalpha((wint_t)0x03b1)) {
        return 7;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object || MB_CUR_MAX != 1) {
        return 8;
    }

    result = newlocale(LC_NUMERIC_MASK, "POSIX", object);
    if (result != object || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4 || !decode_euro()) {
        return 9;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 10;
    }

    errno = EIO;
    if (newlocale(LC_NUMERIC_MASK, "C.UTF-8", object) != (locale_t)0 ||
        errno != ENOENT || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4 || !decode_euro()) {
        return 11;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 12;
    }

    errno = EIO;
    if (newlocale(LC_ALL_MASK, "C.UTF-8", object) != (locale_t)0 ||
        errno != ENOENT || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4) {
        return 13;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 14;
    }

    errno = EIO;
    if (newlocale(1 << LC_ALL, "C", object) != (locale_t)0 ||
        errno != EINVAL || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4) {
        return 15;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 16;
    }

    errno = EIO;
    if (newlocale(LC_CTYPE_MASK, (const char *)0, object) != (locale_t)0 ||
        errno != EINVAL || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 4) {
        return 17;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 18;
    }

    errno = EIO;
    if (newlocale(LC_CTYPE_MASK, "C",
                  LC_GLOBAL_LOCALE) != (locale_t)0 ||
        errno != EINVAL) {
        return 19;
    }

    errno = EIO;
    if (newlocale(LC_CTYPE_MASK, "C",
                  (locale_t)(unsigned long)0x1234UL) != (locale_t)0 ||
        errno != EINVAL) {
        return 20;
    }

    result = newlocale(LC_ALL_MASK, "C", object);
    if (result != object || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 1 || !reject_euro() ||
        iswalpha((wint_t)0x03b1)) {
        return 21;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 22;
    }
    freelocale(object);

    object = newlocale(LC_CTYPE_MASK, "POSIX", (locale_t)0);
    if (object == (locale_t)0 || uselocale(object) != LC_GLOBAL_LOCALE ||
        MB_CUR_MAX != 1 || !reject_euro()) {
        return 23;
    }
    if (uselocale(LC_GLOBAL_LOCALE) != object) {
        return 24;
    }
    freelocale(object);

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 25;
    }
    return 0;
}
