#include <stddef.h>
#include <string.h>

#define BUF_SIZE 96
#define RANDOM_CASES 5000
#define MAX_STEPS 64

char *mini_test_strtok(char *restrict str, const char *restrict delim);

static unsigned long rng_state = 0xbb67ae8584caa73bUL;

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static void copy_string(char *dest, const char *src)
{
    size_t i = 0;

    do {
        dest[i] = src[i];
    } while (src[i++] != '\0');
}

static int token_equal(const char *a, const char *b)
{
    size_t i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if ((unsigned char)a[i] != (unsigned char)b[i]) return 0;
        ++i;
    }
    return a[i] == b[i];
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

static int compare_token(const char *mini_base, char *mini_token,
                         const char *host_base, char *host_token)
{
    if ((mini_token == (char *)0) != (host_token == (char *)0)) return 0;
    if (mini_token == (char *)0) return 1;
    if ((long)(mini_token - mini_base) != (long)(host_token - host_base)) return 0;
    return token_equal(mini_token, host_token);
}

int main(void)
{
    char input[BUF_SIZE];
    char mini_buf[BUF_SIZE];
    char host_buf[BUF_SIZE];
    char delim[8];
    int case_no;

    {
        char mini_change[] = "a,b;c";
        char host_change[] = "a,b;c";

        if (!compare_token(mini_change, mini_test_strtok(mini_change, ","),
                           host_change, strtok(host_change, ","))) return 1;
        if (!compare_token(mini_change, mini_test_strtok((char *)0, ";"),
                           host_change, strtok((char *)0, ";"))) return 2;
        if (!compare_token(mini_change, mini_test_strtok((char *)0, ","),
                           host_change, strtok((char *)0, ","))) return 3;
        if (!compare_token(mini_change, mini_test_strtok((char *)0, ","),
                           host_change, strtok((char *)0, ","))) return 4;
    }

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        char *mini_token;
        char *host_token;
        int step;

        fill_input(input);
        copy_string(mini_buf, input);
        copy_string(host_buf, input);
        fill_delim(delim);

        mini_token = mini_test_strtok(mini_buf, delim);
        host_token = strtok(host_buf, delim);
        if (!compare_token(mini_buf, mini_token, host_buf, host_token)) return 10;

        for (step = 0; step < MAX_STEPS; ++step) {
            fill_delim(delim);
            mini_token = mini_test_strtok((char *)0, delim);
            host_token = strtok((char *)0, delim);
            if (!compare_token(mini_buf, mini_token, host_buf, host_token)) return 11;
            if (mini_token == (char *)0) break;
        }
        if (step == MAX_STEPS) return 12;
    }
    return 0;
}
