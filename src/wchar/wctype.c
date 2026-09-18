#include <ctype.h>
#include <wctype.h>

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

int iswalpha(wint_t wc)
{
    return is_ascii(wc) && isalpha((int)wc);
}

int iswblank(wint_t wc)
{
    return is_ascii(wc) && isblank((int)wc);
}

int iswdigit(wint_t wc)
{
    return is_ascii(wc) && isdigit((int)wc);
}

int iswalnum(wint_t wc)
{
    return iswalpha(wc) || iswdigit(wc);
}

int iswcntrl(wint_t wc)
{
    return is_ascii(wc) && iscntrl((int)wc);
}

int iswgraph(wint_t wc)
{
    return is_ascii(wc) && isgraph((int)wc);
}

int iswlower(wint_t wc)
{
    return is_ascii(wc) && islower((int)wc);
}

int iswprint(wint_t wc)
{
    return is_ascii(wc) && isprint((int)wc);
}

int iswpunct(wint_t wc)
{
    return is_ascii(wc) && ispunct((int)wc);
}

int iswspace(wint_t wc)
{
    return is_ascii(wc) && isspace((int)wc);
}

int iswupper(wint_t wc)
{
    return is_ascii(wc) && isupper((int)wc);
}

int iswxdigit(wint_t wc)
{
    return is_ascii(wc) && isxdigit((int)wc);
}

wint_t towlower(wint_t wc)
{
    return is_ascii(wc) ? (wint_t)tolower((int)wc) : wc;
}

wint_t towupper(wint_t wc)
{
    return is_ascii(wc) ? (wint_t)toupper((int)wc) : wc;
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

int iswctype(wint_t wc, wctype_t desc)
{
    switch (desc) {
    case MINI_WCTYPE_ALNUM:
        return iswalnum(wc);
    case MINI_WCTYPE_ALPHA:
        return iswalpha(wc);
    case MINI_WCTYPE_BLANK:
        return iswblank(wc);
    case MINI_WCTYPE_CNTRL:
        return iswcntrl(wc);
    case MINI_WCTYPE_DIGIT:
        return iswdigit(wc);
    case MINI_WCTYPE_GRAPH:
        return iswgraph(wc);
    case MINI_WCTYPE_LOWER:
        return iswlower(wc);
    case MINI_WCTYPE_PRINT:
        return iswprint(wc);
    case MINI_WCTYPE_PUNCT:
        return iswpunct(wc);
    case MINI_WCTYPE_SPACE:
        return iswspace(wc);
    case MINI_WCTYPE_UPPER:
        return iswupper(wc);
    case MINI_WCTYPE_XDIGIT:
        return iswxdigit(wc);
    default:
        return 0;
    }
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

