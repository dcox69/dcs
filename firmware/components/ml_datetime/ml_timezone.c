#include "ml_timezone.h"

#include <stddef.h>
#include <string.h>

static const posix_tz_entry_t g_africa_tz_table[] = {
    { "Africa/Abidjan", "GMT0" },
    { "Africa/Accra", "GMT0" },
    { "Africa/Addis_Ababa", "EAT-3" },
    { "Africa/Algiers", "CET-1" },
    { "Africa/Asmara", "EAT-3" },
    { "Africa/Bamako", "GMT0" },
    { "Africa/Bangui", "WAT-1" },
    { "Africa/Banjul", "GMT0" },
    { "Africa/Bissau", "GMT0" },
    { "Africa/Blantyre", "CAT-2" },
    { "Africa/Brazzaville", "WAT-1" },
    { "Africa/Bujumbura", "CAT-2" },
    { "Africa/Cairo", "EET-2EEST,M4.5.5/0,M10.5.4/24" },
    { "Africa/Casablanca", "<+01>-1" },
    { "Africa/Ceuta", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Africa/Conakry", "GMT0" },
    { "Africa/Dakar", "GMT0" },
    { "Africa/Dar_es_Salaam", "EAT-3" },
    { "Africa/Djibouti", "EAT-3" },
    { "Africa/Douala", "WAT-1" },
    { "Africa/El_Aaiun", "<+01>-1" },
    { "Africa/Freetown", "GMT0" },
    { "Africa/Gaborone", "CAT-2" },
    { "Africa/Harare", "CAT-2" },
    { "Africa/Johannesburg", "SAST-2" },
    { "Africa/Juba", "CAT-2" },
    { "Africa/Kampala", "EAT-3" },
    { "Africa/Khartoum", "CAT-2" },
    { "Africa/Kigali", "CAT-2" },
    { "Africa/Kinshasa", "WAT-1" },
    { "Africa/Lagos", "WAT-1" },
    { "Africa/Libreville", "WAT-1" },
    { "Africa/Lome", "GMT0" },
    { "Africa/Luanda", "WAT-1" },
    { "Africa/Lubumbashi", "CAT-2" },
    { "Africa/Lusaka", "CAT-2" },
    { "Africa/Malabo", "WAT-1" },
    { "Africa/Maputo", "CAT-2" },
    { "Africa/Maseru", "SAST-2" },
    { "Africa/Mbabane", "SAST-2" },
    { "Africa/Mogadishu", "EAT-3" },
    { "Africa/Monrovia", "GMT0" },
    { "Africa/Nairobi", "EAT-3" },
    { "Africa/Ndjamena", "WAT-1" },
    { "Africa/Niamey", "WAT-1" },
    { "Africa/Nouakchott", "GMT0" },
    { "Africa/Ouagadougou", "GMT0" },
    { "Africa/Porto-Novo", "WAT-1" },
    { "Africa/Sao_Tome", "GMT0" },
    { "Africa/Tripoli", "EET-2" },
    { "Africa/Tunis", "CET-1" },
    { "Africa/Windhoek", "CAT-2" },
};

static const size_t g_africa_tz_table_count =
    sizeof(g_africa_tz_table) / sizeof(g_africa_tz_table[0]);

static const posix_tz_entry_t g_america_tz_table[] = {
    { "America/Adak", "HST10HDT,M3.2.0,M11.1.0" },
    { "America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0" },
    { "America/Anguilla", "AST4" },
    { "America/Antigua", "AST4" },
    { "America/Araguaina", "<-03>3" },
    { "America/Argentina/Buenos_Aires", "<-03>3" },
    { "America/Argentina/Catamarca", "<-03>3" },
    { "America/Argentina/Cordoba", "<-03>3" },
    { "America/Argentina/Jujuy", "<-03>3" },
    { "America/Argentina/La_Rioja", "<-03>3" },
    { "America/Argentina/Mendoza", "<-03>3" },
    { "America/Argentina/Rio_Gallegos", "<-03>3" },
    { "America/Argentina/Salta", "<-03>3" },
    { "America/Argentina/San_Juan", "<-03>3" },
    { "America/Argentina/San_Luis", "<-03>3" },
    { "America/Argentina/Tucuman", "<-03>3" },
    { "America/Argentina/Ushuaia", "<-03>3" },
    { "America/Aruba", "AST4" },
    { "America/Asuncion", "<-03>3" },
    { "America/Atikokan", "EST5" },
    { "America/Bahia", "<-03>3" },
    { "America/Bahia_Banderas", "CST6" },
    { "America/Barbados", "AST4" },
    { "America/Belem", "<-03>3" },
    { "America/Belize", "CST6" },
    { "America/Blanc-Sablon", "AST4" },
    { "America/Boa_Vista", "<-04>4" },
    { "America/Bogota", "<-05>5" },
    { "America/Boise", "MST7MDT,M3.2.0,M11.1.0" },
    { "America/Cambridge_Bay", "MST7MDT,M3.2.0,M11.1.0" },
    { "America/Campo_Grande", "<-04>4" },
    { "America/Cancun", "EST5" },
    { "America/Caracas", "<-04>4" },
    { "America/Cayenne", "<-03>3" },
    { "America/Cayman", "EST5" },
    { "America/Chicago", "CST6CDT,M3.2.0,M11.1.0" },
    { "America/Chihuahua", "CST6" },
    { "America/Costa_Rica", "CST6" },
    { "America/Creston", "MST7" },
    { "America/Cuiaba", "<-04>4" },
    { "America/Curacao", "AST4" },
    { "America/Danmarkshavn", "GMT0" },
    { "America/Dawson", "MST7" },
    { "America/Dawson_Creek", "MST7" },
    { "America/Denver", "MST7MDT,M3.2.0,M11.1.0" },
    { "America/Detroit", "EST5EDT,M3.2.0,M11.1.0" },
    { "America/New_York", "EST5EDT,M3.2.0,M11.1.0" },
    { "America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0" },
    { "America/Phoenix", "MST7" },
    { "America/Sao_Paulo", "<-03>3" },
    { "America/Santiago", "<-04>4<-03>,M9.1.6/24,M4.1.6/24" },
    { "America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0" },
    { "America/Toronto", "EST5EDT,M3.2.0,M11.1.0" },
    { "America/Vancouver", "PST8PDT,M3.2.0,M11.1.0" },
    { "America/Winnipeg", "CST6CDT,M3.2.0,M11.1.0" },
};

static const size_t g_america_tz_table_count =
    sizeof(g_america_tz_table) / sizeof(g_america_tz_table[0]);

static const posix_tz_entry_t g_europe_tz_table[] = {
    { "Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Andorra", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Astrakhan", "MSK-3" },
    { "Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Belgrade", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Bratislava", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Bucharest", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Budapest", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Chisinau", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Dublin", "IST-1GMT0,M10.5.0,M3.5.0/1" },
    { "Europe/Gibraltar", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Istanbul", "<+03>-3" },
    { "Europe/Kaliningrad", "EET-2" },
    { "Europe/Kiev", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0" },
    { "Europe/London", "GMT0BST,M3.5.0/1,M10.5.0" },
    { "Europe/Luxembourg", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Malta", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Minsk", "<+03>-3" },
    { "Europe/Monaco", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Moscow", "MSK-3" },
    { "Europe/Oslo", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Riga", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Samara", "<+04>-4" },
    { "Europe/Sofia", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Stockholm", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Tallinn", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Tirane", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Vienna", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Vilnius", "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Zurich", "CET-1CEST,M3.5.0,M10.5.0/3" },
};

static const size_t g_europe_tz_table_count =
    sizeof(g_europe_tz_table) / sizeof(g_europe_tz_table[0]);

static const posix_tz_entry_t g_asia_tz_table[] = {
    { "Asia/Aden", "<+03>-3" },
    { "Asia/Almaty", "<+05>-5" },
    { "Asia/Amman", "<+03>-3" },
    { "Asia/Anadyr", "<+12>-12" },
    { "Asia/Aqtau", "<+05>-5" },
    { "Asia/Aqtobe", "<+05>-5" },
    { "Asia/Ashgabat", "<+05>-5" },
    { "Asia/Atyrau", "<+05>-5" },
    { "Asia/Baghdad", "<+03>-3" },
    { "Asia/Baku", "<+04>-4" },
    { "Asia/Bangkok", "<+07>-7" },
    { "Asia/Beirut", "EET-2EEST,M3.5.0/0,M10.5.0/0" },
    { "Asia/Bishkek", "<+06>-6" },
    { "Asia/Brunei", "<+08>-8" },
    { "Asia/Colombo", "<+0530>-5:30" },
    { "Asia/Damascus", "<+03>-3" },
    { "Asia/Dhaka", "<+06>-6" },
    { "Asia/Dubai", "<+04>-4" },
    { "Asia/Hong_Kong", "HKT-8" },
    { "Asia/Jakarta", "WIB-7" },
    { "Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0" },
    { "Asia/Kabul", "<+0430>-4:30" },
    { "Asia/Karachi", "PKT-5" },
    { "Asia/Kathmandu", "<+0545>-5:45" },
    { "Asia/Kolkata", "IST-5:30" },
    { "Asia/Manila", "PST-8" },
    { "Asia/Seoul", "KST-9" },
    { "Asia/Shanghai", "CST-8" },
    { "Asia/Singapore", "<+08>-8" },
    { "Asia/Taipei", "CST-8" },
    { "Asia/Tokyo", "JST-9" },
};

static const size_t g_asia_tz_table_count =
    sizeof(g_asia_tz_table) / sizeof(g_asia_tz_table[0]);

static const posix_tz_entry_t g_etc_tz_table[] = {
    { "Etc/GMT", "GMT0" },
    { "Etc/GMT+1", "<-01>1" },
    { "Etc/GMT+2", "<-02>2" },
    { "Etc/GMT+3", "<-03>3" },
    { "Etc/GMT+4", "<-04>4" },
    { "Etc/GMT+5", "<-05>5" },
    { "Etc/GMT+6", "<-06>6" },
    { "Etc/GMT+7", "<-07>7" },
    { "Etc/GMT+8", "<-08>8" },
    { "Etc/GMT+9", "<-09>9" },
    { "Etc/GMT+10", "<-10>10" },
    { "Etc/GMT-1", "<+01>-1" },
    { "Etc/GMT-2", "<+02>-2" },
    { "Etc/GMT-3", "<+03>-3" },
    { "Etc/GMT-4", "<+04>-4" },
    { "Etc/GMT-5", "<+05>-5" },
    { "Etc/GMT-6", "<+06>-6" },
    { "Etc/GMT-7", "<+07>-7" },
    { "Etc/GMT-8", "<+08>-8" },
    { "Etc/GMT-9", "<+09>-9" },
    { "Etc/GMT-10", "<+10>-10" },
    { "Etc/UTC", "UTC0" },
};

static const size_t g_etc_tz_table_count =
    sizeof(g_etc_tz_table) / sizeof(g_etc_tz_table[0]);

static const posix_tz_entry_t *find_in_region(const posix_tz_entry_t *table,
                                              size_t count,
                                              const char *zone_name)
{
    size_t i;

    for (i = 0; i < count; ++i) {
        if (strcmp(table[i].zone_name, zone_name) == 0) {
            return &table[i];
        }
    }

    return NULL;
}

const posix_tz_entry_t *posix_tz_find_entry(const char *zone_name)
{
    if (zone_name == NULL) {
        return NULL;
    }

    if (strncmp(zone_name, "Africa/", 7) == 0) {
        return find_in_region(g_africa_tz_table, g_africa_tz_table_count, zone_name);
    }

    if (strncmp(zone_name, "America/", 8) == 0) {
        return find_in_region(g_america_tz_table, g_america_tz_table_count, zone_name);
    }

    if (strncmp(zone_name, "Europe/", 7) == 0) {
        return find_in_region(g_europe_tz_table, g_europe_tz_table_count, zone_name);
    }

    if (strncmp(zone_name, "Asia/", 5) == 0) {
        return find_in_region(g_asia_tz_table, g_asia_tz_table_count, zone_name);
    }

    if (strncmp(zone_name, "Etc/", 4) == 0) {
        return find_in_region(g_etc_tz_table, g_etc_tz_table_count, zone_name);
    }

    return NULL;
}

const char *posix_tz_from_zone(const char *zone_name)
{
    const posix_tz_entry_t *entry = posix_tz_find_entry(zone_name);
    return (entry != NULL) ? entry->posix_tz : NULL;
}