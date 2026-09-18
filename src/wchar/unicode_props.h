#ifndef MINI_LIBC_UNICODE_PROPS_H
#define MINI_LIBC_UNICODE_PROPS_H

#define MINI_UNICODE_ALPHA  (1U << 0)
#define MINI_UNICODE_DIGIT  (1U << 1)
#define MINI_UNICODE_LOWER  (1U << 2)
#define MINI_UNICODE_UPPER  (1U << 3)
#define MINI_UNICODE_SPACE  (1U << 4)
#define MINI_UNICODE_BLANK  (1U << 5)
#define MINI_UNICODE_CNTRL  (1U << 6)
#define MINI_UNICODE_PUNCT  (1U << 7)
#define MINI_UNICODE_PRINT  (1U << 8)
#define MINI_UNICODE_GRAPH  (1U << 9)

int __mini_unicode_has(unsigned int codepoint, unsigned int property);
unsigned int __mini_unicode_tolower(unsigned int codepoint);
unsigned int __mini_unicode_toupper(unsigned int codepoint);

#endif
