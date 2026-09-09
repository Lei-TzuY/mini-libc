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
    unsigned char byte;
    wchar_t wc = 0;
    mbstate_t state = {0U, 0U};
    size_t converted;
    wint_t result;

    __mini_stdio_lock();
    if (__mini_stdio_require_wide(stream) == EOF) {
        result = WEOF;
    } else if (__mini_stdio_read(stream, &byte, 1U) != 1U) {
        result = WEOF;
    } else {
        converted = mbrtowc(&wc, (const char *)&byte, 1U, &state);
        if (converted == (size_t)-1) {
            result = wide_error(stream, errno);
        } else {
            result = (wint_t)wc;
        }
    }
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

wint_t fputwc(wchar_t wc, FILE *stream)
{
    char byte;
    mbstate_t state = {0U, 0U};
    size_t converted;
    wint_t result;

    __mini_stdio_lock();
    if (__mini_stdio_require_wide(stream) == EOF) {
        result = WEOF;
    } else {
        converted = wcrtomb(&byte, wc, &state);
        if (converted == (size_t)-1) {
            result = wide_error(stream, errno);
        } else if (__mini_stdio_write(stream, (const unsigned char *)&byte,
                                      converted) != converted) {
            result = WEOF;
        } else {
            result = (wint_t)wc;
        }
    }
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
