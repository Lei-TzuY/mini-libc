#include <errno.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

char *mini_test_setlocale(int category, const char *locale);
struct lconv *mini_test_localeconv(void);
int mini_test_mblen(const char *s, size_t n);
int mini_test_mbtowc(wchar_t *restrict pwc, const char *restrict s, size_t n);
int mini_test_wctomb(char *s, wchar_t wc);
size_t mini_test_mbstowcs(wchar_t *restrict dst, const char *restrict src, size_t len);
size_t mini_test_wcstombs(char *restrict dst, const wchar_t *restrict src, size_t len);
int *__mini_errno_location(void);

static int same_lconv(const struct lconv *left, const struct lconv *right)
{
    return strcmp(left->decimal_point, right->decimal_point) == 0 &&
           strcmp(left->thousands_sep, right->thousands_sep) == 0 &&
           strcmp(left->grouping, right->grouping) == 0 &&
           strcmp(left->int_curr_symbol, right->int_curr_symbol) == 0 &&
           strcmp(left->currency_symbol, right->currency_symbol) == 0 &&
           strcmp(left->mon_decimal_point, right->mon_decimal_point) == 0 &&
           strcmp(left->mon_thousands_sep, right->mon_thousands_sep) == 0 &&
           strcmp(left->mon_grouping, right->mon_grouping) == 0 &&
           strcmp(left->positive_sign, right->positive_sign) == 0 &&
           strcmp(left->negative_sign, right->negative_sign) == 0 &&
           left->int_frac_digits == right->int_frac_digits &&
           left->frac_digits == right->frac_digits &&
           left->p_cs_precedes == right->p_cs_precedes &&
           left->p_sep_by_space == right->p_sep_by_space &&
           left->n_cs_precedes == right->n_cs_precedes &&
           left->n_sep_by_space == right->n_sep_by_space &&
           left->p_sign_posn == right->p_sign_posn &&
           left->n_sign_posn == right->n_sign_posn &&
           left->int_p_cs_precedes == right->int_p_cs_precedes &&
           left->int_p_sep_by_space == right->int_p_sep_by_space &&
           left->int_n_cs_precedes == right->int_n_cs_precedes &&
           left->int_n_sep_by_space == right->int_n_sep_by_space &&
           left->int_p_sign_posn == right->int_p_sign_posn &&
           left->int_n_sign_posn == right->int_n_sign_posn;
}

static int compare_single_byte(const char *s, size_t n)
{
    wchar_t host_wc = 999;
    wchar_t mini_wc = 999;
    int host_result;
    int mini_result;
    int host_errno;
    int mini_errno;

    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mblen(s, n);
    host_errno = errno;
    mini_result = mini_test_mblen(s, n);
    mini_errno = *__mini_errno_location();
    if (host_result != mini_result || host_errno != mini_errno) return 0;

    errno = 123;
    *__mini_errno_location() = 123;
    host_result = mbtowc(&host_wc, s, n);
    host_errno = errno;
    mini_result = mini_test_mbtowc(&mini_wc, s, n);
    mini_errno = *__mini_errno_location();
    return host_result == mini_result && host_errno == mini_errno &&
           host_wc == mini_wc;
}

int main(void)
{
    static const char high[] = {(char)0x80, '\0'};
    static const wchar_t valid_wide[] = {'A', 'B', 'C', 0};
    static const wchar_t invalid_wide[] = {'A', 0x80, 0};
    wchar_t host_wide[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    wchar_t mini_wide[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    char host_bytes[8] = {'?', '?', '?', '?', '?', '?', '?', '?'};
    char mini_bytes[8] = {'?', '?', '?', '?', '?', '?', '?', '?'};
    size_t host_count;
    size_t mini_count;

    if (setlocale(LC_ALL, "C") == NULL ||
        mini_test_setlocale(LC_ALL, "C") == NULL ||
        strcmp(mini_test_setlocale(LC_ALL, NULL), "C") != 0 ||
        !same_lconv(localeconv(), mini_test_localeconv())) {
        return 1;
    }

    if (!compare_single_byte("A", 1U) || !compare_single_byte("", 1U) ||
        !compare_single_byte("A", 0U) || !compare_single_byte(high, 1U)) {
        return 2;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    if (wctomb(host_bytes, (wchar_t)'A') != mini_test_wctomb(mini_bytes, (wchar_t)'A') ||
        host_bytes[0] != mini_bytes[0] || errno != *__mini_errno_location()) {
        return 3;
    }
    errno = 123;
    *__mini_errno_location() = 123;
    if (wctomb(host_bytes, (wchar_t)0x80) != mini_test_wctomb(mini_bytes, (wchar_t)0x80) ||
        errno != *__mini_errno_location()) {
        return 4;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    host_count = mbstowcs(host_wide, "ABC", 8U);
    mini_count = mini_test_mbstowcs(mini_wide, "ABC", 8U);
    if (host_count != mini_count || host_count != 3U ||
        memcmp(host_wide, mini_wide, 4U * sizeof(wchar_t)) != 0 ||
        errno != *__mini_errno_location()) {
        return 5;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    host_count = wcstombs(host_bytes, valid_wide, 8U);
    mini_count = mini_test_wcstombs(mini_bytes, valid_wide, 8U);
    if (host_count != mini_count || host_count != 3U ||
        memcmp(host_bytes, mini_bytes, 4U) != 0 || errno != *__mini_errno_location()) {
        return 6;
    }

    errno = 123;
    *__mini_errno_location() = 123;
    host_count = wcstombs(host_bytes, invalid_wide, 8U);
    mini_count = mini_test_wcstombs(mini_bytes, invalid_wide, 8U);
    if (host_count != mini_count || host_count != (size_t)-1 ||
        errno != *__mini_errno_location()) {
        return 7;
    }

    puts("locale-differential-ok");
    return 0;
}
