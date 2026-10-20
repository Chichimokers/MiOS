#ifndef MIOS_TIME_H
#define MIOS_TIME_H

/*
 * MiOS - time.h (cabecera publica).
 * Sin zonas horarias: localtime() y gmtime() dan la misma hora (UTC).
 */

#include "sys/types.h"

struct timeval {
    time_t      tv_sec;
    long        tv_usec;
};

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

#define CLOCKS_PER_SEC 1000000L

time_t time(time_t *tloc);
int    gettimeofday(struct timeval *tv, void *tz);
int    clock_gettime(int clockid, struct timespec *ts);
int    clock_getres(int clockid, struct timespec *ts);
clock_t clock(void);
double difftime(time_t a, time_t b);

struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);
struct tm *gmtime_r(const time_t *t, struct tm *out);
struct tm *localtime_r(const time_t *t, struct tm *out);
time_t     mktime(struct tm *tm);
char      *asctime(const struct tm *tm);
char      *ctime(const time_t *t);
size_t     strftime(char *s, size_t max, const char *fmt, const struct tm *tm);
int        nanosleep(const struct timespec *req, struct timespec *rem);

#endif /* MIOS_TIME_H */
