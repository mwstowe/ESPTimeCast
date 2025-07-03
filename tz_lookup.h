#ifndef TZ_LOOKUP_H
#define TZ_LOOKUP_H

typedef struct {
    const char* iana;
    const char* posix;
} TimeZoneMapping;

const TimeZoneMapping tz_mappings[] = {
    {"Etc/UTC", "UTC0"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0/2"},
    {"Europe/Dublin", "IST-1GMT0,M10.5.0,M3.5.0/1"},
    {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Europe/Paris", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Rome", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Madrid", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Brussels", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Zurich", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Vienna", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Prague", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Warsaw", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Budapest", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Istanbul", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Moscow", "MSK-3"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Bucharest", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Sofia", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Kiev", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Belgrade", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Copenhagen", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Stockholm", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Oslo", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/Riga", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Tallinn", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Vilnius", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Africa/Cairo", "EET-2"},
    {"Africa/Johannesburg", "SAST-2"},
    {"Africa/Lagos", "WAT-1"},
    {"Africa/Nairobi", "EAT-3"},
    {"Africa/Casablanca", "WET0"},
    {"Africa/Algiers", "CET-1"},
    {"Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0"},
    {"Asia/Dubai", "GST-4"},
    {"Asia/Kolkata", "IST-5:30"},
    {"Asia/Karachi", "PKT-5"},
    {"Asia/Dhaka", "BDT-6"},
    {"Asia/Ho_Chi_Minh", "ICT-7"},
    {"Asia/Bangkok", "ICT-7"},
    {"Asia/Jakarta", "WIB-7"},
    {"Asia/Singapore", "SGT-8"},
    {"Asia/Kuala_Lumpur", "MYT-8"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Hong_Kong", "HKT-8"},
    {"Asia/Taipei", "CST-8"},
    {"Asia/Seoul", "KST-9"},
    {"Asia/Tokyo", "JST-9"},
    {"Asia/Manila", "PHT-8"},
    {"Asia/Yangon", "MMT-6:30"},
    {"Asia/Kathmandu", "NPT-5:45"},
    {"Asia/Almaty", "ALMT-6"},
    {"Asia/Baku", "AZT-4AZST,M3.5.0/5,M10.5.0/6"},
    {"Asia/Tashkent", "UZT-5"},
    {"Asia/Tehran", "IRST-3:30IRDT,J79/24,J263/24"},
    {"Asia/Baghdad", "AST-3"},
    {"Asia/Riyadh", "AST-3"},
    {"Asia/Kuwait", "AST-3"},
    {"Asia/Amman", "EET-2EEST,J80/24,J273/1"},
    {"Asia/Beirut", "EET-2EEST,M3.5.0/0,M10.5.0/0"},
    {"Asia/Kabul", "AFT-4:30"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Brisbane", "AEST-10"},
    {"Australia/Perth", "AWST-8"},
    {"Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"Pacific/Fiji", "FJT-12FJST,M11.2.0,M1/1"},
    {"Pacific/Honolulu", "HST10"},
    {"Pacific/Guam", "ChST-10"},
    {"America/Anchorage", "AKDT9AKST,M3.2.0,M11.1.0"},
    {"America/Los_Angeles", "PDT8PST,M3.2.0,M11.1.0"},
    {"America/Vancouver", "PDT8PST,M3.2.0,M11.1.0"},
    {"America/Denver", "MDT7MST,M3.2.0,M11.1.0"},
    {"America/Phoenix", "MST7"},
    {"America/Chicago", "CDT6CST,M3.2.0,M11.1.0"},
    {"America/Mexico_City", "CDT6CST,M4.1.0,M10.5.0"},
    {"America/Toronto", "EDT5EST,M3.2.0,M11.1.0"},
    {"America/New_York", "EDT5EST,M3.2.0,M11.1.0"},
    {"America/Caracas", "VET4"},
    {"America/Bogota", "COT5"},
    {"America/Lima", "PET5"},
    {"America/Santiago", "CLST4CLT,M9.1.1,M4.2.7"},
    {"America/Argentina/Buenos_Aires", "ART3"},
    {"America/Sao_Paulo", "BRT3BRST,M10.3.0/0,M2.3.0/0"}
};

#define TZ_MAPPINGS_COUNT (sizeof(tz_mappings)/sizeof(tz_mappings[0]))

inline const char* ianaToPosix(const char* iana) {
    for (size_t i = 0; i < TZ_MAPPINGS_COUNT; i++) {
        if (strcmp(iana, tz_mappings[i].iana) == 0)
            return tz_mappings[i].posix;
    }
    return "UTC0"; // fallback
}

#endif // TZ_LOOKUP_H