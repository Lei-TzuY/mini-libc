#ifndef MINI_LIBC_SETJMP_H
#define MINI_LIBC_SETJMP_H

typedef struct {
    unsigned long __rbx;
    unsigned long __rbp;
    unsigned long __r12;
    unsigned long __r13;
    unsigned long __r14;
    unsigned long __r15;
    unsigned long __rsp;
    unsigned long __rip;
} jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int value);

#endif
