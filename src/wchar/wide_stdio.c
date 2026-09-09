#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <wchar.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "../stdio/stdio_internal.h"

static wint_t wide_error(FILE *stream, int error)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return WEOF;
}

static wint_t wide_read_character_unlocked(FILE *stream)
{
    unsigned char byte;
    wchar_t wc = 0;
    mbstate_t state = {0U, 0U};
    size_t converted;

    if (__mini_stdio_read(stream, &byte, 1U) != 1U) {
        return WEOF;
    }

    converted = mbrtowc(&wc, (const char *)&byte, 1U, &state);
    if (converted == (size_t)-1) {
        return wide_error(stream, errno);
    }
    return (wint_t)wc;
}

static wint_t wide_write_character_unlocked(wchar_t wc, FILE *stream)
{
    char byte;
    mbstate_t state = {0U, 0U};
    size_t converted;

    converted = wcrtomb(&byte, wc, &state);
    if (converted == (size_t)-1) {
        return wide_error(stream, errno);
    }
    if (__mini_stdio_write(stream, (const unsigned char *)&byte, converted) !=
        converted) {
        return WEOF;
    }
    return (wint_t)wc;
}

static wint_t wide_get_unlocked(FILE *stream)
{
    if (__mini_stdio_require_wide(stream) == EOF) {
        return WEOF;
    }
    return wide_read_character_unlocked(stream);
}

static wint_t wide_put_unlocked(wchar_t wc, FILE *stream)
{
    if (__mini_stdio_require_wide(stream) == EOF) {
        return WEOF;
    }
    return wide_write_character_unlocked(wc, stream);
}

int fwide(FILE *stream, int mode)
{
    int result;

    __mini_stdio_lock();
    result = __mini_stdio_fwide_unlocked(stream, mode);
    __mini_stdio_unlock();
    return result;
}

wint_t fgetwc(FILE *stream)
{
    wint_t result;

    __mini_stdio_lock();
    result = wide_get_unlocked(stream);
    __mini_stdio_unlock();
    return result;
}

wint_t getwc(FILE *stream)
{
    return fgetwc(stream);
}

wint_t getwchar(void)
{
    return fgetwc(stdin);
}

wchar_t *fgetws(wchar_t *restrict s, int n, FILE *restrict stream)
{
    size_t length = 0U;
    wchar_t *result = s;

    __mini_stdio_lock();
    if (s == (wchar_t *)0 || n <= 0) {
        (void)wide_error(stream, EINVAL);
        result = (wchar_t *)0;
    } else if (__mini_stdio_require_wide(stream) == EOF) {
        result = (wchar_t *)0;
    } else {
        while (length + 1U < (size_t)n) {
            wint_t wc = wide_read_character_unlocked(stream);

            if (wc == WEOF) {
                if ((stream->state & MINI_FILE_EOF) != 0U) {
                    if (length == 0U) {
                        result = (wchar_t *)0;
                    }
                } else {
                    result = (wchar_t *)0;
                }
                break;
            }
            s[length++] = (wchar_t)wc;
            if (wc == (wint_t)'\n') {
                break;
            }
        }
        s[length] = 0;
    }
    __mini_stdio_unlock();
    return result;
}

wint_t fputwc(wchar_t wc, FILE *stream)
{
    wint_t result;

    __mini_stdio_lock();
    result = wide_put_unlocked(wc, stream);
    __mini_stdio_unlock();
    return result;
}

wint_t putwc(wchar_t wc, FILE *stream)
{
    return fputwc(wc, stream);
}

wint_t putwchar(wchar_t wc)
{
    return fputwc(wc, stdout);
}

int fputws(const wchar_t *restrict s, FILE *restrict stream)
{
    int result = 0;

    __mini_stdio_lock();
    if (s == (const wchar_t *)0) {
        (void)wide_error(stream, EINVAL);
        result = EOF;
    } else if (__mini_stdio_require_wide(stream) == EOF) {
        result = EOF;
    } else {
        while (*s != 0) {
            if (wide_write_character_unlocked(*s++, stream) == WEOF) {
                result = EOF;
                break;
            }
        }
    }
    __mini_stdio_unlock();
    return result;
}

wint_t ungetwc(wint_t wc, FILE *stream)
{
    char byte;
    mbstate_t state = {0U, 0U};
    size_t converted;
    wint_t result;

    if (wc == WEOF) {
        return WEOF;
    }

    __mini_stdio_lock();
    if (__mini_stdio_require_wide(stream) == EOF) {
        result = WEOF;
    } else if ((stream->mode & MINI_FILE_READABLE) == 0U ||
               (stream->state & MINI_FILE_WRITE_NEEDS_SYNC) != 0U ||
               stream->pushback_valid != 0U) {
        result = wide_error(stream, EINVAL);
    } else {
        converted = wcrtomb(&byte, (wchar_t)wc, &state);
        if (converted == (size_t)-1 || converted != 1U) {
            result = wide_error(stream, converted == (size_t)-1 ? errno : EILSEQ);
        } else {
            stream->pushback_byte = (unsigned char)byte;
            stream->pushback_valid = 1U;
            stream->state &= ~MINI_FILE_EOF;
            stream->state |= MINI_FILE_READ_NEEDS_POSITION;
            result = wc;
        }
    }
    __mini_stdio_unlock();
    return result;
}
