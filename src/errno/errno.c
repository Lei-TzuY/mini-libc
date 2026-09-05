#include <errno.h>

static int mini_errno_value;
static int *(*mini_errno_provider)(void);

void __mini_errno_set_provider(int *(*provider)(void))
{
    mini_errno_provider = provider;
}

int *__mini_errno_location(void)
{
    if (mini_errno_provider != (int *(*)(void))0) {
        return mini_errno_provider();
    }
    return &mini_errno_value;
}
