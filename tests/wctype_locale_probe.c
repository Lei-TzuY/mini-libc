#include <locale.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

static int expect_utf8_properties(locale_t locale)
{
    wctype_t alpha = wctype_l("alpha", locale);
    wctype_t digit = wctype_l("digit", locale);
    wctype_t punct = wctype_l("punct", locale);
    wctrans_t upper = wctrans_l("toupper", locale);
    wctrans_t lower = wctrans_l("tolower", locale);

    if (alpha == (wctype_t)0 || digit == (wctype_t)0 ||
        punct == (wctype_t)0 || upper == (wctrans_t)0 ||
        lower == (wctrans_t)0) {
        return 0;
    }

    return iswalpha_l((wint_t)0x03b1, locale) &&
           iswalnum_l((wint_t)0x03b1, locale) &&
           iswlower_l((wint_t)0x03b1, locale) &&
           !iswupper_l((wint_t)0x03b1, locale) &&
           iswgraph_l((wint_t)0x03b1, locale) &&
           iswprint_l((wint_t)0x03b1, locale) &&
           !iswdigit_l((wint_t)0x03b1, locale) &&
           !iswblank_l((wint_t)0x03b1, locale) &&
           !iswcntrl_l((wint_t)0x03b1, locale) &&
           !iswpunct_l((wint_t)0x03b1, locale) &&
           !iswspace_l((wint_t)0x03b1, locale) &&
           !iswxdigit_l((wint_t)0x03b1, locale) &&
           iswctype_l((wint_t)0x03b1, alpha, locale) &&
           iswctype_l((wint_t)0x0660, digit, locale) &&
           iswctype_l((wint_t)0x2014, punct, locale) &&
           towupper_l((wint_t)0x03b1, locale) == (wint_t)0x0391 &&
           towlower_l((wint_t)0x0391, locale) == (wint_t)0x03b1 &&
           towctrans_l((wint_t)0x03b1, upper, locale) == (wint_t)0x0391 &&
           towctrans_l((wint_t)0x0391, lower, locale) == (wint_t)0x03b1;
}

static int expect_c_properties(locale_t locale)
{
    wctype_t alpha = wctype_l("alpha", locale);
    wctrans_t upper = wctrans_l("toupper", locale);

    if (alpha == (wctype_t)0 || upper == (wctrans_t)0) {
        return 0;
    }

    return !iswalpha_l((wint_t)0x03b1, locale) &&
           !iswalnum_l((wint_t)0x03b1, locale) &&
           !iswgraph_l((wint_t)0x03b1, locale) &&
           !iswprint_l((wint_t)0x03b1, locale) &&
           !iswctype_l((wint_t)0x03b1, alpha, locale) &&
           towupper_l((wint_t)0x03b1, locale) == (wint_t)0x03b1 &&
           towlower_l((wint_t)0x0391, locale) == (wint_t)0x0391 &&
           towctrans_l((wint_t)0x03b1, upper, locale) == (wint_t)0x03b1 &&
           iswalpha_l((wint_t)'A', locale) &&
           iswupper_l((wint_t)'A', locale) &&
           towlower_l((wint_t)'A', locale) == (wint_t)'a' &&
           iswxdigit_l((wint_t)'F', locale);
}

int main(void)
{
    static const char marker[] = "wctype-l-ok\n";
    locale_t c_locale;
    locale_t utf8_locale;
    locale_t previous;

    if (setlocale(LC_ALL, "C") == (char *)0 ||
        iswalpha((wint_t)0x03b1)) {
        return 1;
    }

    c_locale = newlocale(LC_CTYPE_MASK, "C", (locale_t)0);
    utf8_locale = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (c_locale == (locale_t)0 || utf8_locale == (locale_t)0) {
        return 2;
    }

    if (!expect_c_properties(c_locale) ||
        !expect_utf8_properties(utf8_locale) ||
        wctype_l("not-a-class", c_locale) != (wctype_t)0 ||
        wctrans_l("not-a-transform", utf8_locale) != (wctrans_t)0) {
        return 3;
    }

    previous = uselocale(utf8_locale);
    if (previous != LC_GLOBAL_LOCALE ||
        !iswalpha((wint_t)0x03b1) ||
        !expect_c_properties(c_locale)) {
        return 4;
    }

    previous = uselocale(c_locale);
    if (previous != utf8_locale ||
        iswalpha((wint_t)0x03b1) ||
        !expect_utf8_properties(utf8_locale)) {
        return 5;
    }

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 ||
        iswalpha((wint_t)0x03b1) ||
        !expect_utf8_properties(utf8_locale) ||
        !expect_c_properties(c_locale)) {
        return 6;
    }

    if (uselocale(LC_GLOBAL_LOCALE) != c_locale ||
        !iswalpha((wint_t)0x03b1) ||
        !expect_c_properties(c_locale)) {
        return 7;
    }

    if (setlocale(LC_CTYPE, "C") == (char *)0 ||
        iswalpha((wint_t)0x03b1) ||
        !expect_utf8_properties(utf8_locale)) {
        return 8;
    }

    freelocale(utf8_locale);
    freelocale(c_locale);

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 9;
    }
    return 0;
}
