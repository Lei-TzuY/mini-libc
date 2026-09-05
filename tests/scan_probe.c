#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char path[] = "build/scan_probe.tmp";
    static const char file_input[] =
        "-5 -30000 -5000000000 -9000000000 250 60000 4000000000 "
        "9000000000 word Z % skip 12X 0x2a 077 42 17 ff 0X10 "
        "abcXYZ42_tail! @ ]- 0x9 33";
    static const char ok[] = "scan-ok\n";
    signed char signed_byte = 0;
    short signed_short = 0;
    long signed_long = 0;
    long long signed_long_long = 0;
    unsigned char unsigned_byte = 0;
    unsigned short unsigned_short = 0;
    unsigned long unsigned_long = 0;
    unsigned long long unsigned_long_long = 0;
    char word[5];
    char lower[4];
    char tail[6];
    char literal_set[3];
    char mismatch[2] = {'?', '\0'};
    char input_word[6];
    char character = '\0';
    char input_character = '\0';
    FILE *stream;
    int first = 0;
    int second = 777;
    int auto_hex = 0;
    int auto_oct = 0;
    int auto_dec = 0;
    int stdin_value = 0;
    unsigned int octal = 0;
    unsigned int hex = 0;
    unsigned int hex_upper = 0;

    stream = fopen(path, "w+");
    if (stream == (FILE *)0 ||
        fwrite(file_input, 1, sizeof(file_input) - 1U, stream) !=
            sizeof(file_input) - 1U ||
        fseek(stream, 0L, SEEK_SET) != 0) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 1;
    }

    errno = EIO;
    if (fscanf(stream, "%hhd %hd %ld %lld %hhu %hu %lu %llu",
               &signed_byte, &signed_short, &signed_long, &signed_long_long,
               &unsigned_byte, &unsigned_short, &unsigned_long,
               &unsigned_long_long) != 8 ||
        signed_byte != (signed char)-5 || signed_short != (short)-30000 ||
        signed_long != -5000000000L || signed_long_long != -9000000000LL ||
        unsigned_byte != (unsigned char)250U ||
        unsigned_short != (unsigned short)60000U ||
        unsigned_long != 4000000000UL ||
        unsigned_long_long != 9000000000ULL || errno != EIO) {
        fclose(stream);
        return 2;
    }

    if (fscanf(stream, " %4s %c %% %*s", word, &character) != 2 ||
        strcmp(word, "word") != 0 || character != 'Z') {
        fclose(stream);
        return 3;
    }

    if (fscanf(stream, "%d%d", &first, &second) != 1 || first != 12 ||
        second != 777 || fgetc(stream) != 'X') {
        fclose(stream);
        return 4;
    }

    if (fscanf(stream,
               " %i %i %i %o %x %X %3[a-z]%*[A-Z0-9]%5[^!]%*1[!]",
               &auto_hex, &auto_oct, &auto_dec, &octal, &hex, &hex_upper,
               lower, tail) != 8 ||
        auto_hex != 42 || auto_oct != 63 || auto_dec != 42 || octal != 15U ||
        hex != 255U || hex_upper != 16U || strcmp(lower, "abc") != 0 ||
        strcmp(tail, "_tail") != 0) {
        fclose(stream);
        return 5;
    }

    if (fscanf(stream, " %1[a-z]", mismatch) != 0 || mismatch[0] != '?' ||
        fgetc(stream) != '@') {
        fclose(stream);
        return 6;
    }
    if (fscanf(stream, " %2[]-]", literal_set) != 1 ||
        literal_set[0] != ']' || literal_set[1] != '-' ||
        literal_set[2] != '\0') {
        fclose(stream);
        return 7;
    }

    first = 91;
    if (fscanf(stream, " %2i", &first) != 0 || first != 91 ||
        fgetc(stream) != '9') {
        fclose(stream);
        return 8;
    }

    first = 0;
    if (fscanf(stream, "%d", &first) != 1 || first != 33 || !feof(stream) ||
        ferror(stream)) {
        fclose(stream);
        return 9;
    }
    first = 99;
    if (fscanf(stream, "%d", &first) != EOF || first != 99) {
        fclose(stream);
        return 10;
    }

    if (fclose(stream) != 0) {
        return 11;
    }

    errno = ERANGE;
    if (scanf("%d %5s %c", &stdin_value, input_word, &input_character) != 3 ||
        stdin_value != 41 || strcmp(input_word, "token") != 0 ||
        input_character != 'Q' || errno != ERANGE) {
        return 12;
    }
    if (scanf("%d", &stdin_value) != EOF) {
        return 13;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) != (long)(sizeof(ok) - 1U)) {
        return 14;
    }
    return 0;
}
