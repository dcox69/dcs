#ifndef ML_TIMEZONE_H
#define ML_TIMEZONE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *zone_name;
    const char *posix_tz;
} posix_tz_entry_t;

const char *posix_tz_from_zone(const char *zone_name);
const posix_tz_entry_t *posix_tz_find_entry(const char *zone_name);

#ifdef __cplusplus
}
#endif

#endif