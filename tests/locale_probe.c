#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

static int same_string(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) return 0;
        ++left;
        ++right;
    }
    return *left == *right;
}

static int check_lconv(void)
{
    struct lconv *lc = localeconv();

    if (lc == (struct lconv *)0 || !same_string(lc->decimal_point, ".") ||
        !same_string(lc->thousands_sep, "") || !same_string(lc->grouping, "") ||
        !same_string(lc->int_curr_symbol, "") || !same_string(lc->currency_symbol, "") ||
        !same_string(lc->mon_decimal_point, "") ||
        !same_string(lc->mon_thousands_sep, "") || !same_string(lc->mon_grouping, "") ||
        !same_string(lc->positive_sign, "") || !same_string(lc->negative_sign, "")) {
        return 0;
    }
    return lc->int_frac_digits == 127 && lc->frac_digits == 127 &&
           lc->p_cs_precedes == 127 && lc->p_sep_by_space == 127 &&
           lc->n_cs_precedes == 127 && lc->n_sep_by_space == 127 &&
           lc->p_sign_posn == 127 && lc->n_sign_posn == 127 &&
           lc->int_p_cs_precedes == 127 && lc->int_p_sep_by_space == 127 &&
           lc->int_n_cs_precedes == 127 && lc->int_n_sep_by_space == 127 &&
           lc->int_p_sign_posn == 127 && lc->int_n_sign_posn == 127;
}

static int environment_mode(int argc, char **argv)
{
    static const char ok[] = "locale-env-ok\n";
    char *selected;
    int category;

    if (argc == 3 &&
        (same_string(argv[1], "env-ctype") || same_string(argv[1], "env-all"))) {
        category = same_string(argv[1], "env-ctype") ? LC_CTYPE : LC_ALL;
        errno = EIO;
        selected = setlocale(category, "");
        if (selected == (char *)0 || !same_string(selected, argv[2]) ||
            !same_string(setlocale(category, (const char *)0), argv[2]) ||
            errno != EIO) {
            return 30;
        }
        if ((same_string(argv[2], "C.UTF-8") && MB_CUR_MAX != 4) ||
            (same_string(argv[2], "C") && MB_CUR_MAX != 1)) {
            return 31;
        }
        if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
            (long)(sizeof(ok) - 1U)) {
            return 32;
        }
        return 0;
    }

    if (argc == 2 && same_string(argv[1], "env-ctype-fail")) {
        errno = EIO;
        if (setlocale(LC_CTYPE, "") != (char *)0 ||
            !same_string(setlocale(LC_CTYPE, (const char *)0), "C") ||
            MB_CUR_MAX != 1 || errno != EIO) {
            return 33;
        }
        if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
            (long)(sizeof(ok) - 1U)) {
            return 34;
        }
        return 0;
    }

    if (argc == 2 && same_string(argv[1], "env-numeric-fail")) {
        errno = EIO;
        if (setlocale(LC_NUMERIC, "") != (char *)0 ||
            !same_string(setlocale(LC_NUMERIC, (const char *)0), "C") ||
            !same_string(setlocale(LC_CTYPE, (const char *)0), "C") ||
            MB_CUR_MAX != 1 || errno != EIO) {
            return 35;
        }
        if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
            (long)(sizeof(ok) - 1U)) {
            return 36;
        }
        return 0;
    }

    return -1;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "locale-ok\n";
    static const char invalid_mb[] = {'A', (char)0x80, '\0'};
    static const char euro_utf8[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};
    static const char utf8_text[] = {
        'A', (char)0xe2, (char)0x82, (char)0xac,
        (char)0xf0, (char)0x9f, (char)0x98, (char)0x80, 'B', '\0'
    };
    static const char overlong[] = {(char)0xc0, (char)0x80, '\0'};
    static const wchar_t utf8_wide[] = {'A', (wchar_t)0x20ac,
                                        (wchar_t)0x1f600, 'B', 0};
    wchar_t wide[16] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t invalid_wide[] = {'A', 0x80, 0};
    wchar_t wc = 999;
    char bytes[16] = {'?', '?', '?', '?', '?', '?', '?', '?'};

    {
        int env_result = environment_mode(argc, argv);
        if (env_result >= 0) {
            return env_result;
        }
    }
    if (argc != 1) {
        return 37;
    }
    (void)envp;

    errno = EIO;
    if (MB_CUR_MAX != 1 || setlocale(LC_ALL, (const char *)0) == (char *)0 ||
        !same_string(setlocale(LC_ALL, (const char *)0), "C") ||
        !same_string(setlocale(LC_CTYPE, "C"), "C") ||
        !same_string(setlocale(LC_NUMERIC, "C"), "C") ||
        setlocale(12345, "C") != (char *)0 ||
        setlocale(LC_ALL, "not-a-locale") != (char *)0 ||
        !same_string(setlocale(LC_ALL, (const char *)0), "C") || errno != EIO ||
        !check_lconv()) {
        return 1;
    }

    if (mblen((const char *)0, 0U) != 0 || mblen("A", 1U) != 1 ||
        mblen("", 1U) != 0 || mblen("A", 0U) != -1 || errno != EIO) {
        return 2;
    }
    if (mblen(&invalid_mb[1], 1U) != -1 || errno != EILSEQ) {
        return 3;
    }

    errno = EIO;
    if (mbtowc(&wc, "A", 1U) != 1 || wc != (wchar_t)'A' || errno != EIO ||
        mbtowc(&wc, "", 1U) != 0 || wc != 0 || errno != EIO ||
        mbtowc((wchar_t *)0, (const char *)0, 0U) != 0 || errno != EIO) {
        return 4;
    }
    wc = 999;
    if (mbtowc(&wc, &invalid_mb[1], 1U) != -1 || wc != 999 || errno != EILSEQ) {
        return 5;
    }

    errno = EIO;
    if (wctomb((char *)0, 0) != 0 || wctomb(bytes, (wchar_t)'A') != 1 ||
        bytes[0] != 'A' || wctomb(bytes, 0) != 1 || bytes[0] != '\0' || errno != EIO) {
        return 6;
    }
    if (wctomb(bytes, (wchar_t)0x80) != -1 || errno != EILSEQ) {
        return 7;
    }

    errno = EIO;
    if (mbstowcs((wchar_t *)0, "ABC", 0U) != 3U || errno != EIO ||
        mbstowcs(wide, "ABC", 8U) != 3U || wide[0] != 'A' || wide[1] != 'B' ||
        wide[2] != 'C' || wide[3] != 0 || errno != EIO) {
        return 8;
    }
    wide[0] = 9;
    wide[1] = 9;
    wide[2] = 9;
    if (mbstowcs(wide, "ABC", 2U) != 2U || wide[0] != 'A' || wide[1] != 'B' ||
        wide[2] != 9 || errno != EIO) {
        return 9;
    }
    wide[0] = 9;
    wide[1] = 9;
    if (mbstowcs(wide, invalid_mb, 8U) != (size_t)-1 || wide[0] != 'A' ||
        wide[1] != 9 || errno != EILSEQ) {
        return 10;
    }

    errno = EIO;
    if (wcstombs((char *)0, (const wchar_t[]){'A', 'B', 'C', 0}, 0U) != 3U ||
        errno != EIO ||
        wcstombs(bytes, (const wchar_t[]){'A', 'B', 'C', 0}, 8U) != 3U ||
        bytes[0] != 'A' || bytes[1] != 'B' || bytes[2] != 'C' || bytes[3] != '\0' ||
        errno != EIO) {
        return 11;
    }
    bytes[0] = '?';
    bytes[1] = '?';
    bytes[2] = '?';
    if (wcstombs(bytes, (const wchar_t[]){'A', 'B', 'C', 0}, 2U) != 2U ||
        bytes[0] != 'A' || bytes[1] != 'B' || bytes[2] != '?' || errno != EIO) {
        return 12;
    }
    bytes[0] = '?';
    bytes[1] = '?';
    if (wcstombs(bytes, invalid_wide, 8U) != (size_t)-1 || bytes[0] != 'A' ||
        bytes[1] != '?' || errno != EILSEQ) {
        return 13;
    }

    errno = EIO;
    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 ||
        !same_string(setlocale(LC_CTYPE, (const char *)0), "C.UTF-8") ||
        MB_CUR_MAX != 4 || setlocale(LC_NUMERIC, "C.UTF-8") != (char *)0 ||
        errno != EIO) {
        return 14;
    }
    if (!same_string(setlocale(LC_CTYPE, "C.utf8"), "C.UTF-8") ||
        MB_CUR_MAX != 4) {
        return 15;
    }

    errno = EIO;
    wc = 999;
    if (mbtowc(&wc, euro_utf8, 3U) != 3 || wc != (wchar_t)0x20ac ||
        errno != EIO || wctomb(bytes, (wchar_t)0x20ac) != 3 ||
        (unsigned char)bytes[0] != 0xe2U ||
        (unsigned char)bytes[1] != 0x82U ||
        (unsigned char)bytes[2] != 0xacU || errno != EIO) {
        return 16;
    }

    if (mbstowcs((wchar_t *)0, utf8_text, 0U) != 4U ||
        mbstowcs(wide, utf8_text, 16U) != 4U ||
        wide[0] != utf8_wide[0] || wide[1] != utf8_wide[1] ||
        wide[2] != utf8_wide[2] || wide[3] != utf8_wide[3] ||
        wide[4] != 0 || wcstombs((char *)0, utf8_wide, 0U) != 9U ||
        wcstombs(bytes, utf8_wide, sizeof(bytes)) != 9U ||
        bytes[9] != '\0' || errno != EIO) {
        return 17;
    }
    {
        size_t i;
        for (i = 0U; i < 10U; ++i) {
            if ((unsigned char)bytes[i] != (unsigned char)utf8_text[i]) {
                return 18;
            }
        }
    }

    if (mblen(overlong, 2U) != -1 || errno != EILSEQ) {
        return 19;
    }

    errno = EIO;
    if (!same_string(setlocale(LC_ALL, "C"), "C") || MB_CUR_MAX != 1 ||
        wctomb(bytes, (wchar_t)0x20ac) != -1 || errno != EILSEQ) {
        return 20;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) != (long)(sizeof(ok) - 1U)) {
        return 21;
    }
    return 0;
}
