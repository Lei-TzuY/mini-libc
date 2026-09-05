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

static void nested_middle(void)
{
    int value = setjmp(inner_env);

    if (value == 0) {
        trace_state = 1;
        jump_inner();
    }
    if (value != 7 || trace_state != 2) {
        longjmp(outer_env, 91);
    }
    trace_state = 3;
    longjmp(outer_env, 11);
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
    int value = setjmp(zero_env);

    if (value == 0) {
        longjmp(zero_env, 0);
    }
    if (value != 1) {
        return 1;
    }

    value = setjmp(value_env);
    if (value == 0) {
        jump_value();
    }
    if (value != 37) {
        return 2;
    }

    trace_state = 0;
    value = setjmp(outer_env);
    if (value == 0) {
        nested_middle();
    }
    if (value != 11 || trace_state != 3) {
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
