#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/utime.h"

/*
 * MiOS - tiempo (time.h).
 *
 * Sin zonas horarias: localtime() == gmtime() (todo en UTC).
 */

#define SECS_PER_DAY 86400L

time_t time(time_t *tloc) {
    long r = mios_time((long *)tloc);

    return (time_t)mios_ret(r);
}

int gettimeofday(struct timeval *tv, void *tz) {
    return (int)mios_ret(mios_gettimeofday((struct mios_timeval *)tv, tz));
}

int clock_gettime(int clockid, struct timespec *ts) {
    return (int)mios_ret(mios_clock_gettime(clockid,
                                            (struct mios_timespec *)ts));
}

int clock_getres(int clockid, struct timespec *ts) {
    return (int)mios_ret(mios_clock_getres(clockid,
                                           (struct mios_timespec *)ts));
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    (void)rem;
    return mios_sleep_ns((long)req->tv_sec, (long)req->tv_nsec);
}

/* Fase 7: binutils copia tambien las fechas de los ficheros (rename.c). */
int utime(const char *path, const struct utimbuf *times) {
    return (int)mios_ret(mios_utime(path, (const struct mios_utimbuf *)times));
}

clock_t clock(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return -1;
    }
    return (clock_t)(ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}

double difftime(time_t a, time_t b) {
    return (double)(a - b);
}

/* ------------------------------------------------------------------ */
/* Calendario                                                          */
/* ------------------------------------------------------------------ */

static int is_leap(long long y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

/* Dias desde 1970-01-01 (algoritmo de Howard Hinnant). */
static long long days_from_civil(long long y, unsigned m, unsigned d) {
    y -= (m <= 2);
    {
        long long era = (y >= 0 ? y : y - 399) / 400;
        unsigned yoe = (unsigned)(y - era * 400);
        unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

        return era * 146097 + (long long)doe - 719468;
    }
}

static void civil_from_days(long long z, long long *yy, unsigned *mm,
                            unsigned *dd) {
    long long era;
    unsigned long long doe, yoe, doy, mp, d, y, m;

    z += 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = (unsigned long long)(z - era * 146097);
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + (unsigned long long)era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3 : (unsigned long long)-9);
    y += (m <= 2);

    *yy = (long long)y;
    *mm = (unsigned)m;
    *dd = (unsigned)d;
}

static const char *const wday_names[7] = {"dom", "lun", "mar", "mie",
                                          "jue", "vie", "sab"};
static const char *const mon_names[12] = {"ene", "feb", "mar", "abr",
                                          "may", "jun", "jul", "ago",
                                          "sep", "oct", "nov", "dic"};

struct tm *gmtime_r(const time_t *t, struct tm *out) {
    long long secs = (long long)*t;
    long long days = secs / SECS_PER_DAY;
    long long rem = secs % SECS_PER_DAY;
    long long y;
    unsigned m, d;
    static const int cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243,
                                273, 304, 334};

    if (rem < 0) {
        rem += SECS_PER_DAY;
        days--;
    }

    out->tm_hour = (int)(rem / 3600);
    rem %= 3600;
    out->tm_min = (int)(rem / 60);
    out->tm_sec = (int)(rem % 60);

    civil_from_days(days, &y, &m, &d);
    out->tm_year = (int)(y - 1900);
    out->tm_mon = (int)(m - 1);
    out->tm_mday = (int)d;
    out->tm_wday = (int)(((days % 7) + 11) % 7);
    out->tm_yday = cum[m - 1] + (int)d - 1;
    if (m > 2 && is_leap(y)) {
        out->tm_yday++;
    }
    out->tm_isdst = 0;
    return out;
}

struct tm *gmtime(const time_t *t) {
    static struct tm buf;

    return gmtime_r(t, &buf);
}

struct tm *localtime_r(const time_t *t, struct tm *out) {
    return gmtime_r(t, out);
}

struct tm *localtime(const time_t *t) {
    static struct tm buf;

    return gmtime_r(t, &buf);
}

time_t mktime(struct tm *tm) {
    long long y = tm->tm_year + 1900;
    long long mon = tm->tm_mon;
    long long day = tm->tm_mday;
    long long days;
    long long secs;
    struct tm norm;

    /* Normaliza el mes fuera de rango. */
    y += mon / 12;
    mon %= 12;
    if (mon < 0) {
        mon += 12;
        y--;
    }

    days = days_from_civil(y, (unsigned)(mon + 1), 1) + (day - 1);
    secs = days * SECS_PER_DAY + (long long)tm->tm_hour * 3600 +
           (long long)tm->tm_min * 60 + tm->tm_sec;

    time_t result = (time_t)secs;

    if (gmtime_r(&result, &norm) != (struct tm *)0) {
        norm.tm_isdst = 0;
        *tm = norm;
    }
    return result;
}

char *asctime(const struct tm *tm) {
    static char buf[26];
    const char *wd = (tm->tm_wday >= 0 && tm->tm_wday < 7)
                         ? wday_names[tm->tm_wday]
                         : "???";
    const char *mo = (tm->tm_mon >= 0 && tm->tm_mon < 12)
                         ? mon_names[tm->tm_mon]
                         : "???";
    char tmp[32];
    int i = 0;

    for (const char *p = wd; *p != '\0'; p++) {
        tmp[i++] = *p;
    }
    tmp[i++] = ' ';
    for (const char *p = mo; *p != '\0'; p++) {
        tmp[i++] = *p;
    }
    tmp[i++] = ' ';
    {
        int dd = tm->tm_mday;
        tmp[i++] = (char)('0' + (dd / 10) % 10);
        tmp[i++] = (char)('0' + dd % 10);
    }
    tmp[i++] = ' ';
    {
        int hh = tm->tm_hour;
        tmp[i++] = (char)('0' + (hh / 10) % 10);
        tmp[i++] = (char)('0' + hh % 10);
        tmp[i++] = ':';
    }
    {
        int mi = tm->tm_min;
        tmp[i++] = (char)('0' + (mi / 10) % 10);
        tmp[i++] = (char)('0' + mi % 10);
        tmp[i++] = ':';
    }
    {
        int ss = tm->tm_sec;
        tmp[i++] = (char)('0' + (ss / 10) % 10);
        tmp[i++] = (char)('0' + ss % 10);
    }
    tmp[i++] = ' ';
    {
        int yr = tm->tm_year + 1900;
        tmp[i++] = (char)('0' + (yr / 1000) % 10);
        tmp[i++] = (char)('0' + (yr / 100) % 10);
        tmp[i++] = (char)('0' + (yr / 10) % 10);
        tmp[i++] = (char)('0' + yr % 10);
    }
    tmp[i++] = '\n';
    tmp[i] = '\0';

    memcpy(buf, tmp, (size_t)i + 1);
    return buf;
}

char *ctime(const time_t *t) {
    return asctime(localtime(t));
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
    size_t n = 0;

#define PUT(ch)                     \
    do {                            \
        if (n + 1 < max) {          \
            s[n] = (char)(ch);      \
        }                           \
        n++;                        \
    } while (0)

#define PUT2(a, b)      \
    do {                \
        PUT(a);         \
        PUT(b);         \
    } while (0)

    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            PUT(*fmt);
            continue;
        }
        fmt++;
        switch (*fmt) {
        case 'a':
            for (const char *p = wday_names[(tm->tm_wday % 7 + 7) % 7]; *p; p++) {
                PUT(*p);
            }
            break;
        case 'A':
            for (const char *p = wday_names[(tm->tm_wday % 7 + 7) % 7]; *p; p++) {
                PUT(*p);
            }
            break;
        case 'b':
        case 'h':
            for (const char *p = mon_names[(tm->tm_mon % 12 + 12) % 12]; *p; p++) {
                PUT(*p);
            }
            break;
        case 'B':
            for (const char *p = mon_names[(tm->tm_mon % 12 + 12) % 12]; *p; p++) {
                PUT(*p);
            }
            break;
        case 'd':
            PUT('0' + (tm->tm_mday / 10) % 10);
            PUT('0' + tm->tm_mday % 10);
            break;
        case 'e':
            PUT(tm->tm_mday < 10 ? ' ' : ('0' + (tm->tm_mday / 10) % 10));
            PUT('0' + tm->tm_mday % 10);
            break;
        case 'H':
            PUT('0' + (tm->tm_hour / 10) % 10);
            PUT('0' + tm->tm_hour % 10);
            break;
        case 'I': {
            int h = tm->tm_hour % 12;
            if (h == 0) {
                h = 12;
            }
            PUT('0' + (h / 10) % 10);
            PUT('0' + h % 10);
            break;
        }
        case 'j':
            PUT('0' + (tm->tm_yday + 1) / 100);
            PUT('0' + ((tm->tm_yday + 1) / 10) % 10);
            PUT('0' + (tm->tm_yday + 1) % 10);
            break;
        case 'm':
            PUT('0' + ((tm->tm_mon + 1) / 10) % 10);
            PUT('0' + (tm->tm_mon + 1) % 10);
            break;
        case 'M':
            PUT('0' + (tm->tm_min / 10) % 10);
            PUT('0' + tm->tm_min % 10);
            break;
        case 'n':
            PUT('\n');
            break;
        case 'p':
            PUT(tm->tm_hour < 12 ? 'A' : 'P');
            PUT('M');
            break;
        case 'S':
            PUT('0' + (tm->tm_sec / 10) % 10);
            PUT('0' + tm->tm_sec % 10);
            break;
        case 't':
            PUT('\t');
            break;
        case 'T':
            PUT('0' + (tm->tm_hour / 10) % 10);
            PUT('0' + tm->tm_hour % 10);
            PUT(':');
            PUT('0' + (tm->tm_min / 10) % 10);
            PUT('0' + tm->tm_min % 10);
            PUT(':');
            PUT('0' + (tm->tm_sec / 10) % 10);
            PUT('0' + tm->tm_sec % 10);
            break;
        case 'u':
            PUT('0' + (tm->tm_wday == 0 ? 7 : tm->tm_wday));
            break;
        case 'w':
            PUT('0' + tm->tm_wday);
            break;
        case 'y':
            PUT('0' + ((tm->tm_year + 1900) / 10) % 10);
            PUT('0' + (tm->tm_year + 1900) % 10);
            break;
        case 'Y': {
            int yr = tm->tm_year + 1900;
            PUT('0' + (yr / 1000) % 10);
            PUT('0' + (yr / 100) % 10);
            PUT('0' + (yr / 10) % 10);
            PUT('0' + yr % 10);
            break;
        }
        case 'F': {
            int yr = tm->tm_year + 1900;
            PUT('0' + (yr / 1000) % 10);
            PUT('0' + (yr / 100) % 10);
            PUT('0' + (yr / 10) % 10);
            PUT('0' + yr % 10);
            PUT('-');
            PUT('0' + ((tm->tm_mon + 1) / 10) % 10);
            PUT('0' + (tm->tm_mon + 1) % 10);
            PUT('-');
            PUT('0' + (tm->tm_mday / 10) % 10);
            PUT('0' + tm->tm_mday % 10);
            break;
        }
        case 'D':
            PUT('0' + ((tm->tm_mon + 1) / 10) % 10);
            PUT('0' + (tm->tm_mon + 1) % 10);
            PUT('/');
            PUT('0' + (tm->tm_mday / 10) % 10);
            PUT('0' + tm->tm_mday % 10);
            PUT('/');
            PUT('0' + ((tm->tm_year + 1900) / 10) % 10);
            PUT('0' + (tm->tm_year + 1900) % 10);
            break;
        case 'R':
            PUT2('0' + (tm->tm_hour / 10) % 10, '0' + tm->tm_hour % 10);
            PUT(':');
            PUT2('0' + (tm->tm_min / 10) % 10, '0' + tm->tm_min % 10);
            break;
        case 'Z':
            PUT('U');
            PUT('T');
            PUT('C');
            break;
        case 's': {
            time_t v = mktime((struct tm *)tm);
            char num[32];
            int k = 0;
            long long x = (long long)v;

            if (x == 0) {
                num[k++] = '0';
            }
            while (x > 0) {
                num[k++] = (char)('0' + (int)(x % 10));
                x /= 10;
            }
            while (k > 0) {
                PUT(num[--k]);
            }
            break;
        }
        case '%':
            PUT('%');
            break;
        default:
            PUT('%');
            if (*fmt != '\0') {
                PUT(*fmt);
            }
            break;
        }
    }

    if (max > 0) {
        size_t end = (n < max) ? n : max - 1;
        s[end] = '\0';
    }
    return n;
}
