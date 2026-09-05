#ifndef MINI_LIBC_STDIO_H
#define MINI_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct __mini_FILE FILE;

extern FILE *__mini_stdin;
extern FILE *__mini_stdout;
extern FILE *__mini_stderr;

#define stdin (__mini_stdin)
#define stdout (__mini_stdout)
#define stderr (__mini_stderr)

FILE *fopen(const char *restrict filename, const char *restrict mode);
int fclose(FILE *stream);
int fflush(FILE *stream);
size_t fread(void *restrict ptr, size_t size, size_t nmemb,
             FILE *restrict stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb,
              FILE *restrict stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar(void);
char *fgets(char *restrict s, int n, FILE *restrict stream);
int ungetc(int c, FILE *stream);
int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int fputs(const char *restrict s, FILE *restrict stream);
int puts(const char *s);
int fscanf(FILE *restrict stream, const char *restrict format, ...);
int scanf(const char *restrict format, ...);
int sscanf(const char *restrict s, const char *restrict format, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int printf(const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
int vprintf(const char *restrict format, va_list ap);
int vsnprintf(char *restrict s, size_t n, const char *restrict format, va_list ap);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);

#endif
