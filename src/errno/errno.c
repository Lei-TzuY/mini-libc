#include <errno.h>

static int mini_errno_value;

int *__mini_errno_location(void)
{
    return &mini_errno_value;
}
