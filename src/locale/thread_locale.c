#include "../internal/thread_runtime.h"
#include "locale_internal.h"

static struct mini_locale_state *thread_locale_provider(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    if (tcb != (struct mini_thread_tcb *)0 && tcb->locale_override_active) {
        return &tcb->locale_state;
    }
    return __mini_locale_process_state();
}

void __mini_thread_locale_runtime_enable(void)
{
    __mini_locale_set_state_provider(thread_locale_provider);
}

void __mini_thread_locale_init_child(struct mini_thread_tcb *child)
{
    if (child == (struct mini_thread_tcb *)0) {
        return;
    }

    __mini_locale_state_init(&child->locale_state);
    child->locale_handle = LC_GLOBAL_LOCALE;
    child->locale_override_active = 0U;
}

int __mini_locale_thread_install(locale_t handle,
                                 const struct mini_locale_state *state)
{
    struct mini_thread_tcb *tcb;

    if (handle == (locale_t)0 || handle == LC_GLOBAL_LOCALE ||
        state == (const struct mini_locale_state *)0) {
        return 0;
    }

    tcb = __mini_thread_current_tcb();
    if (tcb == (struct mini_thread_tcb *)0) {
        return 0;
    }
    __mini_locale_state_copy(&tcb->locale_state, state);
    tcb->locale_handle = handle;
    tcb->locale_override_active = 1U;
    return 1;
}

locale_t __mini_locale_thread_current_handle(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    if (tcb != (struct mini_thread_tcb *)0 &&
        tcb->locale_override_active) {
        return tcb->locale_handle;
    }
    return LC_GLOBAL_LOCALE;
}

void __mini_locale_thread_use_global(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    if (tcb != (struct mini_thread_tcb *)0) {
        tcb->locale_handle = LC_GLOBAL_LOCALE;
        tcb->locale_override_active = 0U;
    }
}

int __mini_locale_thread_override_active(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    return tcb != (struct mini_thread_tcb *)0 &&
           tcb->locale_override_active != 0U;
}
