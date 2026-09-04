#ifndef MINI_LIBC_STDIO_H
#define MINI_LIBC_STDIO_H

#define EOF (-1)

typedef struct __mini_FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar(void);
int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int fputs(const char *restrict s, FILE *restrict stream);
int puts(const char *s);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);

#endif
