#include <setjmp.h>
#include <mini/syscall.h>

static jmp_buf outer_env;
static jmp_buf inner_env;
static volatile int trace_state;

static void deep_jump(int depth)
{
    if (depth == 0) {
        trace_state = 2;
        longjmp(inner_env, 0);
    }
    deep_jump(depth - 1);
}

static void run_nested(void)
{
    switch (setjmp(inner_env)) {
    case 0:
        trace_state = 1;
        deep_jump(4);
        return;
    case 1:
        if (trace_state != 2) {
            longjmp(outer_env, 99);
            return;
        }
        trace_state = 3;
        longjmp(outer_env, 27);
        return;
    default:
        longjmp(outer_env, 98);
        return;
    }
}

int main(void)
{
    switch (setjmp(outer_env)) {
    case 0:
        run_nested();
        return 1;
    case 27:
        if (trace_state != 3) {
            return 1;
        }
        break;
    default:
        return 1;
    }

    if (mini_sys_write(1, "tiny-setjmp-ok\n", 16) != 16) {
        return 2;
    }
    return 0;
}
