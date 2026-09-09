#include <errno.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

int mini_test_mbsinit(const mbstate_t *ps);
size_t mini_test_mbrtowc(wchar_t *restrict pwc, const char *restrict s, size_t n,
                         mbstate_t *restrict ps);
size_t mini_test_wcrtomb(char *restrict s, wchar_t wc, mbstate_t *restrict ps);
size_t mini_test_mbsrtowcs(wchar_t *restrict dst, const char **restrict src,
                           size_t len, mbstate_t *restrict ps);
size_t mini_test_wcsrtombs(char *restrict dst, const wchar_t **restrict src,
                           size_t len, mbstate_t *restrict ps);
size_t mini_test_wcslen(const wchar_t *s);
int mini_test_wcscmp(const wchar_t *left, const wchar_t *right);
wchar_t *mini_test_wcscpy(wchar_t *restrict dst, const wchar_t *restrict src);
int *__mini_errno_location(void);

static mbstate_t *mini_state(void *storage)
{
    return (mbstate_t *)storage;
}

static int same_sign(int left, int right)
{
    return (left < 0 && right < 0) || (left == 0 && right == 0) ||
           (left > 0 && right > 0);
}

int main(void)
{
    static const char high[] = {(char)0x80, '\0'};
    static const wchar_t wide_abc[] = {'A', 'B', 'C', 0};
    static const wchar_t invalid_wide[] = {'A', 0x80, 0};
    mbstate_t host_state = {0};
    unsigned long mini_storage[2] = {0UL, 0UL};
    wchar_t host_wc = 777;
    wchar_t mini_wc = 777;
    wchar_t host_wide[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t mini_wide[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t host_copy[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t mini_copy[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    char host_bytes[8] = {'?', '?', '?', '?', '?', '?', '?', '?'};
    char mini_bytes[8] = {'?', '?', '?', '?', '?', '?', '?', '?'};
    const char *host_src;
    const char *mini_src;
    const wchar_t *host_wsrc;
    const wchar_t *mini_wsrc;
    size_t host_result;
    size_t mini_result;

    if (setlocale(LC_ALL, "C") == NULL || !mbsinit(&host_state) ||
        !mini_test_mbsinit(mini_state(mini_storage))) {
        return 1;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbrtowc(&host_wc, "A", 1U, &host_state);
    mini_result = mini_test_mbrtowc(&mini_wc, "A", 1U, mini_state(mini_storage));
    if (host_result != mini_result || host_wc != mini_wc ||
        errno != *__mini_errno_location()) {
        return 2;
    }

    host_wc = 777;
    mini_wc = 777;
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbrtowc(&host_wc, "A", 0U, &host_state);
    mini_result = mini_test_mbrtowc(&mini_wc, "A", 0U, mini_state(mini_storage));
    if (host_result != mini_result || host_wc != mini_wc ||
        errno != *__mini_errno_location()) {
        return 3;
    }

    host_wc = 777;
    mini_wc = 777;
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbrtowc(&host_wc, high, 1U, &host_state);
    mini_result = mini_test_mbrtowc(&mini_wc, high, 1U, mini_state(mini_storage));
    if (host_result != mini_result || host_wc != mini_wc ||
        errno != *__mini_errno_location()) {
        return 4;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    host_result = wcrtomb(host_bytes, (wchar_t)'A', &host_state);
    mini_result = mini_test_wcrtomb(mini_bytes, (wchar_t)'A', mini_state(mini_storage));
    if (host_result != mini_result || host_bytes[0] != mini_bytes[0] ||
        errno != *__mini_errno_location()) {
        return 5;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    host_result = wcrtomb((char *)0, (wchar_t)0x80, &host_state);
    mini_result = mini_test_wcrtomb((char *)0, (wchar_t)0x80,
                                    mini_state(mini_storage));
    if (host_result != mini_result || errno != *__mini_errno_location()) {
        return 6;
    }

    host_src = "ABC";
    mini_src = "ABC";
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbsrtowcs(host_wide, &host_src, 2U, &host_state);
    mini_result = mini_test_mbsrtowcs(mini_wide, &mini_src, 2U,
                                      mini_state(mini_storage));
    if (host_result != mini_result || host_result != 2U ||
        memcmp(host_wide, mini_wide, 2U * sizeof(wchar_t)) != 0 ||
        host_src == NULL || mini_src == NULL || *host_src != *mini_src ||
        errno != *__mini_errno_location()) {
        return 7;
    }

    host_src = "ABC";
    mini_src = "ABC";
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbsrtowcs((wchar_t *)0, &host_src, 0U, &host_state);
    mini_result = mini_test_mbsrtowcs((wchar_t *)0, &mini_src, 0U,
                                      mini_state(mini_storage));
    if (host_result != mini_result || host_result != 3U ||
        strcmp(host_src, mini_src) != 0 || errno != *__mini_errno_location()) {
        return 8;
    }

    host_src = high;
    mini_src = high;
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbsrtowcs(host_wide, &host_src, 8U, &host_state);
    mini_result = mini_test_mbsrtowcs(mini_wide, &mini_src, 8U,
                                      mini_state(mini_storage));
    if (host_result != mini_result || host_result != (size_t)-1 ||
        errno != *__mini_errno_location()) {
        return 9;
    }

    host_wsrc = wide_abc;
    mini_wsrc = wide_abc;
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = wcsrtombs(host_bytes, &host_wsrc, 2U, &host_state);
    mini_result = mini_test_wcsrtombs(mini_bytes, &mini_wsrc, 2U,
                                      mini_state(mini_storage));
    if (host_result != mini_result || host_result != 2U ||
        memcmp(host_bytes, mini_bytes, 2U) != 0 ||
        host_wsrc == NULL || mini_wsrc == NULL || *host_wsrc != *mini_wsrc ||
        errno != *__mini_errno_location()) {
        return 10;
    }

    host_wsrc = invalid_wide;
    mini_wsrc = invalid_wide;
    errno = 123;
    *__mini_errno_location() = 123;
    host_result = wcsrtombs(host_bytes, &host_wsrc, 8U, &host_state);
    mini_result = mini_test_wcsrtombs(mini_bytes, &mini_wsrc, 8U,
                                      mini_state(mini_storage));
    if (host_result != mini_result || host_result != (size_t)-1 ||
        errno != *__mini_errno_location()) {
        return 11;
    }

    if (wcslen(wide_abc) != mini_test_wcslen(wide_abc) ||
        !same_sign(wcscmp(wide_abc, (const wchar_t[]){'A', 'C', 0}),
                   mini_test_wcscmp(wide_abc, (const wchar_t[]){'A', 'C', 0})) ||
        mini_test_wcscpy(mini_copy, wide_abc) != mini_copy ||
        wcscpy(host_copy, wide_abc) != host_copy ||
        memcmp(host_copy, mini_copy, 4U * sizeof(wchar_t)) != 0) {
        return 12;
    }

    puts("wchar-differential-ok");
    return 0;
}
