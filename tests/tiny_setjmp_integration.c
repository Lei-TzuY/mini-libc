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
    int value = setjmp(inner_env);

    if (value == 0) {
        trace_state = 1;
        deep_jump(4);
    }
    if (value != 1 || trace_state != 2) {
        longjmp(outer_env, 99);
    }
    trace_state = 3;
    longjmp(outer_env, 27);
}

int main(void)
{
    int value = setjmp(outer_env);

    if (value == 0) {
        run_nested();
    }
    if (value != 27 || trace_state != 3) {
        return 1;
    }
    if (mini_sys_write(1, "tiny-setjmp-ok\n", 16) != 16) {
        return 2;
    }
    return 0;
}
