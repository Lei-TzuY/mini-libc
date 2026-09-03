#ifndef MINI_LIBC_STDLIB_H
#define MINI_LIBC_STDLIB_H

#include <stddef.h>

int atoi(const char *nptr);
long strtol(const char *restrict nptr, char **restrict endptr, int base);
unsigned long strtoul(const char *restrict nptr, char **restrict endptr, int base);
void *malloc(size_t size);
void free(void *ptr);

#endif
