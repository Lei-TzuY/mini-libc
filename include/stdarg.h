#ifndef MINI_LIBC_STDARG_H
#define MINI_LIBC_STDARG_H

#if defined(__GNUC__) || defined(__clang__)
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start((ap), (last))
#define va_arg(ap, type) __builtin_va_arg((ap), type)
#define va_copy(dest, src) __builtin_va_copy((dest), (src))
#define __va_copy(dest, src) va_copy((dest), (src))
#define va_end(ap) __builtin_va_end((ap))
#else
typedef struct __mini_va_list_tag {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];
#define va_start(ap, last) __builtin_va_start((ap))
#define va_arg(ap, type) __builtin_va_arg((ap), type)
#define va_copy(dest, src) ((dest)[0] = (src)[0])
#define __va_copy(dest, src) va_copy((dest), (src))
#define va_end(ap) ((void)0)
#endif

#endif
