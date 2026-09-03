#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>

#define BUF_SIZE 96
#define RANDOM_CASES 2500

static unsigned long rng_state = 0xa0761d6478bd642fUL;

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

static size_t model_strlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

static int model_strcmp(const char *left, const char *right)
{
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;

    while (*a != 0 && *a == *b) {
        ++a;
        ++b;
    }
    return (int)*a - (int)*b;
}

static int model_strncmp(const char *left, const char *right, size_t n)
{
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    size_t i;

    for (i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == 0) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

static const char *model_strstr(const char *haystack, const char *needle)
{
    size_t start;

    if (needle[0] == '\0') {
        return haystack;
    }
    for (start = 0; haystack[start] != '\0'; ++start) {
        size_t i = 0;

        while (needle[i] != '\0' && haystack[start + i] != '\0' &&
               (unsigned char)haystack[start + i] == (unsigned char)needle[i]) {
            ++i;
        }
        if (needle[i] == '\0') {
            return haystack + start;
        }
    }
    return (const char *)0;
}

static void fill_string(char *s, size_t max_len)
{
    size_t length = (size_t)(next_random() % max_len);
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

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "string-ok\n";
    char source[BUF_SIZE];
    char other[BUF_SIZE];
    char dest[BUF_SIZE];
    char expected[BUF_SIZE];
    int case_no;

    (void)argc;
    (void)argv;
    (void)envp;

    if (strlen("") != 0 || strlen("abc") != 3) {
        return 30;
    }
    if (sign_of(strcmp("", "")) != 0 || sign_of(strcmp("abc", "abd")) >= 0 ||
        sign_of(strcmp("abd", "abc")) <= 0) {
        return 31;
    }
    {
        const char high[] = {(char)0x80, '\0'};
        const char low[] = {(char)0x7f, '\0'};
        if (sign_of(strcmp(high, low)) <= 0 || sign_of(strncmp(high, low, 1)) <= 0) {
            return 32;
        }
    }
    if (strncmp("x", "y", 0) != 0 || strncmp("prefix-a", "prefix-b", 6) != 0 ||
        sign_of(strncmp("prefix-a", "prefix-b", 8)) >= 0) {
        return 33;
    }

    memset(dest, 0x5a, sizeof(dest));
    if (strcpy(dest, "copy") != dest || memcmp(dest, "copy\0", 5) != 0 ||
        (unsigned char)dest[5] != 0x5a) {
        return 34;
    }

    memset(dest, 0x5a, sizeof(dest));
    if (strncpy(dest, "ab", 5) != dest || dest[0] != 'a' || dest[1] != 'b' ||
        dest[2] != '\0' || dest[3] != '\0' || dest[4] != '\0' ||
        (unsigned char)dest[5] != 0x5a) {
        return 35;
    }
    memset(dest, 0x5a, sizeof(dest));
    if (strncpy(dest, "abcd", 3) != dest || dest[0] != 'a' || dest[1] != 'b' ||
        dest[2] != 'c' || (unsigned char)dest[3] != 0x5a) {
        return 36;
    }

    {
        const char text[] = "abca";
        if (strchr(text, 'a') != text || strchr(text, 'z') != (char *)0 ||
            strchr(text, '\0') != text + 4 || strchr(text, 0x100 + 'b') != text + 1 ||
            strrchr(text, 'a') != text + 3 || strrchr(text, 'z') != (char *)0 ||
            strrchr(text, '\0') != text + 4 || strrchr(text, 0x100 + 'b') != text + 1) {
            return 37;
        }
    }

    {
        const char text[] = "aaaab-tail";
        const char high_text[] = {(char)0x80, (char)0xff, 'x', (char)0x80,
                                  (char)0xff, 'y', '\0'};
        const char high_needle[] = {(char)0x80, (char)0xff, 'y', '\0'};

        if (strstr(text, "") != text || strstr(text, "aaa") != text ||
            strstr(text, "aaab") != text + 1 || strstr(text, "tail") != text + 6 ||
            strstr(text, "missing") != (char *)0 ||
            strstr(high_text, high_needle) != high_text + 3) {
            return 38;
        }
    }

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        size_t n;
        size_t src_len;
        size_t i;
        int c;
        const char *actual_ptr;
        const char *expected_ptr = (const char *)0;

        fill_string(source, BUF_SIZE - 1);
        fill_string(other, BUF_SIZE - 1);
        src_len = model_strlen(source);

        if (strlen(source) != src_len) {
            return 40;
        }
        if (sign_of(strcmp(source, other)) != sign_of(model_strcmp(source, other))) {
            return 41;
        }
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        if (sign_of(strncmp(source, other, n)) != sign_of(model_strncmp(source, other, n))) {
            return 42;
        }

        memset(dest, 0xa5, sizeof(dest));
        if (strcpy(dest, source) != dest || memcmp(dest, source, src_len + 1) != 0) {
            return 43;
        }

        n = (size_t)(next_random() % BUF_SIZE);
        memset(dest, 0xa5, sizeof(dest));
        memset(expected, 0xa5, sizeof(expected));
        for (i = 0; i < n && source[i] != '\0'; ++i) {
            expected[i] = source[i];
        }
        for (; i < n; ++i) {
            expected[i] = '\0';
        }
        if (strncpy(dest, source, n) != dest || memcmp(dest, expected, sizeof(dest)) != 0) {
            return 44;
        }

        c = (int)(next_random() & 0x1ffUL);
        for (i = 0;; ++i) {
            if ((unsigned char)source[i] == (unsigned char)c) {
                expected_ptr = source + i;
                break;
            }
            if (source[i] == '\0') {
                break;
            }
        }
        actual_ptr = strchr(source, c);
        if (pointer_offset(source, actual_ptr) != pointer_offset(source, expected_ptr)) {
            return 45;
        }

        expected_ptr = (const char *)0;
        for (i = 0;; ++i) {
            if ((unsigned char)source[i] == (unsigned char)c) {
                expected_ptr = source + i;
            }
            if (source[i] == '\0') {
                break;
            }
        }
        actual_ptr = strrchr(source, c);
        if (pointer_offset(source, actual_ptr) != pointer_offset(source, expected_ptr)) {
            return 46;
        }

        actual_ptr = strstr(source, other);
        expected_ptr = model_strstr(source, other);
        if (pointer_offset(source, actual_ptr) != pointer_offset(source, expected_ptr)) {
            return 47;
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 48;
    }
    return 0;
}
