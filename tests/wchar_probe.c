#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <wchar.h>
#include <wctype.h>

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "wchar-ok\n";
    static const char invalid_mb[] = {'A', (char)0x80, '\0'};
    static const wchar_t wide_abc[] = {'A', 'B', 'C', 0};
    static const wchar_t invalid_wide[] = {'A', 0x80, 0};
    static const char euro_utf8[] = {(char)0xe2, (char)0x82, (char)0xac, '\0'};
    static const char emoji_utf8[] = {
        (char)0xf0, (char)0x9f, (char)0x98, (char)0x80, '\0'
    };
    static const char surrogate_utf8[] = {
        (char)0xed, (char)0xa0, (char)0x80, '\0'
    };
    static const char too_high_utf8[] = {
        (char)0xf4, (char)0x90, (char)0x80, (char)0x80, '\0'
    };
    static const char overlong_utf8[] = {(char)0xc0, (char)0x80, '\0'};
    static const char utf8_text[] = {
        'A', (char)0xe2, (char)0x82, (char)0xac,
        (char)0xf0, (char)0x9f, (char)0x98, (char)0x80, 'B', '\0'
    };
    static const wchar_t utf8_wide[] = {
        'A', (wchar_t)0x20ac, (wchar_t)0x1f600, 'B', 0
    };
    mbstate_t state = {0U, 0U};
    wchar_t wc = 999;
    wchar_t wide[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t copy[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    char bytes[10] = {'?', '?', '?', '?', '?', '?', '?', '?', '?', '?'};
    const char *mbsrc;
    const wchar_t *wcsrc;
    size_t result;

    (void)argc;
    (void)argv;
    (void)envp;

    if (!mbsinit((const mbstate_t *)0) || !mbsinit(&state)) {
        return 1;
    }

    errno = EIO;
    result = mbrtowc(&wc, "A", 1U, &state);
    if (result != 1U || wc != (wchar_t)'A' || errno != EIO || !mbsinit(&state)) {
        return 2;
    }
    result = mbrtowc(&wc, "", 1U, &state);
    if (result != 0U || wc != 0 || errno != EIO || !mbsinit(&state)) {
        return 3;
    }
    wc = 777;
    result = mbrtowc(&wc, "A", 0U, &state);
    if (result != (size_t)-2 || wc != 777 || errno != EIO || !mbsinit(&state)) {
        return 4;
    }
    wc = 777;
    result = mbrtowc(&wc, (const char *)0, 0U, &state);
    if (result != 0U || wc != 777 || errno != EIO || !mbsinit(&state)) {
        return 5;
    }
    wc = 777;
    result = mbrtowc(&wc, &invalid_mb[1], 1U, &state);
    if (result != (size_t)-1 || wc != 777 || errno != EILSEQ || !mbsinit(&state)) {
        return 6;
    }

    errno = EIO;
    result = wcrtomb(bytes, (wchar_t)'A', &state);
    if (result != 1U || bytes[0] != 'A' || errno != EIO || !mbsinit(&state)) {
        return 7;
    }
    result = wcrtomb((char *)0, (wchar_t)0x80, &state);
    if (result != 1U || errno != EIO || !mbsinit(&state)) {
        return 8;
    }
    result = wcrtomb(bytes, (wchar_t)0x80, &state);
    if (result != (size_t)-1 || errno != EILSEQ || !mbsinit(&state)) {
        return 9;
    }

    errno = EIO;
    mbsrc = "ABC";
    result = mbsrtowcs((wchar_t *)0, &mbsrc, 0U, &state);
    if (result != 3U || mbsrc[0] != 'A' || errno != EIO || !mbsinit(&state)) {
        return 10;
    }
    mbsrc = "ABC";
    result = mbsrtowcs(wide, &mbsrc, 2U, &state);
    if (result != 2U || wide[0] != 'A' || wide[1] != 'B' ||
        mbsrc == (const char *)0 || *mbsrc != 'C' || errno != EIO) {
        return 11;
    }
    result = mbsrtowcs(&wide[2], &mbsrc, 6U, &state);
    if (result != 1U || wide[2] != 'C' || wide[3] != 0 ||
        mbsrc != (const char *)0 || errno != EIO) {
        return 12;
    }
    mbsrc = invalid_mb;
    wide[0] = 9;
    wide[1] = 9;
    result = mbsrtowcs(wide, &mbsrc, 8U, &state);
    if (result != (size_t)-1 || wide[0] != 'A' || wide[1] != 9 ||
        mbsrc == (const char *)0 || (unsigned char)*mbsrc != 0x80U ||
        errno != EILSEQ || !mbsinit(&state)) {
        return 13;
    }

    errno = EIO;
    wcsrc = wide_abc;
    result = wcsrtombs((char *)0, &wcsrc, 0U, &state);
    if (result != 3U || wcsrc != wide_abc || errno != EIO || !mbsinit(&state)) {
        return 14;
    }
    wcsrc = wide_abc;
    result = wcsrtombs(bytes, &wcsrc, 2U, &state);
    if (result != 2U || bytes[0] != 'A' || bytes[1] != 'B' ||
        wcsrc == (const wchar_t *)0 || *wcsrc != (wchar_t)'C' || errno != EIO) {
        return 15;
    }
    result = wcsrtombs(&bytes[2], &wcsrc, 6U, &state);
    if (result != 1U || bytes[2] != 'C' || bytes[3] != '\0' ||
        wcsrc != (const wchar_t *)0 || errno != EIO) {
        return 16;
    }
    wcsrc = invalid_wide;
    bytes[0] = '?';
    bytes[1] = '?';
    result = wcsrtombs(bytes, &wcsrc, 8U, &state);
    if (result != (size_t)-1 || bytes[0] != 'A' || bytes[1] != '?' ||
        wcsrc == (const wchar_t *)0 || *wcsrc != (wchar_t)0x80 ||
        errno != EILSEQ || !mbsinit(&state)) {
        return 17;
    }

    if (wcslen(wide_abc) != 3U || wcscmp(wide_abc, wide_abc) != 0 ||
        wcscmp((const wchar_t[]){'A', 'A', 0}, wide_abc) >= 0 ||
        wcscmp(wide_abc, (const wchar_t[]){'A', 'A', 0}) <= 0 ||
        wcscpy(copy, wide_abc) != copy || wcslen(copy) != 3U ||
        wcscmp(copy, wide_abc) != 0) {
        return 18;
    }

    {
        wctype_t alpha = wctype("alpha");
        wctrans_t lower = wctrans("tolower");

        if (!iswalpha((wint_t)'A') || !iswdigit((wint_t)'7') ||
            !iswspace((wint_t)'\n') || !iswpunct((wint_t)'!') ||
            towlower((wint_t)'Q') != (wint_t)'q' ||
            towupper((wint_t)'q') != (wint_t)'Q' ||
            alpha == 0UL || !iswctype((wint_t)'Z', alpha) ||
            lower == 0UL || towctrans((wint_t)'R', lower) != (wint_t)'r' ||
            wctype("not-a-class") != 0UL || wctrans("not-a-map") != 0UL ||
            iswalpha((wint_t)0x03b1) || iswprint((wint_t)0x20ac)) {
            return 36;
        }
    }

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0) {
        return 19;
    }

    state.__count = 0U;
    state.__value = 0U;
    errno = EIO;
    wc = 777;
    result = mbrtowc(&wc, euro_utf8, 1U, &state);
    if (result != (size_t)-2 || wc != 777 || mbsinit(&state) || errno != EIO) {
        return 20;
    }
    result = mbrtowc(&wc, euro_utf8 + 1, 0U, &state);
    if (result != (size_t)-2 || mbsinit(&state) || errno != EIO) {
        return 21;
    }
    result = mbrtowc(&wc, euro_utf8 + 1, 1U, &state);
    if (result != (size_t)-2 || mbsinit(&state) || errno != EIO) {
        return 22;
    }
    result = mbrtowc(&wc, euro_utf8 + 2, 1U, &state);
    if (result != 1U || wc != (wchar_t)0x20ac || !mbsinit(&state) ||
        errno != EIO) {
        return 23;
    }

    result = mbrtowc(&wc, emoji_utf8, 4U, &state);
    if (result != 4U || wc != (wchar_t)0x1f600 || !mbsinit(&state) ||
        errno != EIO) {
        return 24;
    }

    errno = EIO;
    if (mbrtowc(&wc, overlong_utf8, 2U, &state) != (size_t)-1 ||
        errno != EILSEQ || !mbsinit(&state)) {
        return 25;
    }
    errno = EIO;
    if (mbrtowc(&wc, surrogate_utf8, 3U, &state) != (size_t)-1 ||
        errno != EILSEQ || !mbsinit(&state)) {
        return 26;
    }
    errno = EIO;
    if (mbrtowc(&wc, too_high_utf8, 4U, &state) != (size_t)-1 ||
        errno != EILSEQ || !mbsinit(&state)) {
        return 27;
    }

    errno = EIO;
    result = wcrtomb(bytes, (wchar_t)0x20ac, &state);
    if (result != 3U || (unsigned char)bytes[0] != 0xe2U ||
        (unsigned char)bytes[1] != 0x82U || (unsigned char)bytes[2] != 0xacU ||
        errno != EIO || !mbsinit(&state)) {
        return 28;
    }
    result = wcrtomb(bytes, (wchar_t)0x1f600, &state);
    if (result != 4U || (unsigned char)bytes[0] != 0xf0U ||
        (unsigned char)bytes[1] != 0x9fU || (unsigned char)bytes[2] != 0x98U ||
        (unsigned char)bytes[3] != 0x80U || errno != EIO) {
        return 29;
    }
    if (wcrtomb(bytes, (wchar_t)0xd800, &state) != (size_t)-1 ||
        errno != EILSEQ) {
        return 30;
    }

    errno = EIO;
    mbsrc = utf8_text;
    result = mbsrtowcs(wide, &mbsrc, 8U, &state);
    if (result != 4U || mbsrc != (const char *)0 ||
        wide[0] != utf8_wide[0] || wide[1] != utf8_wide[1] ||
        wide[2] != utf8_wide[2] || wide[3] != utf8_wide[3] ||
        wide[4] != 0 || errno != EIO) {
        return 31;
    }
    wcsrc = utf8_wide;
    result = wcsrtombs(bytes, &wcsrc, sizeof(bytes), &state);
    if (result != 9U || wcsrc != (const wchar_t *)0 || bytes[9] != '\0' ||
        errno != EIO) {
        return 32;
    }
    {
        size_t i;
        for (i = 0U; i < 10U; ++i) {
            if ((unsigned char)bytes[i] != (unsigned char)utf8_text[i]) {
                return 33;
            }
        }
    }

    {
        wctype_t alpha = wctype("alpha");
        wctrans_t upper = wctrans("toupper");

        if (!iswalpha((wint_t)0x00e9) || !iswalpha((wint_t)0x03b1) ||
            !iswalpha((wint_t)0x0416) || !iswalpha((wint_t)0x4e2d) ||
            !iswupper((wint_t)0x03a3) || !iswlower((wint_t)0x03c2) ||
            towupper((wint_t)0x03c2) != (wint_t)0x03a3 ||
            towlower((wint_t)0x0416) != (wint_t)0x0436 ||
            !iswspace((wint_t)0x3000) || !iswblank((wint_t)0x00a0) ||
            !iswprint((wint_t)0x1f600) || !iswgraph((wint_t)0x1f600) ||
            !iswpunct((wint_t)0x20ac) || alpha == 0UL ||
            !iswctype((wint_t)0x4e2d, alpha) || upper == 0UL ||
            towctrans((wint_t)0x03c2, upper) != (wint_t)0x03a3 ||
            iswalpha(WEOF) || iswprint(WEOF)) {
            return 37;
        }
    }

    if (setlocale(LC_CTYPE, "C") == (char *)0) {
        return 34;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) != (long)(sizeof(ok) - 1U)) {
        return 35;
    }
    return 0;
}
