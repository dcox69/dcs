#include "ml_datetime.h"
#include "ml_timezone.h"

#ifndef ML_DT_NO_GETTIMEOFDAY
/* On ESP-IDF and most POSIX-like libs this is available. */
#include <sys/time.h>
#endif

#ifdef ML_PLATFORM_ESP
/* On ESP-IDF this is available. */
#include <esp_timer.h>
#endif

#include <stdlib.h>
#include <string.h>


/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

static bool ml_dt_is_leap_year_internal(int32_t year)
{
    if ((year % 4) != 0)
    {
        return false;
    }
    if ((year % 100) != 0)
    {
        return true;
    }
    return (year % 400) == 0;
}

static int ml_dt_days_in_month_internal(int32_t year, int month0)
{
    static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month0 < 0 || month0 > 11)
    {
        return 0;
    }

    int dim = days_in_month[month0];
    if (month0 == 1 && ml_dt_is_leap_year_internal(year))
    {
        dim = 29;
    }
    return dim;
}

/*
 * Convert civil date to number of days since Unix epoch 1970-01-01.
 * Based on a well-known algorithm for Gregorian calendars.
 */
static int64_t ml_dt_days_from_civil(int64_t y, unsigned m, unsigned d)
{
    /* m: 1-12, d: 1-31 */
    y -= (m <= 2);
    const int64_t  era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400); /* [0, 399] */
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; /* [0, 146096] */
    return era * 146097 + (int64_t)doe - 719468;
}

/*
 * Inverse of ml_dt_days_from_civil.
 * Convert days since Unix epoch 1970-01-01 to civil date.
 */
static void ml_dt_civil_from_days(int64_t z, int32_t *year, uint8_t *month0, uint8_t *day)
{
    z += 719468;
    const int64_t  era       = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe       = (unsigned)(z - era * 146097); /* [0, 146096] */
    const unsigned yoe       = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; /* [0, 399] */
    const int64_t  y         = (int64_t)yoe + era * 400;
    const unsigned doy       = doe - (365 * yoe + yoe / 4 - yoe / 100); /* [0, 365] */
    const unsigned mp        = (5 * doy + 2) / 153;                     /* [0, 11] */
    const unsigned d         = doy - (153 * mp + 2) / 5 + 1;            /* [1, 31] */
    const unsigned m         = mp + (mp < 10 ? 3 : -9);                 /* [1, 12] */

    int32_t        y_out     = (int32_t)(y + (m <= 2));
    uint8_t        month_out = (uint8_t)(m - 1); /* convert to 0-11 */

    *year                    = y_out;
    *month0                  = month_out;
    *day                     = (uint8_t)d;
}

/* Compute day-of-week 0-6, Sunday = 0, from day count since 1970-01-01. */
static uint8_t ml_dt_wday_from_days(int64_t days_since_epoch)
{
    /* 1970-01-01 was a Thursday (4), Sunday = 0 => offset of 4. */
    int64_t wday = days_since_epoch + 4;
    int64_t mod  = wday % 7;
    if (mod < 0)
    {
        mod += 7;
    }
    return (uint8_t)mod;
}

/* Convert ml_datetime_t to day count + seconds-of-day. */
static bool ml_dt_to_day_and_seconds(const ml_datetime_t *dt,
                                     int64_t          *out_days,
                                     int32_t          *out_sec_of_day)
{
    if (!dt || !out_days || !out_sec_of_day)
    {
        return false;
    }

    int32_t year   = dt->year;
    int     month0 = dt->month;
    int     day    = dt->day;
    int     hour   = dt->hour;
    int     minute = dt->minute;
    int     second = dt->second;

    if (month0 < 0 || month0 > 11)
    {
        return false;
    }

    /* Basic range checks for time-of-day */
    if (hour < 0 || hour > 23)
    {
        return false;
    }
    if (minute < 0 || minute > 59)
    {
        return false;
    }
    if (second < 0 || second > 61)
    {
        return false;
    }

    /* Days since epoch for the date part. Our algorithm expects month 1-12. */
    int64_t days    = ml_dt_days_from_civil((int64_t)year, (unsigned)(month0 + 1), (unsigned)day);

    int32_t sod     = hour * 3600 + minute * 60 + (second > 59 ? 59 : second);
    *out_days       = days;
    *out_sec_of_day = sod;
    return true;
}

/* Convert day count + seconds-of-day back to ml_datetime_t. */
static bool ml_dt_from_day_and_seconds(int64_t days, int32_t sec_of_day, ml_datetime_t *out)
{
    if (!out)
    {
        return false;
    }

    /* Normalize seconds-of-day into range [0, 86400) adjusting days accordingly. */
    if (sec_of_day < 0 || sec_of_day >= 86400)
    {
        int64_t delta_days = sec_of_day / 86400;
        int32_t rem        = sec_of_day % 86400;
        if (rem < 0)
        {
            rem += 86400;
            delta_days -= 1;
        }
        days += delta_days;
        sec_of_day = rem;
    }

    int32_t year;
    uint8_t month0;
    uint8_t mday;
    ml_dt_civil_from_days(days, &year, &month0, &mday);

    /* Check year fits into int16_t. */
    if (year < INT16_MIN || year > INT16_MAX)
    {
        return false;
    }

    uint8_t hour   = (uint8_t)(sec_of_day / 3600);
    uint8_t minute = (uint8_t)((sec_of_day / 60) % 60);
    uint8_t second = (uint8_t)(sec_of_day % 60);

    out->year      = (int16_t)year;
    out->month     = month0;
    out->day       = mday;
    out->wday      = ml_dt_wday_from_days(days);
    out->hour      = hour;
    out->minute    = minute;
    out->second    = second;
    return true;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

bool ml_dt_is_valid(const ml_datetime_t *dt)
{
    if (!dt)
    {
        return false;
    }

    int32_t year   = dt->year;
    int     month0 = dt->month;
    int     day    = dt->day;
    int     hour   = dt->hour;
    int     minute = dt->minute;
    int     second = dt->second;

    if (month0 < 0 || month0 > 11)
    {
        return false;
    }

    int dim = ml_dt_days_in_month_internal(year, month0);
    if (dim <= 0)
    {
        return false;
    }
    if (day < 1 || day > dim)
    {
        return false;
    }

    if (hour < 0 || hour > 23)
    {
        return false;
    }
    if (minute < 0 || minute > 59)
    {
        return false;
    }
    if (second < 0 || second > 61)
    {
        return false;
    }

    /* wday is not strictly validated; 0-6 is recommended. */
    return true;
}

bool ml_dt_normalize(ml_datetime_t *dt)
{
    if (!dt)
    {
        return false;
    }

    /* Convert to day + seconds, then back: this will normalize. */
    int64_t days;
    int32_t sec_of_day;
    if (!ml_dt_to_day_and_seconds(dt, &days, &sec_of_day))
    {
        return false;
    }

    return ml_dt_from_day_and_seconds(days, sec_of_day, dt);
}

/**
 * @brief Check if two datetimes are equal.
 *
 * @param dt1 First datetime
 * @param dt2 Second datetime
 * @return true if equal, false otherwise
 */
bool ml_dt_is_equal(const ml_datetime_t *dt1, const ml_datetime_t *dt2)
{
    if (!dt1 || !dt2)
    {
        return false;
    }

    int64_t days1, days2;
    int32_t sod1, sod2;
    if (!ml_dt_to_day_and_seconds(dt1, &days1, &sod1))
    {
        return false;
    }
    if (!ml_dt_to_day_and_seconds(dt2, &days2, &sod2))
    {
        return false;
    }

    return (days1 == days2) && (sod1 == sod2);
}

/**
 * @brief Check if datetime1 is before datetime2.
 *
 * @param dt1 First datetime
 * @param dt2 Second datetime
 * @return true if dt1 is before dt2
 */
bool ml_dt_is_before(const ml_datetime_t *dt1, const ml_datetime_t *dt2)
{
    if (!dt1 || !dt2)
    {
        return false;
    }
    return ml_dt_compare(dt1, dt2) < 0;
}

/**
 * @brief Check if datetime1 is after datetime2.
 *
 * @param dt1 First datetime
 * @param dt2 Second datetime
 * @return true if dt1 is after dt2
 */
bool ml_dt_is_after(const ml_datetime_t *dt1, const ml_datetime_t *dt2)
{
    if (!dt1 || !dt2)
    {
        return false;
    }
    return ml_dt_compare(dt1, dt2) > 0;
}

int ml_dt_compare(const ml_datetime_t *a, const ml_datetime_t *b)
{
    if (!a || !b)
    {
        return 0;
    }

    int64_t days_a, days_b;
    int32_t sod_a, sod_b;
    if (!ml_dt_to_day_and_seconds(a, &days_a, &sod_a))
    {
        return 0;
    }
    if (!ml_dt_to_day_and_seconds(b, &days_b, &sod_b))
    {
        return 0;
    }

    if (days_a < days_b)
    {
        return -1;
    }
    else if (days_a > days_b)
    {
        return 1;
    }
    else if (sod_a < sod_b)
    {
        return -1;
    }
    else if (sod_a > sod_b)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

ml_datetime_span_t ml_dt_diff(const ml_datetime_t *a, const ml_datetime_t *b)
{
    ml_datetime_span_t span;
    span.sign    = 0;
    span.days    = 0;
    span.hours   = 0;
    span.minutes = 0;
    span.seconds = 0;

    if (!a || !b)
    {
        return span;
    }

    int64_t days_a, days_b;
    int32_t sod_a, sod_b;
    if (!ml_dt_to_day_and_seconds(a, &days_a, &sod_a))
    {
        return span;
    }
    if (!ml_dt_to_day_and_seconds(b, &days_b, &sod_b))
    {
        return span;
    }

    int64_t total_sec_a = days_a * 86400 + sod_a;
    int64_t total_sec_b = days_b * 86400 + sod_b;
    int64_t diff        = total_sec_a - total_sec_b;

    if (diff == 0)
    {
        span.sign = 0;
        return span;
    }

    span.sign = (diff > 0) ? 1 : -1;
    if (diff < 0)
    {
        diff = -diff;
    }

    span.days   = (int32_t)(diff / 86400);
    int64_t rem = diff % 86400;
    span.hours  = (int8_t)(rem / 3600);
    rem %= 3600;
    span.minutes = (int8_t)(rem / 60);
    rem %= 60;
    span.seconds = (int8_t)rem;

    return span;
}

int64_t ml_dt_span_total_seconds(const ml_datetime_span_t *span)
{
    if (!span)
    {
        return 0;
    }

    int64_t sec = 0;
    sec += (int64_t)span->days * 86400;
    sec += (int64_t)span->hours * 3600;
    sec += (int64_t)span->minutes * 60;
    sec += (int64_t)span->seconds;
    if (span->sign < 0)
    {
        sec = -sec;
    }
    return sec;
}

int64_t ml_dt_to_unix_seconds_utc(const ml_datetime_t *dt)
{
    int64_t days;
    int32_t sod;
    if (!ml_dt_to_day_and_seconds(dt, &days, &sod))
    {
        return 0;
    }
    return days * 86400 + sod;
}

bool ml_dt_from_unix_seconds_utc(int64_t unix_sec, ml_datetime_t *out)
{
    if (!out)
    {
        return false;
    }

    int64_t days = unix_sec / 86400;
    int32_t sod  = (int32_t)(unix_sec % 86400);
    return ml_dt_from_day_and_seconds(days, sod, out);
}

/* ----- Arithmetic -------------------------------------------------------- */

bool ml_dt_add_seconds(const ml_datetime_t *in, int64_t seconds, ml_datetime_t *out)
{
    if (!in || !out)
    {
        return false;
    }

    int64_t base = ml_dt_to_unix_seconds_utc(in);
    int64_t sum  = base + seconds;
    return ml_dt_from_unix_seconds_utc(sum, out);
}

bool ml_dt_add_minutes(const ml_datetime_t *in, int64_t minutes, ml_datetime_t *out)
{
    return ml_dt_add_seconds(in, minutes * 60, out);
}

bool ml_dt_add_hours(const ml_datetime_t *in, int64_t hours, ml_datetime_t *out)
{
    return ml_dt_add_seconds(in, hours * 3600, out);
}

bool ml_dt_add_days(const ml_datetime_t *in, int64_t days, ml_datetime_t *out)
{
    if (!in || !out)
    {
        return false;
    }

    int64_t days_base;
    int32_t sod;
    if (!ml_dt_to_day_and_seconds(in, &days_base, &sod))
    {
        return false;
    }

    int64_t days_sum = days_base + days;
    return ml_dt_from_day_and_seconds(days_sum, sod, out);
}

bool ml_dt_add_months(const ml_datetime_t *in, int32_t months, ml_datetime_t *out)
{
    if (!in || !out)
    {
        return false;
    }

    int32_t year         = in->year;
    int32_t month0       = in->month;
    int32_t day          = in->day;

    int64_t total_months = (int64_t)year * 12 + month0 + months;
    int32_t new_year     = (int32_t)(total_months / 12);
    int32_t new_month0   = (int32_t)(total_months % 12);
    if (new_month0 < 0)
    {
        new_month0 += 12;
        new_year -= 1;
    }

    if (new_year < INT16_MIN || new_year > INT16_MAX)
    {
        return false;
    }

    int dim = ml_dt_days_in_month_internal(new_year, new_month0);
    if (dim <= 0)
    {
        return false;
    }
    if (day > dim)
    {
        day = dim;
    }

    out->year   = (int16_t)new_year;
    out->month  = (uint8_t)new_month0;
    out->day    = (uint8_t)day;
    out->hour   = in->hour;
    out->minute = in->minute;
    out->second = in->second;

    /* Recompute day-of-week. */
    int64_t days =
        ml_dt_days_from_civil((int64_t)new_year, (unsigned)(new_month0 + 1), (unsigned)day);
    out->wday = ml_dt_wday_from_days(days);
    return true;
}

bool ml_dt_add_years(const ml_datetime_t *in, int32_t years, ml_datetime_t *out)
{
    if (!in || !out)
    {
        return false;
    }

    int32_t year   = (int32_t)in->year + years;
    int32_t month0 = in->month;
    int32_t day    = in->day;

    if (year < INT16_MIN || year > INT16_MAX)
    {
        return false;
    }

    int dim = ml_dt_days_in_month_internal(year, month0);
    if (dim <= 0)
    {
        return false;
    }
    if (day > dim)
    {
        day = dim;
    }

    out->year    = (int16_t)year;
    out->month   = (uint8_t)month0;
    out->day     = (uint8_t)day;
    out->hour    = in->hour;
    out->minute  = in->minute;
    out->second  = in->second;

    int64_t days = ml_dt_days_from_civil((int64_t)year, (unsigned)(month0 + 1), (unsigned)day);
    out->wday    = ml_dt_wday_from_days(days);
    return true;
}

bool ml_dt_tick_one_second(ml_datetime_t *dt)
{
    if (!dt)
    {
        return false;
    }

    ml_datetime_t tmp;
    if (!ml_dt_add_seconds(dt, 1, &tmp))
    {
        return false;
    }
    *dt = tmp;
    return true;
}

bool ml_dt_tick_minus_one_second(ml_datetime_t *dt)
{
    if (!dt)
    {
        return false;
    }

    ml_datetime_t tmp;
    if (!ml_dt_add_seconds(dt, -1, &tmp))
    {
        return false;
    }
    *dt = tmp;
    return true;
}

/* ----- Uptime functions -------------------------------------------------- */

uint64_t ml_dt_get_uptime_seconds(void)
{
#if defined(ML_PLATFORM_ESP)
    return (uint64_t)(esp_timer_get_time() / 1000000ULL);
#elif defined(ML_PLATFORM_PICO)
    return time_us_64() / 1000000ULL;
#else
    #error "No platform defined !"
#endif    
}

ml_uptime_t ml_dt_get_uptime(void)
{
    uint64_t secs = ml_dt_get_uptime_seconds();
    ml_uptime_t uptime;
    uptime.days  = secs / 86400;
    uptime.hours = (secs % 86400) / 3600;
    uptime.minutes  = (secs % 3600) / 60;
    uptime.seconds  = secs % 60;
    return uptime;
}

/* ----- System time ------------------------------------------------------- */

bool ml_dt_now_utc(ml_datetime_t *out)
{
    if (!out)
    {
        return false;
    }

    struct tm tm_utc;
    memset(&tm_utc, 0, sizeof(tm_utc));

#ifndef ML_DT_NO_GETTIMEOFDAY
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        return false;
    }
    time_t sec = (time_t)tv.tv_sec;
    /* Thread-safe UTC conversion if available. */
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(ESP_PLATFORM)
    if (gmtime_r(&sec, &tm_utc) == NULL)
    {
        return false;
    }
#else
    struct tm *ptm = gmtime(&sec);
    if (!ptm)
    {
        return false;
    }
    tm_utc = *ptm;
#endif
#else
    time_t sec = time(NULL);
    if (sec == (time_t)-1)
    {
        return false;
    }
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(ESP_PLATFORM)
    if (gmtime_r(&sec, &tm_utc) == NULL)
    {
        return false;
    }
#else
    struct tm *ptm = gmtime(&sec);
    if (!ptm)
    {
        return false;
    }
    tm_utc = *ptm;
#endif
#endif

    ml_dt_from_tm_utc(&tm_utc, out);
    return true;
}

bool ml_dt_now_local(ml_datetime_t *out)
{
    if (!out)
    {
        return false;
    }

    struct tm tm_local;
    memset(&tm_local, 0, sizeof(tm_local));

#ifndef ML_DT_NO_GETTIMEOFDAY
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        return false;
    }
    time_t sec = (time_t)tv.tv_sec;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(ESP_PLATFORM)
    if (localtime_r(&sec, &tm_local) == NULL)
    {
        return false;
    }
#else
    struct tm *ptm = localtime(&sec);
    if (!ptm)
    {
        return false;
    }
    tm_local = *ptm;
#endif
#else
    time_t sec = time(NULL);
    if (sec == (time_t)-1)
    {
        return false;
    }
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(ESP_PLATFORM)
    if (localtime_r(&sec, &tm_local) == NULL)
    {
        return false;
    }
#else
    struct tm *ptm = localtime(&sec);
    if (!ptm)
    {
        return false;
    }
    tm_local = *ptm;
#endif
#endif

    ml_dt_from_tm_local(&tm_local, out);
    return true;
}

bool ml_dt_set_timezone_region(const char *tz_region)
{
    const char *posix_tz = posix_tz_from_zone(tz_region);
    if (!posix_tz)
    {
        return false;
    }
    setenv("TZ", posix_tz, 1);
    tzset();
    return true;
}

/* ----- utility functions  ---------------------------------------------- */

/**
 * @brief Get the number of days in a specific year.
 *
 * @param year The year
 * @return 365 or 366
 */
uint16_t ml_dt_days_in_year(int16_t year)
{
    return ml_dt_is_leap_year_internal(year) ? 366 : 365;
}

bool ml_dt_is_leap_year(int16_t year)
{
    return ml_dt_is_leap_year_internal(year);
}

/* ----- struct tm bridge helpers ----------------------------------------- */

void ml_dt_from_tm_utc(const struct tm *tm_src, ml_datetime_t *out)
{
    if (!tm_src || !out)
    {
        return;
    }

    out->year   = (int16_t)(tm_src->tm_year + 1900);
    out->month  = (uint8_t)tm_src->tm_mon;
    out->day    = (uint8_t)tm_src->tm_mday;
    out->wday   = (uint8_t)tm_src->tm_wday;
    out->hour   = (uint8_t)tm_src->tm_hour;
    out->minute = (uint8_t)tm_src->tm_min;
    out->second = (uint8_t)tm_src->tm_sec;
}

void ml_dt_from_tm_local(const struct tm *tm_src, ml_datetime_t *out)
{
    /* Currently identical to UTC version because struct tm carries local vs UTC
     * semantics via how it was filled (localtime_r vs gmtime_r).
     * This function exists only for API symmetry.
     */
    ml_dt_from_tm_utc(tm_src, out);
}

void ml_dt_to_tm_utc(const ml_datetime_t *dt, struct tm *tm_dst)
{
    if (!dt || !tm_dst)
    {
        return;
    }

    memset(tm_dst, 0, sizeof(*tm_dst));
    tm_dst->tm_year = dt->year - 1900;
    tm_dst->tm_mon  = dt->month;
    tm_dst->tm_mday = dt->day;
    tm_dst->tm_wday = dt->wday;
    tm_dst->tm_hour = dt->hour;
    tm_dst->tm_min  = dt->minute;
    tm_dst->tm_sec  = dt->second;
}

void ml_dt_to_tm_local(const ml_datetime_t *dt, struct tm *tm_dst)
{
    /* Same mapping as UTC; the caller decides how to interpret struct tm. */
    ml_dt_to_tm_utc(dt, tm_dst);
}