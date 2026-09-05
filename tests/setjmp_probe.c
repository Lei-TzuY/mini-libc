#include <setjmp.h>
#include <mini/syscall.h>

static jmp_buf zero_env;
static jmp_buf value_env;
static jmp_buf outer_env;
static jmp_buf inner_env;
static jmp_buf register_env;
static volatile int trace_state;

int mini_test_setjmp_registers(jmp_buf env);

static void jump_value(void)
{
    longjmp(value_env, 37);
}

static void jump_inner(void)
{
    trace_state = 2;
    longjmp(inner_env, 7);
}

static int test_zero_normalization(void)
{
    switch (setjmp(zero_env)) {
    case 0:
        longjmp(zero_env, 0);
        return -1;
    case 1:
        return 0;
    default:
        return -1;
    }
}

static int test_explicit_value(void)
{
    switch (setjmp(value_env)) {
    case 0:
        jump_value();
        return -1;
    case 37:
        return 0;
    default:
        return -1;
    }
}

static void nested_middle(void)
{
    switch (setjmp(inner_env)) {
    case 0:
        trace_state = 1;
        jump_inner();
        return;
    case 7:
        if (trace_state != 2) {
            longjmp(outer_env, 91);
            return;
        }
        trace_state = 3;
        longjmp(outer_env, 11);
        return;
    default:
        longjmp(outer_env, 92);
        return;
    }
}

static int write_all(const char *text, unsigned long length)
{
    unsigned long offset = 0;

    while (offset < length) {
        long result = mini_sys_write(1, text + offset, length - offset);

        if (result <= 0) {
            return -1;
        }
        offset += (unsigned long)result;
    }
    return 0;
}

int main(void)
{
    if (test_zero_normalization() != 0) {
        return 1;
    }
    if (test_explicit_value() != 0) {
        return 2;
    }

    trace_state = 0;
    switch (setjmp(outer_env)) {
    case 0:
        nested_middle();
        return 3;
    case 11:
        if (trace_state != 3) {
            return 3;
        }
        break;
    default:
        return 3;
    }

    if (mini_test_setjmp_registers(register_env) != 0) {
        return 4;
    }

    if (write_all("setjmp-ok\n", 10) != 0) {
        return 5;
    }
    return 0;
}
