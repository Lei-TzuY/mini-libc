#ifndef MINI_LIBC_STDATOMIC_H
#define MINI_LIBC_STDATOMIC_H

/*
 * Atomics are a compiler-language boundary, like <stdarg.h>. GCC and Clang
 * provide their own C11 lowering contract through the next compiler header.
 * The pinned tiny-c compiler is built with -nostdinc for mini-libc integration,
 * so the fallback below mirrors its documented scalar atomic builtins.
 */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC system_header
#include_next <stdatomic.h>
#else

typedef enum memory_order {
    memory_order_relaxed = 0,
    memory_order_consume = 1,
    memory_order_acquire = 2,
    memory_order_release = 3,
    memory_order_acq_rel = 4,
    memory_order_seq_cst = 5
} memory_order;

typedef _Atomic _Bool atomic_bool;
typedef _Atomic char atomic_char;
typedef _Atomic signed char atomic_schar;
typedef _Atomic unsigned char atomic_uchar;
typedef _Atomic short atomic_short;
typedef _Atomic unsigned short atomic_ushort;
typedef _Atomic int atomic_int;
typedef _Atomic unsigned int atomic_uint;
typedef _Atomic long atomic_long;
typedef _Atomic unsigned long atomic_ulong;
typedef _Atomic long long atomic_llong;
typedef _Atomic unsigned long long atomic_ullong;
typedef atomic_bool atomic_flag;
typedef atomic_long atomic_intptr_t;
typedef atomic_ulong atomic_uintptr_t;
typedef atomic_long atomic_ptrdiff_t;
typedef atomic_ulong atomic_size_t;
typedef atomic_llong atomic_intmax_t;
typedef atomic_ullong atomic_uintmax_t;

#define ATOMIC_BOOL_LOCK_FREE 2
#define ATOMIC_CHAR_LOCK_FREE 2
#define ATOMIC_CHAR16_T_LOCK_FREE 2
#define ATOMIC_CHAR32_T_LOCK_FREE 2
#define ATOMIC_WCHAR_T_LOCK_FREE 2
#define ATOMIC_SHORT_LOCK_FREE 2
#define ATOMIC_INT_LOCK_FREE 2
#define ATOMIC_LONG_LOCK_FREE 2
#define ATOMIC_LLONG_LOCK_FREE 2
#define ATOMIC_POINTER_LOCK_FREE 2

#define ATOMIC_FLAG_INIT 0
#define ATOMIC_VAR_INIT(value) (value)
#define kill_dependency(value) (value)

#define atomic_init(object, desired) \
    ((void)__builtin_atomic_store((object), (desired), memory_order_relaxed))
#define atomic_is_lock_free(object) __builtin_atomic_is_lock_free((object))
#define atomic_load(object) \
    __builtin_atomic_load((object), memory_order_seq_cst)
#define atomic_load_explicit(object, order) \
    __builtin_atomic_load((object), (order))
#define atomic_store(object, desired) \
    ((void)__builtin_atomic_store((object), (desired), memory_order_seq_cst))
#define atomic_store_explicit(object, desired, order) \
    ((void)__builtin_atomic_store((object), (desired), (order)))
#define atomic_exchange(object, desired) \
    __builtin_atomic_exchange((object), (desired), memory_order_seq_cst)
#define atomic_exchange_explicit(object, desired, order) \
    __builtin_atomic_exchange((object), (desired), (order))

#define atomic_fetch_add(object, operand) \
    __builtin_atomic_fetch_add((object), (operand), memory_order_seq_cst)
#define atomic_fetch_add_explicit(object, operand, order) \
    __builtin_atomic_fetch_add((object), (operand), (order))
#define atomic_fetch_sub(object, operand) \
    __builtin_atomic_fetch_sub((object), (operand), memory_order_seq_cst)
#define atomic_fetch_sub_explicit(object, operand, order) \
    __builtin_atomic_fetch_sub((object), (operand), (order))
#define atomic_fetch_and(object, operand) \
    __builtin_atomic_fetch_and((object), (operand), memory_order_seq_cst)
#define atomic_fetch_and_explicit(object, operand, order) \
    __builtin_atomic_fetch_and((object), (operand), (order))
#define atomic_fetch_or(object, operand) \
    __builtin_atomic_fetch_or((object), (operand), memory_order_seq_cst)
#define atomic_fetch_or_explicit(object, operand, order) \
    __builtin_atomic_fetch_or((object), (operand), (order))
#define atomic_fetch_xor(object, operand) \
    __builtin_atomic_fetch_xor((object), (operand), memory_order_seq_cst)
#define atomic_fetch_xor_explicit(object, operand, order) \
    __builtin_atomic_fetch_xor((object), (operand), (order))

#define atomic_compare_exchange_strong(object, expected, desired) \
    __builtin_atomic_compare_exchange((object), (expected), (desired), \
                                      memory_order_seq_cst, \
                                      memory_order_seq_cst)
#define atomic_compare_exchange_strong_explicit(object, expected, desired, \
                                                success, failure) \
    __builtin_atomic_compare_exchange((object), (expected), (desired), \
                                      (success), (failure))
#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_strong((object), (expected), (desired))
#define atomic_compare_exchange_weak_explicit(object, expected, desired, \
                                              success, failure) \
    atomic_compare_exchange_strong_explicit((object), (expected), (desired), \
                                            (success), (failure))

#define atomic_flag_test_and_set(object) atomic_exchange((object), 1)
#define atomic_flag_test_and_set_explicit(object, order) \
    atomic_exchange_explicit((object), 1, (order))
#define atomic_flag_clear(object) atomic_store((object), 0)
#define atomic_flag_clear_explicit(object, order) \
    atomic_store_explicit((object), 0, (order))

#define atomic_thread_fence(order) __builtin_atomic_thread_fence((order))
#define atomic_signal_fence(order) __builtin_atomic_signal_fence((order))

#endif

#endif
