#include <wchar.h>
#include <wctype.h>

#include "../locale/locale_internal.h"

#define MINI_WCTYPE_ALNUM 1UL
#define MINI_WCTYPE_ALPHA 2UL
#define MINI_WCTYPE_BLANK 3UL
#define MINI_WCTYPE_CNTRL 4UL
#define MINI_WCTYPE_DIGIT 5UL
#define MINI_WCTYPE_GRAPH 6UL
#define MINI_WCTYPE_LOWER 7UL
#define MINI_WCTYPE_PRINT 8UL
#define MINI_WCTYPE_PUNCT 9UL
#define MINI_WCTYPE_SPACE 10UL
#define MINI_WCTYPE_UPPER 11UL
#define MINI_WCTYPE_XDIGIT 12UL

#define MINI_WCTRANS_TOLOWER 1UL
#define MINI_WCTRANS_TOUPPER 2UL

static int valid_scalar(wint_t wc)
{
    return wc <= 0x10ffffU && !(wc >= 0xd800U && wc <= 0xdfffU);
}

static int ascii_alpha(wint_t wc)
{
    return (wc >= (wint_t)'A' && wc <= (wint_t)'Z') ||
           (wc >= (wint_t)'a' && wc <= (wint_t)'z');
}

static int ascii_digit(wint_t wc)
{
    return wc >= (wint_t)'0' && wc <= (wint_t)'9';
}

static int utf8_space(wint_t wc)
{
    if (wc == 0x0085U || wc == 0x00a0U || wc == 0x1680U ||
        wc == 0x2028U || wc == 0x2029U || wc == 0x202fU ||
        wc == 0x205fU || wc == 0x3000U) {
        return 1;
    }
    return wc >= 0x2000U && wc <= 0x200aU;
}

static int utf8_blank(wint_t wc)
{
    return wc == 0x00a0U || wc == 0x1680U || wc == 0x202fU ||
           wc == 0x205fU || wc == 0x3000U ||
           (wc >= 0x2000U && wc <= 0x200aU);
}

static int unicode_upper(wint_t wc)
{
    if ((wc >= 0x00c0U && wc <= 0x00d6U) ||
        (wc >= 0x00d8U && wc <= 0x00deU) || wc == 0x0178U) {
        return 1;
    }
    if ((wc >= 0x0391U && wc <= 0x03a1U) ||
        (wc >= 0x03a3U && wc <= 0x03abU)) {
        return 1;
    }
    if ((wc >= 0x0400U && wc <= 0x042fU) ||
        (wc >= 0x0531U && wc <= 0x0556U)) {
        return 1;
    }
    return 0;
}

static int unicode_lower(wint_t wc)
{
    if ((wc >= 0x00e0U && wc <= 0x00f6U) ||
        (wc >= 0x00f8U && wc <= 0x00ffU)) {
        return 1;
    }
    if ((wc >= 0x03b1U && wc <= 0x03c1U) ||
        (wc >= 0x03c2U && wc <= 0x03cbU)) {
        return 1;
    }
    if ((wc >= 0x0430U && wc <= 0x045fU) ||
        (wc >= 0x0561U && wc <= 0x0586U)) {
        return 1;
    }
    return 0;
}

static int unicode_uncased_alpha(wint_t wc)
{
    if ((wc >= 0x3041U && wc <= 0x3096U) ||
        (wc >= 0x30a1U && wc <= 0x30faU) ||
        (wc >= 0x3400U && wc <= 0x4dbfU) ||
        (wc >= 0x4e00U && wc <= 0x9fffU) ||
        (wc >= 0xac00U && wc <= 0xd7a3U) ||
        (wc >= 0xf900U && wc <= 0xfaffU) ||
        (wc >= 0x20000U && wc <= 0x2fa1fU)) {
        return 1;
    }
    return 0;
}

int iswalpha(wint_t wc)
{
    if (wc <= 0x7fU) {
        return ascii_alpha(wc);
    }
    if (!__mini_locale_is_utf8() || !valid_scalar(wc)) {
        return 0;
    }
    return unicode_upper(wc) || unicode_lower(wc) || unicode_uncased_alpha(wc);
}

int iswdigit(wint_t wc)
{
    return ascii_digit(wc);
}

int iswalnum(wint_t wc)
{
    return iswalpha(wc) || iswdigit(wc);
}

int iswcntrl(wint_t wc)
{
    if (wc <= 0x1fU || (wc >= 0x7fU && wc <= 0x9fU)) {
        return 1;
    }
    return 0;
}

int iswspace(wint_t wc)
{
    if (wc == (wint_t)' ' || wc == (wint_t)'\t' ||
        wc == (wint_t)'\n' || wc == (wint_t)'\v' ||
        wc == (wint_t)'\f' || wc == (wint_t)'\r') {
        return 1;
    }
    return __mini_locale_is_utf8() && valid_scalar(wc) && utf8_space(wc);
}

int iswblank(wint_t wc)
{
    if (wc == (wint_t)' ' || wc == (wint_t)'\t') {
        return 1;
    }
    return __mini_locale_is_utf8() && valid_scalar(wc) && utf8_blank(wc);
}

int iswupper(wint_t wc)
{
    if (wc <= 0x7fU) {
        return wc >= (wint_t)'A' && wc <= (wint_t)'Z';
    }
    return __mini_locale_is_utf8() && valid_scalar(wc) && unicode_upper(wc);
}

int iswlower(wint_t wc)
{
    if (wc <= 0x7fU) {
        return wc >= (wint_t)'a' && wc <= (wint_t)'z';
    }
    return __mini_locale_is_utf8() && valid_scalar(wc) && unicode_lower(wc);
}

int iswxdigit(wint_t wc)
{
    return ascii_digit(wc) ||
           (wc >= (wint_t)'A' && wc <= (wint_t)'F') ||
           (wc >= (wint_t)'a' && wc <= (wint_t)'f');
}

int iswprint(wint_t wc)
{
    if (wc == WEOF) {
        return 0;
    }
    if (wc <= 0x7fU) {
        return wc >= 0x20U && wc <= 0x7eU;
    }
    return __mini_locale_is_utf8() && valid_scalar(wc) && !iswcntrl(wc);
}

int iswgraph(wint_t wc)
{
    return iswprint(wc) && !iswspace(wc);
}

int iswpunct(wint_t wc)
{
    return iswgraph(wc) && !iswalnum(wc);
}

wint_t towlower(wint_t wc)
{
    if (wc >= (wint_t)'A' && wc <= (wint_t)'Z') {
        return wc + ((wint_t)'a' - (wint_t)'A');
    }
    if (!__mini_locale_is_utf8() || !valid_scalar(wc)) {
        return wc;
    }
    if ((wc >= 0x00c0U && wc <= 0x00d6U) ||
        (wc >= 0x00d8U && wc <= 0x00deU)) {
        return wc + 0x20U;
    }
    if (wc == 0x0178U) {
        return 0x00ffU;
    }
    if ((wc >= 0x0391U && wc <= 0x03a1U) ||
        (wc >= 0x03a3U && wc <= 0x03abU) ||
        (wc >= 0x0410U && wc <= 0x042fU)) {
        return wc + 0x20U;
    }
    if (wc >= 0x0400U && wc <= 0x040fU) {
        return wc + 0x50U;
    }
    if (wc >= 0x0531U && wc <= 0x0556U) {
        return wc + 0x30U;
    }
    return wc;
}

wint_t towupper(wint_t wc)
{
    if (wc >= (wint_t)'a' && wc <= (wint_t)'z') {
        return wc - ((wint_t)'a' - (wint_t)'A');
    }
    if (!__mini_locale_is_utf8() || !valid_scalar(wc)) {
        return wc;
    }
    if ((wc >= 0x00e0U && wc <= 0x00f6U) ||
        (wc >= 0x00f8U && wc <= 0x00feU)) {
        return wc - 0x20U;
    }
    if (wc == 0x00ffU) {
        return 0x0178U;
    }
    if ((wc >= 0x03b1U && wc <= 0x03c1U) ||
        (wc >= 0x03c3U && wc <= 0x03cbU) ||
        (wc >= 0x0430U && wc <= 0x044fU)) {
        return wc - 0x20U;
    }
    if (wc == 0x03c2U) {
        return 0x03a3U;
    }
    if (wc >= 0x0450U && wc <= 0x045fU) {
        return wc - 0x50U;
    }
    if (wc >= 0x0561U && wc <= 0x0586U) {
        return wc - 0x30U;
    }
    return wc;
}

static int same_name(const char *left, const char *right)
{
    if (left == (const char *)0) {
        return 0;
    }
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

wctype_t wctype(const char *property)
{
    if (same_name(property, "alnum")) return MINI_WCTYPE_ALNUM;
    if (same_name(property, "alpha")) return MINI_WCTYPE_ALPHA;
    if (same_name(property, "blank")) return MINI_WCTYPE_BLANK;
    if (same_name(property, "cntrl")) return MINI_WCTYPE_CNTRL;
    if (same_name(property, "digit")) return MINI_WCTYPE_DIGIT;
    if (same_name(property, "graph")) return MINI_WCTYPE_GRAPH;
    if (same_name(property, "lower")) return MINI_WCTYPE_LOWER;
    if (same_name(property, "print")) return MINI_WCTYPE_PRINT;
    if (same_name(property, "punct")) return MINI_WCTYPE_PUNCT;
    if (same_name(property, "space")) return MINI_WCTYPE_SPACE;
    if (same_name(property, "upper")) return MINI_WCTYPE_UPPER;
    if (same_name(property, "xdigit")) return MINI_WCTYPE_XDIGIT;
    return 0UL;
}

int iswctype(wint_t wc, wctype_t desc)
{
    switch (desc) {
    case MINI_WCTYPE_ALNUM: return iswalnum(wc);
    case MINI_WCTYPE_ALPHA: return iswalpha(wc);
    case MINI_WCTYPE_BLANK: return iswblank(wc);
    case MINI_WCTYPE_CNTRL: return iswcntrl(wc);
    case MINI_WCTYPE_DIGIT: return iswdigit(wc);
    case MINI_WCTYPE_GRAPH: return iswgraph(wc);
    case MINI_WCTYPE_LOWER: return iswlower(wc);
    case MINI_WCTYPE_PRINT: return iswprint(wc);
    case MINI_WCTYPE_PUNCT: return iswpunct(wc);
    case MINI_WCTYPE_SPACE: return iswspace(wc);
    case MINI_WCTYPE_UPPER: return iswupper(wc);
    case MINI_WCTYPE_XDIGIT: return iswxdigit(wc);
    default: return 0;
    }
}

wctrans_t wctrans(const char *property)
{
    if (same_name(property, "tolower")) return MINI_WCTRANS_TOLOWER;
    if (same_name(property, "toupper")) return MINI_WCTRANS_TOUPPER;
    return 0UL;
}

wint_t towctrans(wint_t wc, wctrans_t desc)
{
    if (desc == MINI_WCTRANS_TOLOWER) {
        return towlower(wc);
    }
    if (desc == MINI_WCTRANS_TOUPPER) {
        return towupper(wc);
    }
    return wc;
}
