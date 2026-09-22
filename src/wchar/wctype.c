#include <ctype.h>
#include <wctype.h>

#include "../locale/locale_internal.h"
#include "unicode_props.h"

enum {
    MINI_WCTYPE_ALNUM = 1,
    MINI_WCTYPE_ALPHA,
    MINI_WCTYPE_BLANK,
    MINI_WCTYPE_CNTRL,
    MINI_WCTYPE_DIGIT,
    MINI_WCTYPE_GRAPH,
    MINI_WCTYPE_LOWER,
    MINI_WCTYPE_PRINT,
    MINI_WCTYPE_PUNCT,
    MINI_WCTYPE_SPACE,
    MINI_WCTYPE_UPPER,
    MINI_WCTYPE_XDIGIT
};

enum {
    MINI_WCTRANS_TOLOWER = 1,
    MINI_WCTRANS_TOUPPER
};

static int is_ascii(wint_t wc)
{
    return wc <= (wint_t)0x7fU;
}

static int same_name(const char *left, const char *right)
{
    if (left == (const char *)0 || right == (const char *)0) {
        return 0;
    }
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static int unicode_property(wint_t wc, unsigned int property)
{
    return __mini_unicode_has((unsigned int)wc, property);
}

static int locale_object_utf8(locale_t locale, int *utf8)
{
    struct mini_locale_state state;

    if (utf8 == (int *)0 ||
        !__mini_locale_object_copy_state(locale, &state)) {
        return 0;
    }
    *utf8 = __mini_locale_state_is_utf8(&state);
    return 1;
}

static int iswalpha_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_ALPHA)
                : is_ascii(wc) && isalpha((int)wc);
}

static int iswblank_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_BLANK)
                : is_ascii(wc) && isblank((int)wc);
}

static int iswdigit_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_DIGIT)
                : is_ascii(wc) && isdigit((int)wc);
}

static int iswalnum_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_ALPHA | MINI_UNICODE_DIGIT)
                : is_ascii(wc) && isalnum((int)wc);
}

static int iswcntrl_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_CNTRL)
                : is_ascii(wc) && iscntrl((int)wc);
}

static int iswgraph_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_GRAPH)
                : is_ascii(wc) && isgraph((int)wc);
}

static int iswlower_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_LOWER)
                : is_ascii(wc) && islower((int)wc);
}

static int iswprint_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_PRINT)
                : is_ascii(wc) && isprint((int)wc);
}

static int iswpunct_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_PUNCT)
                : is_ascii(wc) && ispunct((int)wc);
}

static int iswspace_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_SPACE)
                : is_ascii(wc) && isspace((int)wc);
}

static int iswupper_mode(wint_t wc, int utf8)
{
    return utf8 ? unicode_property(wc, MINI_UNICODE_UPPER)
                : is_ascii(wc) && isupper((int)wc);
}

static int iswxdigit_mode(wint_t wc, int utf8)
{
    (void)utf8;
    return is_ascii(wc) && isxdigit((int)wc);
}

static wint_t towlower_mode(wint_t wc, int utf8)
{
    if (utf8) {
        return (wint_t)__mini_unicode_tolower((unsigned int)wc);
    }
    return is_ascii(wc) ? (wint_t)tolower((int)wc) : wc;
}

static wint_t towupper_mode(wint_t wc, int utf8)
{
    if (utf8) {
        return (wint_t)__mini_unicode_toupper((unsigned int)wc);
    }
    return is_ascii(wc) ? (wint_t)toupper((int)wc) : wc;
}

static int iswctype_mode(wint_t wc, wctype_t desc, int utf8)
{
    switch (desc) {
    case MINI_WCTYPE_ALNUM:
        return iswalnum_mode(wc, utf8);
    case MINI_WCTYPE_ALPHA:
        return iswalpha_mode(wc, utf8);
    case MINI_WCTYPE_BLANK:
        return iswblank_mode(wc, utf8);
    case MINI_WCTYPE_CNTRL:
        return iswcntrl_mode(wc, utf8);
    case MINI_WCTYPE_DIGIT:
        return iswdigit_mode(wc, utf8);
    case MINI_WCTYPE_GRAPH:
        return iswgraph_mode(wc, utf8);
    case MINI_WCTYPE_LOWER:
        return iswlower_mode(wc, utf8);
    case MINI_WCTYPE_PRINT:
        return iswprint_mode(wc, utf8);
    case MINI_WCTYPE_PUNCT:
        return iswpunct_mode(wc, utf8);
    case MINI_WCTYPE_SPACE:
        return iswspace_mode(wc, utf8);
    case MINI_WCTYPE_UPPER:
        return iswupper_mode(wc, utf8);
    case MINI_WCTYPE_XDIGIT:
        return iswxdigit_mode(wc, utf8);
    default:
        return 0;
    }
}

static wint_t towctrans_mode(wint_t wc, wctrans_t desc, int utf8)
{
    if (desc == MINI_WCTRANS_TOLOWER) {
        return towlower_mode(wc, utf8);
    }
    if (desc == MINI_WCTRANS_TOUPPER) {
        return towupper_mode(wc, utf8);
    }
    return wc;
}

int iswalpha(wint_t wc)
{
    return iswalpha_mode(wc, __mini_locale_is_utf8());
}

int iswblank(wint_t wc)
{
    return iswblank_mode(wc, __mini_locale_is_utf8());
}

int iswdigit(wint_t wc)
{
    return iswdigit_mode(wc, __mini_locale_is_utf8());
}

int iswalnum(wint_t wc)
{
    return iswalnum_mode(wc, __mini_locale_is_utf8());
}

int iswcntrl(wint_t wc)
{
    return iswcntrl_mode(wc, __mini_locale_is_utf8());
}

int iswgraph(wint_t wc)
{
    return iswgraph_mode(wc, __mini_locale_is_utf8());
}

int iswlower(wint_t wc)
{
    return iswlower_mode(wc, __mini_locale_is_utf8());
}

int iswprint(wint_t wc)
{
    return iswprint_mode(wc, __mini_locale_is_utf8());
}

int iswpunct(wint_t wc)
{
    return iswpunct_mode(wc, __mini_locale_is_utf8());
}

int iswspace(wint_t wc)
{
    return iswspace_mode(wc, __mini_locale_is_utf8());
}

int iswupper(wint_t wc)
{
    return iswupper_mode(wc, __mini_locale_is_utf8());
}

int iswxdigit(wint_t wc)
{
    return iswxdigit_mode(wc, __mini_locale_is_utf8());
}

#define MINI_DEFINE_WCTYPE_L(name, mode_fn)                                  \
    int name##_l(wint_t wc, locale_t locale)                                \
    {                                                                         \
        int utf8;                                                             \
        if (!locale_object_utf8(locale, &utf8)) {                            \
            return 0;                                                         \
        }                                                                     \
        return mode_fn(wc, utf8);                                             \
    }

MINI_DEFINE_WCTYPE_L(iswalnum, iswalnum_mode)
MINI_DEFINE_WCTYPE_L(iswalpha, iswalpha_mode)
MINI_DEFINE_WCTYPE_L(iswblank, iswblank_mode)
MINI_DEFINE_WCTYPE_L(iswcntrl, iswcntrl_mode)
MINI_DEFINE_WCTYPE_L(iswdigit, iswdigit_mode)
MINI_DEFINE_WCTYPE_L(iswgraph, iswgraph_mode)
MINI_DEFINE_WCTYPE_L(iswlower, iswlower_mode)
MINI_DEFINE_WCTYPE_L(iswprint, iswprint_mode)
MINI_DEFINE_WCTYPE_L(iswpunct, iswpunct_mode)
MINI_DEFINE_WCTYPE_L(iswspace, iswspace_mode)
MINI_DEFINE_WCTYPE_L(iswupper, iswupper_mode)
MINI_DEFINE_WCTYPE_L(iswxdigit, iswxdigit_mode)

wint_t towlower(wint_t wc)
{
    return towlower_mode(wc, __mini_locale_is_utf8());
}

wint_t towupper(wint_t wc)
{
    return towupper_mode(wc, __mini_locale_is_utf8());
}

wint_t towlower_l(wint_t wc, locale_t locale)
{
    int utf8;

    if (!locale_object_utf8(locale, &utf8)) {
        return wc;
    }
    return towlower_mode(wc, utf8);
}

wint_t towupper_l(wint_t wc, locale_t locale)
{
    int utf8;

    if (!locale_object_utf8(locale, &utf8)) {
        return wc;
    }
    return towupper_mode(wc, utf8);
}

wctype_t wctype(const char *property)
{
    static const char *const names[] = {
        (const char *)0,
        "alnum", "alpha", "blank", "cntrl", "digit", "graph",
        "lower", "print", "punct", "space", "upper", "xdigit"
    };
    wctype_t index;

    for (index = MINI_WCTYPE_ALNUM; index <= MINI_WCTYPE_XDIGIT; ++index) {
        if (same_name(property, names[index])) {
            return index;
        }
    }
    return (wctype_t)0;
}

wctype_t wctype_l(const char *property, locale_t locale)
{
    int utf8;

    if (!locale_object_utf8(locale, &utf8)) {
        return (wctype_t)0;
    }
    (void)utf8;
    return wctype(property);
}

int iswctype(wint_t wc, wctype_t desc)
{
    return iswctype_mode(wc, desc, __mini_locale_is_utf8());
}

int iswctype_l(wint_t wc, wctype_t desc, locale_t locale)
{
    int utf8;

    if (!locale_object_utf8(locale, &utf8)) {
        return 0;
    }
    return iswctype_mode(wc, desc, utf8);
}

wctrans_t wctrans(const char *property)
{
    if (same_name(property, "tolower")) {
        return MINI_WCTRANS_TOLOWER;
    }
    if (same_name(property, "toupper")) {
        return MINI_WCTRANS_TOUPPER;
    }
    return (wctrans_t)0;
}

wctrans_t wctrans_l(const char *property, locale_t locale)
{
    int utf8;

    if (!locale_object_utf8(locale, &utf8)) {
        return (wctrans_t)0;
    }
    (void)utf8;
    return wctrans(property);
}

wint_t towctrans(wint_t wc, wctrans_t desc)
{
    return towctrans_mode(wc, desc, __mini_locale_is_utf8());
}

wint_t towctrans_l(wint_t wc, wctrans_t desc, locale_t locale)
{
    int utf8;

    if (!locale_object_utf8(locale, &utf8)) {
        return wc;
    }
    return towctrans_mode(wc, desc, utf8);
}
