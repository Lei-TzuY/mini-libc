#include <stddef.h>
#include <stdio.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "stdio_internal.h"

size_t __mini_fread_byte_core(void *restrict ptr, size_t size, size_t nmemb,
                              FILE *restrict stream);
size_t __mini_fwrite_byte_core(const void *restrict ptr, size_t size,
                               size_t nmemb, FILE *restrict stream);
int __mini_fgetc_byte_core(FILE *stream);
int __mini_getc_byte_core(FILE *stream);
int __mini_getchar_byte_core(void);
char *__mini_fgets_byte_core(char *restrict s, int n, FILE *restrict stream);
int __mini_ungetc_byte_core(int c, FILE *stream);
int __mini_fputc_byte_core(int c, FILE *stream);
int __mini_putc_byte_core(int c, FILE *stream);
int __mini_putchar_byte_core(int c);
int __mini_fputs_byte_core(const char *restrict s, FILE *restrict stream);
int __mini_puts_byte_core(const char *s);

size_t fread(void *restrict ptr, size_t size, size_t nmemb,
             FILE *restrict stream)
{
    size_t result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = 0U;
    } else {
        result = __mini_fread_byte_core(ptr, size, nmemb, stream);
    }
    __mini_stdio_unlock();
    return result;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb,
              FILE *restrict stream)
{
    size_t result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = 0U;
    } else {
        result = __mini_fwrite_byte_core(ptr, size, nmemb, stream);
    }
    __mini_stdio_unlock();
    return result;
}

int fgetc(FILE *stream)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_fgetc_byte_core(stream);
    }
    __mini_stdio_unlock();
    return result;
}

int getc(FILE *stream)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_getc_byte_core(stream);
    }
    __mini_stdio_unlock();
    return result;
}

int getchar(void)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stdin) == EOF) {
        result = EOF;
    } else {
        result = __mini_getchar_byte_core();
    }
    __mini_stdio_unlock();
    return result;
}

char *fgets(char *restrict s, int n, FILE *restrict stream)
{
    char *result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = (char *)0;
    } else {
        result = __mini_fgets_byte_core(s, n, stream);
    }
    __mini_stdio_unlock();
    return result;
}

int ungetc(int c, FILE *stream)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_ungetc_byte_core(c, stream);
    }
    __mini_stdio_unlock();
    return result;
}

int fputc(int c, FILE *stream)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_fputc_byte_core(c, stream);
    }
    __mini_stdio_unlock();
    return result;
}

int putc(int c, FILE *stream)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_putc_byte_core(c, stream);
    }
    __mini_stdio_unlock();
    return result;
}

int putchar(int c)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stdout) == EOF) {
        result = EOF;
    } else {
        result = __mini_putchar_byte_core(c);
    }
    __mini_stdio_unlock();
    return result;
}

int fputs(const char *restrict s, FILE *restrict stream)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_fputs_byte_core(s, stream);
    }
    __mini_stdio_unlock();
    return result;
}

int puts(const char *s)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stdout) == EOF) {
        result = EOF;
    } else {
        result = __mini_puts_byte_core(s);
    }
    __mini_stdio_unlock();
    return result;
}
