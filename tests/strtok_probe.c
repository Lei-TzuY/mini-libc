#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>

#define BUF_SIZE 96
#define RANDOM_CASES 2500
#define MAX_STEPS 64

static unsigned long rng_state = 0x6a09e667f3bcc909UL;

struct model_state {
    size_t next;
};

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static int contains_byte(const char *set, unsigned char byte)
{
    size_t i;

    for (i = 0; set[i] != '\0'; ++i) {
        if ((unsigned char)set[i] == byte) {
            return 1;
        }
    }
    return 0;
}

static long model_token(char *s, const char *delim, struct model_state *state,
                        int reset)
{
    size_t pos = reset ? 0 : state->next;
    size_t start;

    while (s[pos] != '\0' && contains_byte(delim, (unsigned char)s[pos])) {
        ++pos;
    }
    if (s[pos] == '\0') {
        state->next = pos;
        return -1L;
    }

    start = pos;
    while (s[pos] != '\0' && !contains_byte(delim, (unsigned char)s[pos])) {
        ++pos;
    }
    if (s[pos] != '\0') {
        s[pos] = '\0';
        ++pos;
    }
    state->next = pos;
    return (long)start;
}

static int token_equal(const char *a, const char *b)
{
    size_t i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if ((unsigned char)a[i] != (unsigned char)b[i]) {
            return 0;
        }
        ++i;
    }
    return a[i] == b[i];
}

static void copy_string(char *dest, const char *src)
{
    size_t i = 0;

    do {
        dest[i] = src[i];
    } while (src[i++] != '\0');
}

static void fill_input(char *s)
{
    static const unsigned char alphabet[] = {'a','b','c','d',',',';','|',0x80,0xff};
    size_t length = (size_t)(next_random() % (BUF_SIZE - 1));
    size_t i;

    for (i = 0; i < length; ++i) {
        s[i] = (char)alphabet[next_random() % (sizeof(alphabet) / sizeof(alphabet[0]))];
    }
    s[length] = '\0';
}

static void fill_delim(char *d)
{
    static const unsigned char alphabet[] = {'a','b',',',';','|',0x80,0xff};
    size_t length = (size_t)(next_random() % 6UL);
    size_t i;

    for (i = 0; i < length; ++i) {
        d[i] = (char)alphabet[next_random() % (sizeof(alphabet) / sizeof(alphabet[0]))];
    }
    d[length] = '\0';
}

static int deterministic_cases(void)
{
    char runs[] = ",;alpha,,beta;gamma;;";
    char changed[] = "a,b;c";
    char empty[] = "";
    char all[] = ",,,";
    char whole[] = "whole";
    char first[] = "x,y";
    char second[] = "m:n";
    char high[] = {(char)0x80, 'a', (char)0xff, 'b', '\0'};
    const char high_delim[] = {(char)0x80, (char)0xff, '\0'};
    char *token;

    errno = ERANGE;
    token = strtok(runs, ",;");
    if (token != runs + 2 || !token_equal(token, "alpha")) return 1;
    token = strtok((char *)0, ",;");
    if (token == (char *)0 || !token_equal(token, "beta")) return 2;
    token = strtok((char *)0, ",;");
    if (token == (char *)0 || !token_equal(token, "gamma")) return 3;
    if (strtok((char *)0, ",;") != (char *)0 ||
        strtok((char *)0, ",;") != (char *)0 || errno != ERANGE) return 4;

    token = strtok(changed, ",");
    if (token != changed || !token_equal(token, "a")) return 5;
    token = strtok((char *)0, ";");
    if (token == (char *)0 || !token_equal(token, "b")) return 6;
    token = strtok((char *)0, ",");
    if (token == (char *)0 || !token_equal(token, "c")) return 7;
    if (strtok((char *)0, ",") != (char *)0) return 8;

    if (strtok(empty, ",") != (char *)0 || strtok((char *)0, ",") != (char *)0) return 9;
    if (strtok(all, ",") != (char *)0 || strtok((char *)0, ",") != (char *)0) return 10;

    token = strtok(whole, "");
    if (token != whole || !token_equal(token, "whole") ||
        strtok((char *)0, "") != (char *)0) return 11;

    token = strtok(high, high_delim);
    if (token != high + 1 || !token_equal(token, "a")) return 12;
    token = strtok((char *)0, high_delim);
    if (token == (char *)0 || !token_equal(token, "b") ||
        strtok((char *)0, high_delim) != (char *)0) return 13;

    token = strtok(first, ",");
    if (token != first || !token_equal(token, "x")) return 14;
    token = strtok(second, ":");
    if (token != second || !token_equal(token, "m")) return 15;
    token = strtok((char *)0, ":");
    if (token == (char *)0 || !token_equal(token, "n") ||
        strtok((char *)0, ":") != (char *)0 || errno != ERANGE) return 16;

    return 0;
}

static int randomized_cases(void)
{
    char input[BUF_SIZE];
    char actual[BUF_SIZE];
    char expected[BUF_SIZE];
    char delim[8];
    int case_no;

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        struct model_state state = {0};
        char *actual_token;
        long expected_offset;
        int step;

        fill_input(input);
        copy_string(actual, input);
        copy_string(expected, input);
        fill_delim(delim);

        actual_token = strtok(actual, delim);
        expected_offset = model_token(expected, delim, &state, 1);
        if ((actual_token == (char *)0) != (expected_offset < 0)) return 20;
        if (actual_token != (char *)0) {
            if ((long)(actual_token - actual) != expected_offset ||
                !token_equal(actual_token, expected + expected_offset)) return 21;
        }

        for (step = 0; step < MAX_STEPS; ++step) {
            fill_delim(delim);
            actual_token = strtok((char *)0, delim);
            expected_offset = model_token(expected, delim, &state, 0);
            if ((actual_token == (char *)0) != (expected_offset < 0)) return 22;
            if (actual_token == (char *)0) break;
            if ((long)(actual_token - actual) != expected_offset ||
                !token_equal(actual_token, expected + expected_offset)) return 23;
        }
        if (step == MAX_STEPS) return 24;
    }
    return 0;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "strtok-ok\n";
    int result;

    (void)argc;
    (void)argv;
    (void)envp;

    result = deterministic_cases();
    if (result != 0) return result;
    result = randomized_cases();
    if (result != 0) return result;
    return mini_sys_write(1, ok, sizeof(ok) - 1) == (long)(sizeof(ok) - 1) ? 0 : 30;
}
