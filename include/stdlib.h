#ifndef MINI_LIBC_STDLIB_H
#define MINI_LIBC_STDLIB_H

int atoi(const char *nptr);
long strtol(const char *restrict nptr, char **restrict endptr, int base);

#endif
