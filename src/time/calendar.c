#include <errno.h>
#include <stddef.h>
#include <time.h>

#define MINI_SECONDS_PER_DAY 86400LL
#define MINI_INT_MAX ((int)((~0U) >> 1))
#define MINI_INT_MIN (-MINI_INT_MAX - 1)

static _Thread_local struct tm mini_calendar_tm;
static _Thread_local char mini_asctime_buffer[26];

static const char *const mini_weekday_short[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char *const mini_weekday_long[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char *const mini_month_short[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char *const mini_month_long[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static long long floor_div(long long value, long long divisor)
{
    long long quotient = value / divisor;
    long long remainder = value % divisor;

    if (remainder < 0) {
        --quotient;
    }
    return quotient;
}

static long long floor_mod(long long value, long long divisor)
{
    long long remainder = value % divisor;

    if (remainder < 0) {
        remainder += divisor;
    }
    return remainder;
}

static int is_leap_year(long long year)
{
    return year % 4LL == 0LL &&
           (year % 100LL != 0LL || year % 400LL == 0LL);
}

static long long days_from_civil(long long year, unsigned int month,
                                 unsigned int day)
{
    long long era;
    unsigned int year_of_era;
    unsigned int day_of_year;
    unsigned int day_of_era;

    year -= month <= 2U;
    era = floor_div(year, 400LL);
    year_of_era = (unsigned int)(year - era * 400LL);
    day_of_year = (153U * (month > 2U ? month - 3U : month + 9U) + 2U) /
                  5U + day - 1U;
    day_of_era = year_of_era * 365U + year_of_era / 4U - year_of_era / 100U +
                 day_of_year;
    return era * 146097LL + (long long)day_of_era - 719468LL;
}

static void civil_from_days(long long days, long long *year,
                            unsigned int *month, unsigned int *day)
{
    long long z = days + 719468LL;
    long long era = floor_div(z, 146097LL);
    unsigned int day_of_era = (unsigned int)(z - era * 146097LL);
    unsigned int year_of_era =
        (day_of_era - day_of_era / 1460U + day_of_era / 36524U -
         day_of_era / 146096U) / 365U;
    long long y = (long long)year_of_era + era * 400LL;
    unsigned int day_of_year =
        day_of_era - (365U * year_of_era + year_of_era / 4U -
                      year_of_era / 100U);
    unsigned int month_prime = (5U * day_of_year + 2U) / 153U;
    unsigned int d = day_of_year - (153U * month_prime + 2U) / 5U + 1U;
    unsigned int m = month_prime < 10U ? month_prime + 3U : month_prime - 9U;

    y += m <= 2U;
    *year = y;
    *month = m;
    *day = d;
}

static int calendar_from_epoch(time_t timer, struct tm *out)
{
    long long seconds = (long long)timer;
    long long days = seconds / MINI_SECONDS_PER_DAY;
    long long second_of_day = seconds % MINI_SECONDS_PER_DAY;
    long long year;
    unsigned int month;
    unsigned int day;
    long long year_offset;
    long long year_start;

    if (second_of_day < 0) {
        --days;
        second_of_day += MINI_SECONDS_PER_DAY;
    }

    civil_from_days(days, &year, &month, &day);
    year_offset = year - 1900LL;
    if (year_offset < (long long)MINI_INT_MIN ||
        year_offset > (long long)MINI_INT_MAX) {
        errno = ERANGE;
        return 0;
    }

    year_start = days_from_civil(year, 1U, 1U);
    out->tm_sec = (int)(second_of_day % 60LL);
    out->tm_min = (int)((second_of_day / 60LL) % 60LL);
    out->tm_hour = (int)(second_of_day / 3600LL);
    out->tm_mday = (int)day;
    out->tm_mon = (int)month - 1;
    out->tm_year = (int)year_offset;
    out->tm_wday = (int)floor_mod(days + 4LL, 7LL);
    out->tm_yday = (int)(days - year_start);
    out->tm_isdst = 0;
    return 1;
}

struct tm *gmtime(const time_t *timer)
{
    if (timer == (const time_t *)0) {
        errno = EINVAL;
        return (struct tm *)0;
    }
    if (!calendar_from_epoch(*timer, &mini_calendar_tm)) {
        return (struct tm *)0;
    }
    return &mini_calendar_tm;
}

struct tm *localtime(const time_t *timer)
{
    /* This runtime deliberately defines the only local zone as UTC. */
    return gmtime(timer);
}

time_t mktime(struct tm *timeptr)
{
    long long year;
    long long month_index;
    long long month_years;
    unsigned int month;
    long long days;
    long long second_of_day;
    long long day_adjust;
    long long normalized_second;
    long long total;
    struct tm normalized;

    if (timeptr == (struct tm *)0) {
        errno = EINVAL;
        return (time_t)-1;
    }

    year = (long long)timeptr->tm_year + 1900LL;
    month_index = (long long)timeptr->tm_mon;
    month_years = floor_div(month_index, 12LL);
    year += month_years;
    month = (unsigned int)(month_index - month_years * 12LL) + 1U;

    days = days_from_civil(year, month, 1U) +
           ((long long)timeptr->tm_mday - 1LL);
    second_of_day = (long long)timeptr->tm_hour * 3600LL +
                    (long long)timeptr->tm_min * 60LL +
                    (long long)timeptr->tm_sec;
    day_adjust = floor_div(second_of_day, MINI_SECONDS_PER_DAY);
    normalized_second = second_of_day - day_adjust * MINI_SECONDS_PER_DAY;
    days += day_adjust;

    total = days * MINI_SECONDS_PER_DAY + normalized_second;
    if (!calendar_from_epoch((time_t)total, &normalized)) {
        return (time_t)-1;
    }
    *timeptr = normalized;
    return (time_t)total;
}

static int valid_text_tm(const struct tm *timeptr)
{
    long long year;

    if (timeptr == (const struct tm *)0 ||
        timeptr->tm_sec < 0 || timeptr->tm_sec > 60 ||
        timeptr->tm_min < 0 || timeptr->tm_min > 59 ||
        timeptr->tm_hour < 0 || timeptr->tm_hour > 23 ||
        timeptr->tm_mday < 1 || timeptr->tm_mday > 31 ||
        timeptr->tm_mon < 0 || timeptr->tm_mon > 11 ||
        timeptr->tm_wday < 0 || timeptr->tm_wday > 6 ||
        timeptr->tm_yday < 0 || timeptr->tm_yday > 365) {
        return 0;
    }
    year = (long long)timeptr->tm_year + 1900LL;
    return year >= 0LL && year <= 9999LL;
}

char *asctime(const struct tm *timeptr)
{
    int year;
    int day;
    const char *weekday;
    const char *month;

    if (!valid_text_tm(timeptr)) {
        errno = EINVAL;
        return (char *)0;
    }

    year = timeptr->tm_year + 1900;
    day = timeptr->tm_mday;
    weekday = mini_weekday_short[timeptr->tm_wday];
    month = mini_month_short[timeptr->tm_mon];

    mini_asctime_buffer[0] = weekday[0];
    mini_asctime_buffer[1] = weekday[1];
    mini_asctime_buffer[2] = weekday[2];
    mini_asctime_buffer[3] = ' ';
    mini_asctime_buffer[4] = month[0];
    mini_asctime_buffer[5] = month[1];
    mini_asctime_buffer[6] = month[2];
    mini_asctime_buffer[7] = ' ';
    mini_asctime_buffer[8] = day >= 10 ? (char)('0' + day / 10) : ' ';
    mini_asctime_buffer[9] = (char)('0' + day % 10);
    mini_asctime_buffer[10] = ' ';
    mini_asctime_buffer[11] = (char)('0' + timeptr->tm_hour / 10);
    mini_asctime_buffer[12] = (char)('0' + timeptr->tm_hour % 10);
    mini_asctime_buffer[13] = ':';
    mini_asctime_buffer[14] = (char)('0' + timeptr->tm_min / 10);
    mini_asctime_buffer[15] = (char)('0' + timeptr->tm_min % 10);
    mini_asctime_buffer[16] = ':';
    mini_asctime_buffer[17] = (char)('0' + timeptr->tm_sec / 10);
    mini_asctime_buffer[18] = (char)('0' + timeptr->tm_sec % 10);
    mini_asctime_buffer[19] = ' ';
    mini_asctime_buffer[20] = (char)('0' + (year / 1000) % 10);
    mini_asctime_buffer[21] = (char)('0' + (year / 100) % 10);
    mini_asctime_buffer[22] = (char)('0' + (year / 10) % 10);
    mini_asctime_buffer[23] = (char)('0' + year % 10);
    mini_asctime_buffer[24] = '\n';
    mini_asctime_buffer[25] = '\0';
    return mini_asctime_buffer;
}

char *ctime(const time_t *timer)
{
    struct tm *timeptr = localtime(timer);

    if (timeptr == (struct tm *)0) {
        return (char *)0;
    }
    return asctime(timeptr);
}

struct mini_time_writer {
    char *buffer;
    size_t capacity;
    size_t length;
    int failed;
};

static void writer_finish(struct mini_time_writer *writer)
{
    if (writer->capacity != 0U && writer->buffer != (char *)0) {
        size_t position = writer->length < writer->capacity ?
                              writer->length : writer->capacity - 1U;
        writer->buffer[position] = '\0';
    }
}

static void writer_char(struct mini_time_writer *writer, char value)
{
    if (writer->failed) {
        return;
    }
    if (writer->capacity == 0U || writer->length >= writer->capacity - 1U) {
        writer->failed = 1;
        return;
    }
    writer->buffer[writer->length++] = value;
    writer->buffer[writer->length] = '\0';
}

static void writer_text(struct mini_time_writer *writer, const char *text)
{
    while (*text != '\0' && !writer->failed) {
        writer_char(writer, *text++);
    }
}

static void writer_unsigned(struct mini_time_writer *writer, unsigned int value,
                            unsigned int minimum, char pad)
{
    char reverse[16];
    unsigned int count = 0U;
    unsigned int i;

    do {
        reverse[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);

    while (count < minimum) {
        writer_char(writer, pad);
        --minimum;
    }
    for (i = count; i != 0U; --i) {
        writer_char(writer, reverse[i - 1U]);
    }
}

static int weekday_for_date(long long year, unsigned int month,
                            unsigned int day)
{
    return (int)floor_mod(days_from_civil(year, month, day) + 4LL, 7LL);
}

static unsigned int iso_weeks_in_year(int year)
{
    int jan1 = weekday_for_date((long long)year, 1U, 1U);

    return jan1 == 4 || (jan1 == 3 && is_leap_year((long long)year)) ? 53U : 52U;
}

static void iso_week_fields(const struct tm *timeptr, int *iso_year,
                            unsigned int *iso_week)
{
    int year = timeptr->tm_year + 1900;
    int iso_wday = timeptr->tm_wday == 0 ? 7 : timeptr->tm_wday;
    int week = (timeptr->tm_yday + 1 - iso_wday + 10) / 7;

    if (week < 1) {
        --year;
        week = (int)iso_weeks_in_year(year);
    } else if (week > (int)iso_weeks_in_year(year)) {
        ++year;
        week = 1;
    }
    *iso_year = year;
    *iso_week = (unsigned int)week;
}

static void writer_format_code(struct mini_time_writer *writer, char code,
                               const struct tm *timeptr)
{
    int year = timeptr->tm_year + 1900;
    int hour12;
    int iso_year;
    unsigned int iso_week;
    unsigned int week;

    switch (code) {
    case '%': writer_char(writer, '%'); break;
    case 'a': writer_text(writer, mini_weekday_short[timeptr->tm_wday]); break;
    case 'A': writer_text(writer, mini_weekday_long[timeptr->tm_wday]); break;
    case 'b':
    case 'h': writer_text(writer, mini_month_short[timeptr->tm_mon]); break;
    case 'B': writer_text(writer, mini_month_long[timeptr->tm_mon]); break;
    case 'C': writer_unsigned(writer, (unsigned int)(year / 100), 2U, '0'); break;
    case 'd': writer_unsigned(writer, (unsigned int)timeptr->tm_mday, 2U, '0'); break;
    case 'e': writer_unsigned(writer, (unsigned int)timeptr->tm_mday, 2U, ' '); break;
    case 'H': writer_unsigned(writer, (unsigned int)timeptr->tm_hour, 2U, '0'); break;
    case 'I':
        hour12 = timeptr->tm_hour % 12;
        if (hour12 == 0) {
            hour12 = 12;
        }
        writer_unsigned(writer, (unsigned int)hour12, 2U, '0');
        break;
    case 'j': writer_unsigned(writer, (unsigned int)timeptr->tm_yday + 1U, 3U, '0'); break;
    case 'm': writer_unsigned(writer, (unsigned int)timeptr->tm_mon + 1U, 2U, '0'); break;
    case 'M': writer_unsigned(writer, (unsigned int)timeptr->tm_min, 2U, '0'); break;
    case 'n': writer_char(writer, '\n'); break;
    case 'p': writer_text(writer, timeptr->tm_hour < 12 ? "AM" : "PM"); break;
    case 'S': writer_unsigned(writer, (unsigned int)timeptr->tm_sec, 2U, '0'); break;
    case 't': writer_char(writer, '\t'); break;
    case 'u': writer_unsigned(writer, (unsigned int)(timeptr->tm_wday == 0 ? 7 : timeptr->tm_wday), 1U, '0'); break;
    case 'U':
        week = ((unsigned int)timeptr->tm_yday + 7U -
                (unsigned int)timeptr->tm_wday) / 7U;
        writer_unsigned(writer, week, 2U, '0');
        break;
    case 'V':
        iso_week_fields(timeptr, &iso_year, &iso_week);
        writer_unsigned(writer, iso_week, 2U, '0');
        break;
    case 'w': writer_unsigned(writer, (unsigned int)timeptr->tm_wday, 1U, '0'); break;
    case 'W': {
        unsigned int mon_wday = (unsigned int)((timeptr->tm_wday + 6) % 7);
        week = ((unsigned int)timeptr->tm_yday + 7U - mon_wday) / 7U;
        writer_unsigned(writer, week, 2U, '0');
        break;
    }
    case 'y': writer_unsigned(writer, (unsigned int)(year % 100), 2U, '0'); break;
    case 'Y': writer_unsigned(writer, (unsigned int)year, 4U, '0'); break;
    case 'g':
        iso_week_fields(timeptr, &iso_year, &iso_week);
        writer_unsigned(writer, (unsigned int)(iso_year % 100), 2U, '0');
        break;
    case 'G':
        iso_week_fields(timeptr, &iso_year, &iso_week);
        writer_unsigned(writer, (unsigned int)iso_year, 4U, '0');
        break;
    case 'z': writer_text(writer, "+0000"); break;
    case 'Z': writer_text(writer, "UTC"); break;
    case 'D':
        writer_format_code(writer, 'm', timeptr); writer_char(writer, '/');
        writer_format_code(writer, 'd', timeptr); writer_char(writer, '/');
        writer_format_code(writer, 'y', timeptr); break;
    case 'F':
        writer_format_code(writer, 'Y', timeptr); writer_char(writer, '-');
        writer_format_code(writer, 'm', timeptr); writer_char(writer, '-');
        writer_format_code(writer, 'd', timeptr); break;
    case 'R':
        writer_format_code(writer, 'H', timeptr); writer_char(writer, ':');
        writer_format_code(writer, 'M', timeptr); break;
    case 'T':
        writer_format_code(writer, 'H', timeptr); writer_char(writer, ':');
        writer_format_code(writer, 'M', timeptr); writer_char(writer, ':');
        writer_format_code(writer, 'S', timeptr); break;
    case 'r':
        writer_format_code(writer, 'I', timeptr); writer_char(writer, ':');
        writer_format_code(writer, 'M', timeptr); writer_char(writer, ':');
        writer_format_code(writer, 'S', timeptr); writer_char(writer, ' ');
        writer_format_code(writer, 'p', timeptr); break;
    case 'x': writer_format_code(writer, 'D', timeptr); break;
    case 'X': writer_format_code(writer, 'T', timeptr); break;
    case 'c':
        writer_format_code(writer, 'a', timeptr); writer_char(writer, ' ');
        writer_format_code(writer, 'b', timeptr); writer_char(writer, ' ');
        writer_format_code(writer, 'e', timeptr); writer_char(writer, ' ');
        writer_format_code(writer, 'T', timeptr); writer_char(writer, ' ');
        writer_format_code(writer, 'Y', timeptr); break;
    default:
        writer->failed = 2;
        break;
    }
}

size_t strftime(char *restrict s, size_t maxsize,
                const char *restrict format,
                const struct tm *restrict timeptr)
{
    struct mini_time_writer writer;
    const char *cursor;

    if (format == (const char *)0 || timeptr == (const struct tm *)0 ||
        (maxsize != 0U && s == (char *)0) || !valid_text_tm(timeptr)) {
        errno = EINVAL;
        return 0U;
    }

    writer.buffer = s;
    writer.capacity = maxsize;
    writer.length = 0U;
    writer.failed = 0;
    if (maxsize != 0U) {
        s[0] = '\0';
    }

    cursor = format;
    while (*cursor != '\0' && writer.failed == 0) {
        if (*cursor != '%') {
            writer_char(&writer, *cursor++);
            continue;
        }
        ++cursor;
        if (*cursor == 'E' || *cursor == 'O') {
            ++cursor;
        }
        if (*cursor == '\0') {
            writer.failed = 2;
            break;
        }
        writer_format_code(&writer, *cursor++, timeptr);
    }

    writer_finish(&writer);
    if (writer.failed != 0) {
        if (writer.failed == 2) {
            errno = EINVAL;
        }
        return 0U;
    }
    return writer.length;
}
