#ifndef ML_DATETIME_H
#define ML_DATETIME_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <ml_platform.h>

#if defined(ML_PLATFORM_ESP) || defined(ML_PLATFORM_PICO)
#undef ML_DT_NO_GETTIMEOFDAY
#endif

/*
 * Application-level date/time structure.
 * - year: full year in signed 16-bit range.
 * - month: 0-11, January = 0.
 * - day: 1-31.
 * - wday: 0-6, Sunday = 0.
 * - hour: 0-23.
 * - minute: 0-59.
 * - second: 0-61 (to allow leap seconds).
 */
typedef struct {
    int16_t year;      /* full year: -32768 .. 32767 */

    uint8_t month;     /* month: 0-11 / January = 0 */
    uint8_t day;       /* day of month: 1-31 */
    uint8_t wday;      /* day of week: 0-6 / 0 = Sunday */

    uint8_t hour;      /* hour: 0-23 */
    uint8_t minute;    /* minutes: 0-59 */
    uint8_t second;    /* seconds: 0-61 */
} ml_datetime_t;

/*
 * Signed span between two date-times expressed in days + h:m:s.
 * This is not a civil date, but a duration.
 * sign: -1, 0, +1.
 */
typedef struct {
    int8_t  sign;      /* -1, 0, +1 */
    int32_t days;      /* absolute number of days in the span */
    int8_t  hours;     /* 0-23 */
    int8_t  minutes;   /* 0-59 */
    int8_t  seconds;   /* 0-59 (leap seconds ignored in spans) */
} ml_datetime_span_t;

/* Uptime struct */ 
typedef struct {
    int32_t days;      /* absolute number of days */
    int8_t  hours;     /* 0-23 */
    int8_t  minutes;   /* 0-59 */
    int8_t  seconds;   /* 0-59 */
} ml_uptime_t;

/* ----- Validation / basic helpers -------------------------------------- */

/* Returns true if all fields are within valid ranges for a civil date/time. */
bool ml_dt_is_valid(const ml_datetime_t *dt);

/*
 * Normalize fields (e.g. second >= 60, month >= 12, day overflow, etc.)
 * so that dt becomes a valid civil date/time.
 * Returns false if normalization fails due to year overflow.
 */
bool ml_dt_normalize(ml_datetime_t *dt);

/* Returns true if dt1 and dt2 represent the same instant (UTC). */
bool ml_dt_is_equal(const ml_datetime_t *dt1, const ml_datetime_t *dt2);

/* Compare two date-times (UTC order): <0 if a < b, 0 if equal, >0 if a > b. */
int ml_dt_compare(const ml_datetime_t *a, const ml_datetime_t *b);

/* Check if datetime1 is before datetime2. */
bool ml_dt_is_before(const ml_datetime_t *dt1, const ml_datetime_t *dt2);

/* Check if datetime1 is after datetime2. */
bool ml_dt_is_after(const ml_datetime_t *dt1, const ml_datetime_t *dt2);

/*
 * Compute signed difference between two date-times:
 *   span = a - b
 * span.sign tells the direction, while days/hours/minutes/seconds are absolute.
 */
ml_datetime_span_t ml_dt_diff(const ml_datetime_t *a, const ml_datetime_t *b);

/*
 * Convert a span back to total seconds (signed).
 * Note: leap seconds are ignored in spans.
 */
int64_t ml_dt_span_total_seconds(const ml_datetime_span_t *span);

/* ----- Uptime functions (time since the system is running) -------------- */ 

ml_uptime_t ml_dt_get_uptime(void);

uint64_t ml_dt_get_uptime_seconds(void);

/* ----- Misc utility functions ------------------------------------------- */

/**
 * @brief Get the number of days in a specific year.
 * 
 * @param year The year
 * @return 365 or 366
 */
uint16_t ml_dt_days_in_year(int16_t year);

bool ml_dt_is_leap_year(int16_t year);


/* ----- Unix epoch conversions (UTC) ------------------------------------- */

/*
 * Convert datetime_t (interpreted as UTC) to Unix seconds since 1970-01-01T00:00:00Z.
 * Returns the signed 64-bit Unix timestamp.
 */
int64_t ml_dt_to_unix_seconds_utc(const ml_datetime_t *dt);

/*
 * Convert Unix seconds since 1970-01-01T00:00:00Z to datetime_t (UTC).
 * Returns false if conversion overflows datetime_t year range.
 */
bool ml_dt_from_unix_seconds_utc(int64_t unix_sec, ml_datetime_t *out);

/* ----- Arithmetic on date/time ----------------------------------------- */

/* Add a signed number of seconds to a datetime (interpreted as UTC). */
bool ml_dt_add_seconds(const ml_datetime_t *in, int64_t seconds, ml_datetime_t *out);

/* Add a signed number of minutes to a datetime (UTC). */
bool ml_dt_add_minutes(const ml_datetime_t *in, int64_t minutes, ml_datetime_t *out);

/* Add a signed number of hours to a datetime (UTC). */
bool ml_dt_add_hours(const ml_datetime_t *in, int64_t hours, ml_datetime_t *out);

/* Add a signed number of days to a datetime (UTC). */
bool ml_dt_add_days(const ml_datetime_t *in, int64_t days, ml_datetime_t *out);

/*
 * Add a signed number of months to a datetime (civil arithmetic).
 * If the resulting day would be past the end of the target month,
 * it is clamped to the last valid day of that month.
 */
bool ml_dt_add_months(const ml_datetime_t *in, int32_t months, ml_datetime_t *out);

/*
 * Add a signed number of years to a datetime (civil arithmetic).
 * If the resulting day (e.g. Feb 29 on non-leap year) is invalid,
 * it is clamped to the last valid day of that month.
 */
bool ml_dt_add_years(const ml_datetime_t *in, int32_t years, ml_datetime_t *out);

/*
 * Increment a datetime by exactly one second (UTC).
 * This can be used as a cheap "tick" helper.
 */
bool ml_dt_tick_one_second(ml_datetime_t *dt);

/*
 * Decrement a datetime by exactly one second (UTC).
 */
bool ml_dt_tick_minus_one_second(ml_datetime_t *dt);

/* ----- System time access ----------------------------------------------- */

/*
 * Get current UTC time into datetime_t.
 *
 * On POSIX-like systems (including ESP-IDF, Pico + newlib with sys/time),
 * this uses gettimeofday() + gmtime_r().
 * If ML_DT_NO_GETTIMEOFDAY is defined, it falls back to time() + gmtime_r().
 */
bool ml_dt_now_utc(ml_datetime_t *out);

/*
 * Get current local time into datetime_t.
 *
 * On POSIX-like systems, this uses gettimeofday() + localtime_r()
 * or time() + localtime_r() depending on configuration.
 * You must configure TZ / tzset() in your application if you want
 * proper local time handling and DST.
 */
bool ml_dt_now_local(ml_datetime_t *out);

/* define the timezone, using the IANA model (eg. "America/Chicago")  */ 
bool ml_dt_set_timezone_region(const char *tz_region);

/* ----- struct tm bridge helpers ---------------------------------------- */

/* Fill datetime from struct tm interpreted as UTC. */
void ml_dt_from_tm_utc(const struct tm *tm_src, ml_datetime_t *out);

/* Fill datetime from struct tm interpreted as local time. */
void ml_dt_from_tm_local(const struct tm *tm_src, ml_datetime_t *out);

/* Fill struct tm (UTC fields) from datetime. */
void ml_dt_to_tm_utc(const ml_datetime_t *dt, struct tm *tm_dst);

/* Fill struct tm (local fields) from datetime. */
void ml_dt_to_tm_local(const ml_datetime_t *dt, struct tm *tm_dst);

#endif /* ML_DATETIME_H */