#include <stddef.h>
#include <string.h>

#define BUF_SIZE 128
#define RANDOM_CASES 5000

size_t mini_test_strlen(const char *s);
int mini_test_strcmp(const char *left, const char *right);
int mini_test_strncmp(const char *left, const char *right, size_t n);
char *mini_test_strcpy(char *restrict dest, const char *restrict src);
char *mini_test_strncpy(char *restrict dest, const char *restrict src, size_t n);
char *mini_test_strchr(const char *s, int c);
char *mini_test_strrchr(const char *s, int c);
char *mini_test_strstr(const char *haystack, const char *needle);
size_t mini_test_strspn(const char *s, const char *accept);
size_t mini_test_strcspn(const char *s, const char *reject);
char *mini_test_strpbrk(const char *s, const char *accept);
char *mini_test_strcat(char *restrict dest, const char *restrict src);

static unsigned long rng_state = 0xe7037ed1a0b428dbUL;
static char *(*volatile host_strncpy_fn)(char *, const char *, size_t) = strncpy;

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static int sign_of(int value)
{
    return (value > 0) - (value < 0);
}

static void fill_string(char *s)
{
    size_t length = (size_t)(next_random() % (BUF_SIZE - 1));
    size_t i;

    for (i = 0; i < length; ++i) {
        s[i] = (char)(1 + (next_random() % 255));
    }
    s[length] = '\0';
}

static long pointer_offset(const char *base, const char *p)
{
    return p == (const char *)0 ? -1L : (long)(p - base);
}

int main(void)
{
    char left[BUF_SIZE];
    char right[BUF_SIZE];
    char mini_buf[BUF_SIZE];
    char host_buf[BUF_SIZE];
    char mini_cat[BUF_SIZE * 2];
    char host_cat[BUF_SIZE * 2];
    int case_no;

    {
        const char high[] = {(char)0x80, '\0'};
        const char low[] = {(char)0x7f, '\0'};
        if (sign_of(mini_test_strcmp(high, low)) != sign_of(strcmp(high, low)) ||
            sign_of(mini_test_strncmp(high, low, 1)) != sign_of(strncmp(high, low, 1))) {
            return 1;
        }
    }

    {
        const char high_text[] = {(char)0x80, (char)0xff, 'x', (char)0x80,
                                  (char)0xff, 'y', '\0'};
        const char high_needle[] = {(char)0x80, (char)0xff, 'y', '\0'};

        if (pointer_offset(high_text, mini_test_strstr(high_text, high_needle)) !=
            pointer_offset(high_text, strstr(high_text, high_needle))) {
            return 9;
        }
    }

    {
        const char high_text[] = {(char)0x80, (char)0xff, 'x', '\0'};
        const char high_set[] = {(char)0xff, (char)0x80, (char)0x80, '\0'};
        const char high_reject[] = {(char)0xff, (char)0xff, '\0'};

        if (mini_test_strspn(high_text, high_set) != strspn(high_text, high_set) ||
            mini_test_strcspn(high_text, high_reject) !=
                strcspn(high_text, high_reject) ||
            pointer_offset(high_text, mini_test_strpbrk(high_text, high_reject)) !=
                pointer_offset(high_text, strpbrk(high_text, high_reject))) {
            return 11;
        }
    }

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        size_t n;
        int c;
        char *mini_ptr;
        char *host_ptr;

        fill_string(left);
        fill_string(right);

        if (mini_test_strlen(left) != strlen(left)) {
            return 2;
        }
        if (sign_of(mini_test_strcmp(left, right)) != sign_of(strcmp(left, right))) {
            return 3;
        }
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        if (sign_of(mini_test_strncmp(left, right, n)) != sign_of(strncmp(left, right, n))) {
            return 4;
        }

        memset(mini_buf, 0xa5, sizeof(mini_buf));
        memset(host_buf, 0xa5, sizeof(host_buf));
        if (mini_test_strcpy(mini_buf, left) != mini_buf || strcpy(host_buf, left) != host_buf ||
            memcmp(mini_buf, host_buf, sizeof(mini_buf)) != 0) {
            return 5;
        }

        n = (size_t)(next_random() % BUF_SIZE);
        memset(mini_buf, 0xa5, sizeof(mini_buf));
        memset(host_buf, 0xa5, sizeof(host_buf));
        if (mini_test_strncpy(mini_buf, left, n) != mini_buf ||
            host_strncpy_fn(host_buf, left, n) != host_buf ||
            memcmp(mini_buf, host_buf, sizeof(mini_buf)) != 0) {
            return 6;
        }

        c = (int)(next_random() & 0x1ffUL);
        mini_ptr = mini_test_strchr(left, c);
        host_ptr = strchr(left, c);
        if (pointer_offset(left, mini_ptr) != pointer_offset(left, host_ptr)) {
            return 7;
        }
        mini_ptr = mini_test_strrchr(left, c);
        host_ptr = strrchr(left, c);
        if (pointer_offset(left, mini_ptr) != pointer_offset(left, host_ptr)) {
            return 8;
        }

        mini_ptr = mini_test_strstr(left, right);
        host_ptr = strstr(left, right);
        if (pointer_offset(left, mini_ptr) != pointer_offset(left, host_ptr)) {
            return 10;
        }


        if (mini_test_strspn(left, right) != strspn(left, right)) {
            return 12;
        }
        if (mini_test_strcspn(left, right) != strcspn(left, right)) {
            return 13;
        }
        mini_ptr = mini_test_strpbrk(left, right);
        host_ptr = strpbrk(left, right);
        if (pointer_offset(left, mini_ptr) != pointer_offset(left, host_ptr)) {
            return 14;
        }


        memset(mini_cat, 0xa5, sizeof(mini_cat));
        memset(host_cat, 0xa5, sizeof(host_cat));
        strcpy(mini_cat, left);
        strcpy(host_cat, left);
        if (mini_test_strcat(mini_cat, right) != mini_cat ||
            strcat(host_cat, right) != host_cat ||
            memcmp(mini_cat, host_cat, sizeof(mini_cat)) != 0) {
            return 15;
        }
    }

    return 0;
}
