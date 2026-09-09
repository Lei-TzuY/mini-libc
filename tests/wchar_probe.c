#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <wchar.h>

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "wchar-ok\n";
    static const char invalid_mb[] = {'A', (char)0x80, '\0'};
    static const wchar_t wide_abc[] = {'A', 'B', 'C', 0};
    static const wchar_t invalid_wide[] = {'A', 0x80, 0};
    mbstate_t state = {0U, 0U};
    wchar_t wc = 999;
    wchar_t wide[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t copy[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    char bytes[8] = {'?', '?', '?', '?', '?', '?', '?', '?'};
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

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) != (long)(sizeof(ok) - 1U)) {
        return 19;
    }
    return 0;
}
