#ifndef MINI_LIBC_PTHREAD_H
#define MINI_LIBC_PTHREAD_H

int pthread_atfork(void (*prepare)(void),
                   void (*parent)(void),
                   void (*child)(void));

#endif
