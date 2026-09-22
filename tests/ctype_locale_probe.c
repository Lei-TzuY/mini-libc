#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <mini/syscall.h>
#include <stdio.h>

static int model_alpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int model_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int model_alnum(int c)
{
    return model_alpha(c) || model_digit(c);
}

static int model_blank(int c)
{
    return c == ' ' || c == '\t';
}

static int model_cntrl(int c)
{
    return (c >= 0x00 && c <= 0x1f) || c == 0x7f;
}

static int model_graph(int c)
{
    return c >= 0x21 && c <= 0x7e;
}

static int model_lower(int c)
{
    return c >= 'a' && c <= 'z';
}

static int model_print(int c)
{
    return c >= 0x20 && c <= 0x7e;
}

static int model_punct(int c)
{
    return model_graph(c) && !model_alnum(c);
}

static int model_space(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

static int model_upper(int c)
{
    return c >= 'A' && c <= 'Z';
}

static int model_xdigit(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

static int model_tolower(int c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

static int model_toupper(int c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

static int check_one(locale_t locale, int c)
{
    if ((isalnum_l(c, locale) != 0) != model_alnum(c)) return 1;
    if ((isalpha_l(c, locale) != 0) != model_alpha(c)) return 2;
    if ((isblank_l(c, locale) != 0) != model_blank(c)) return 3;
    if ((iscntrl_l(c, locale) != 0) != model_cntrl(c)) return 4;
    if ((isdigit_l(c, locale) != 0) != model_digit(c)) return 5;
    if ((isgraph_l(c, locale) != 0) != model_graph(c)) return 6;
    if ((islower_l(c, locale) != 0) != model_lower(c)) return 7;
    if ((isprint_l(c, locale) != 0) != model_print(c)) return 8;
    if ((ispunct_l(c, locale) != 0) != model_punct(c)) return 9;
    if ((isspace_l(c, locale) != 0) != model_space(c)) return 10;
    if ((isupper_l(c, locale) != 0) != model_upper(c)) return 11;
    if ((isxdigit_l(c, locale) != 0) != model_xdigit(c)) return 12;
    if (tolower_l(c, locale) != model_tolower(c)) return 13;
    if (toupper_l(c, locale) != model_toupper(c)) return 14;
    return 0;
}

int main(void)
{
    static const char ok[] = "ctype-l-ok\n";
    locale_t c_locale;
    locale_t utf8_locale;
    locale_t previous;
    int c;
    int rc;

    if (setlocale(LC_ALL, "C") == (char *)0) {
        return 1;
    }

    c_locale = newlocale(LC_CTYPE_MASK, "C", (locale_t)0);
    utf8_locale = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (c_locale == (locale_t)0 || utf8_locale == (locale_t)0) {
        return 2;
    }

    errno = ERANGE;
    rc = check_one(c_locale, EOF);
    if (rc != 0 || errno != ERANGE) {
        return 10 + rc;
    }
    rc = check_one(utf8_locale, EOF);
    if (rc != 0 || errno != ERANGE) {
        return 30 + rc;
    }

    for (c = 0; c <= 255; ++c) {
        rc = check_one(c_locale, c);
        if (rc != 0 || errno != ERANGE) {
            return 50 + rc;
        }
        rc = check_one(utf8_locale, c);
        if (rc != 0 || errno != ERANGE) {
            return 70 + rc;
        }
    }

    /* UTF-8 bytes are not standalone narrow characters. */
    if (isalpha_l(0xc3, utf8_locale) || isalpha_l(0xa9, utf8_locale) ||
        isalpha_l(0xce, utf8_locale) || isalpha_l(0xb1, utf8_locale) ||
        isprint_l(0xc3, utf8_locale) || isgraph_l(0xb1, utf8_locale) ||
        tolower_l(0xc3, utf8_locale) != 0xc3 ||
        toupper_l(0xb1, utf8_locale) != 0xb1) {
        return 90;
    }

    previous = uselocale(utf8_locale);
    if (previous != LC_GLOBAL_LOCALE ||
        !isalpha('A') || isalpha(0xce) ||
        !isalpha_l('A', c_locale) || isalpha_l(0xce, c_locale)) {
        return 91;
    }

    previous = uselocale(c_locale);
    if (previous != utf8_locale ||
        !isalpha('A') || isalpha(0xce) ||
        !isalpha_l('A', utf8_locale) || isalpha_l(0xce, utf8_locale)) {
        return 92;
    }

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 ||
        isalpha(0xce) || isalpha_l(0xce, c_locale) ||
        isalpha_l(0xce, utf8_locale)) {
        return 93;
    }

    if (uselocale(LC_GLOBAL_LOCALE) != c_locale || isalpha(0xce)) {
        return 94;
    }

    freelocale(utf8_locale);
    freelocale(c_locale);

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 95;
    }
    return 0;
}
