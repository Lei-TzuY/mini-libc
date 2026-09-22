#include <errno.h>
#include <locale.h>
#include <stdlib.h>

#include "../internal/futex_lock.h"
#include "locale_internal.h"

struct mini_locale_object {
    struct mini_locale_state state;
    struct mini_locale_object *next;
};

static struct mini_locale_object *mini_locale_objects;
static struct mini_futex_lock mini_locale_object_lock_word =
    MINI_FUTEX_LOCK_INIT;

static void locale_object_lock(void)
{
    mini_futex_lock_acquire(&mini_locale_object_lock_word);
}

static void locale_object_unlock(void)
{
    mini_futex_lock_release(&mini_locale_object_lock_word);
}

static int copy_registered_state(locale_t handle,
                                 struct mini_locale_state *state)
{
    struct mini_locale_object *cursor;
    int found = 0;

    if (handle == (locale_t)0 || handle == LC_GLOBAL_LOCALE ||
        state == (struct mini_locale_state *)0) {
        return 0;
    }

    locale_object_lock();
    cursor = mini_locale_objects;
    while (cursor != (struct mini_locale_object *)0) {
        if ((locale_t)cursor == handle) {
            __mini_locale_state_copy(state, &cursor->state);
            found = 1;
            break;
        }
        cursor = cursor->next;
    }
    locale_object_unlock();
    return found;
}

static int replace_registered_state(locale_t handle,
                                    const struct mini_locale_state *state)
{
    struct mini_locale_object *cursor;
    int replaced = 0;

    if (handle == (locale_t)0 || handle == LC_GLOBAL_LOCALE ||
        state == (const struct mini_locale_state *)0) {
        return 0;
    }

    locale_object_lock();
    cursor = mini_locale_objects;
    while (cursor != (struct mini_locale_object *)0) {
        if ((locale_t)cursor == handle) {
            __mini_locale_state_copy(&cursor->state, state);
            replaced = 1;
            break;
        }
        cursor = cursor->next;
    }
    locale_object_unlock();
    return replaced;
}

static locale_t create_object_from_state(const struct mini_locale_state *state)
{
    struct mini_locale_object *object;

    object = (struct mini_locale_object *)malloc(sizeof(*object));
    if (object == (struct mini_locale_object *)0) {
        errno = ENOMEM;
        return (locale_t)0;
    }

    __mini_locale_state_copy(&object->state, state);
    locale_object_lock();
    object->next = mini_locale_objects;
    mini_locale_objects = object;
    locale_object_unlock();
    return (locale_t)object;
}

static int valid_category_mask(int category_mask)
{
    return (category_mask & ~LC_ALL_MASK) == 0;
}

static int apply_category_mask(struct mini_locale_state *state,
                               int category_mask, const char *locale)
{
    int category;

    for (category = LC_CTYPE; category <= LC_MONETARY; ++category) {
        if ((category_mask & (1 << category)) != 0 &&
            !__mini_locale_state_apply_name(state, category, locale)) {
            return 0;
        }
    }
    return 1;
}

locale_t newlocale(int category_mask, const char *locale, locale_t base)
{
    struct mini_locale_state candidate;
    locale_t result;
    int saved_errno = errno;

    if (!valid_category_mask(category_mask) ||
        locale == (const char *)0 || base == LC_GLOBAL_LOCALE) {
        errno = EINVAL;
        return (locale_t)0;
    }

    if (base == (locale_t)0) {
        __mini_locale_state_init(&candidate);
    } else if (!copy_registered_state(base, &candidate)) {
        errno = EINVAL;
        return (locale_t)0;
    }

    if (!apply_category_mask(&candidate, category_mask, locale)) {
        errno = ENOENT;
        return (locale_t)0;
    }

    if (base == (locale_t)0) {
        result = create_object_from_state(&candidate);
        if (result == (locale_t)0) {
            return (locale_t)0;
        }
    } else {
        if (!replace_registered_state(base, &candidate)) {
            errno = EINVAL;
            return (locale_t)0;
        }
        result = base;
    }

    errno = saved_errno;
    return result;
}

locale_t duplocale(locale_t locobj)
{
    struct mini_locale_state snapshot;
    int saved_errno = errno;
    locale_t result;

    if (locobj == LC_GLOBAL_LOCALE) {
        __mini_locale_state_copy(&snapshot, __mini_locale_process_state());
    } else if (!copy_registered_state(locobj, &snapshot)) {
        errno = EINVAL;
        return (locale_t)0;
    }

    result = create_object_from_state(&snapshot);
    if (result == (locale_t)0) {
        return (locale_t)0;
    }
    errno = saved_errno;
    return result;
}

void freelocale(locale_t locobj)
{
    struct mini_locale_object **link;
    struct mini_locale_object *object = (struct mini_locale_object *)0;
    int saved_errno = errno;

    if (locobj == (locale_t)0 || locobj == LC_GLOBAL_LOCALE) {
        errno = saved_errno;
        return;
    }

    locale_object_lock();
    link = &mini_locale_objects;
    while (*link != (struct mini_locale_object *)0) {
        if ((locale_t)*link == locobj) {
            object = *link;
            *link = object->next;
            object->next = (struct mini_locale_object *)0;
            break;
        }
        link = &(*link)->next;
    }
    locale_object_unlock();

    if (object != (struct mini_locale_object *)0) {
        free(object);
    }
    errno = saved_errno;
}

locale_t uselocale(locale_t newloc)
{
    struct mini_locale_state snapshot;
    locale_t previous = __mini_locale_thread_current_handle();
    int saved_errno = errno;

    if (newloc == (locale_t)0) {
        errno = saved_errno;
        return previous;
    }

    if (newloc == LC_GLOBAL_LOCALE) {
        __mini_locale_thread_use_global();
        errno = saved_errno;
        return previous;
    }

    if (!copy_registered_state(newloc, &snapshot) ||
        !__mini_locale_thread_install(newloc, &snapshot)) {
        errno = EINVAL;
        return (locale_t)0;
    }

    errno = saved_errno;
    return previous;
}
