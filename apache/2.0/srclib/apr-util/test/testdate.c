/* This program tests the date_parse_http routine in ../main/util_date.c.
 *
 * It is only semiautomated in that I would run it, modify the code to
 * use a different algorithm or seed, recompile and run again, etc.
 * Obviously it should use an argument for that, but I never got around
 * to changing the implementation.
 * 
 *     gcc -g -O2 -I../main -o test_date ../main/util_date.o test_date.c
 *     test_date | egrep '^No '
 * 
 * Roy Fielding, 1996
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "apr_date.h"

#ifndef srand48
#define srand48 srandom
#endif

#ifndef mrand48
#define mrand48 random
#endif

void gm_timestr_822(char *ts, apr_time_t sec);
void gm_timestr_850(char *ts, apr_time_t sec);
void gm_timestr_ccc(char *ts, apr_time_t sec);

static const apr_time_t year2secs[] = {
             APR_INT64_C(0),    /* 1970 */
      APR_INT64_C(31536000),    /* 1971 */
      APR_INT64_C(63072000),    /* 1972 */
      APR_INT64_C(94694400),    /* 1973 */
     APR_INT64_C(126230400),    /* 1974 */
     APR_INT64_C(157766400),    /* 1975 */
     APR_INT64_C(189302400),    /* 1976 */
     APR_INT64_C(220924800),    /* 1977 */
     APR_INT64_C(252460800),    /* 1978 */
     APR_INT64_C(283996800),    /* 1979 */
     APR_INT64_C(315532800),    /* 1980 */
     APR_INT64_C(347155200),    /* 1981 */
     APR_INT64_C(378691200),    /* 1982 */
     APR_INT64_C(410227200),    /* 1983 */
     APR_INT64_C(441763200),    /* 1984 */
     APR_INT64_C(473385600),    /* 1985 */
     APR_INT64_C(504921600),    /* 1986 */
     APR_INT64_C(536457600),    /* 1987 */
     APR_INT64_C(567993600),    /* 1988 */
     APR_INT64_C(599616000),    /* 1989 */
     APR_INT64_C(631152000),    /* 1990 */
     APR_INT64_C(662688000),    /* 1991 */
     APR_INT64_C(694224000),    /* 1992 */
     APR_INT64_C(725846400),    /* 1993 */
     APR_INT64_C(757382400),    /* 1994 */
     APR_INT64_C(788918400),    /* 1995 */
     APR_INT64_C(820454400),    /* 1996 */
     APR_INT64_C(852076800),    /* 1997 */
     APR_INT64_C(883612800),    /* 1998 */
     APR_INT64_C(915148800),    /* 1999 */
     APR_INT64_C(946684800),    /* 2000 */
     APR_INT64_C(978307200),    /* 2001 */
    APR_INT64_C(1009843200),    /* 2002 */
    APR_INT64_C(1041379200),    /* 2003 */
    APR_INT64_C(1072915200),    /* 2004 */
    APR_INT64_C(1104537600),    /* 2005 */
    APR_INT64_C(1136073600),    /* 2006 */
    APR_INT64_C(1167609600),    /* 2007 */
    APR_INT64_C(1199145600),    /* 2008 */
    APR_INT64_C(1230768000),    /* 2009 */
    APR_INT64_C(1262304000),    /* 2010 */
    APR_INT64_C(1293840000),    /* 2011 */
    APR_INT64_C(1325376000),    /* 2012 */
    APR_INT64_C(1356998400),    /* 2013 */
    APR_INT64_C(1388534400),    /* 2014 */
    APR_INT64_C(1420070400),    /* 2015 */
    APR_INT64_C(1451606400),    /* 2016 */
    APR_INT64_C(1483228800),    /* 2017 */
    APR_INT64_C(1514764800),    /* 2018 */
    APR_INT64_C(1546300800),    /* 2019 */
    APR_INT64_C(1577836800),    /* 2020 */
    APR_INT64_C(1609459200),    /* 2021 */
    APR_INT64_C(1640995200),    /* 2022 */
    APR_INT64_C(1672531200),    /* 2023 */
    APR_INT64_C(1704067200),    /* 2024 */
    APR_INT64_C(1735689600),    /* 2025 */
    APR_INT64_C(1767225600),    /* 2026 */
    APR_INT64_C(1798761600),    /* 2027 */
    APR_INT64_C(1830297600),    /* 2028 */
    APR_INT64_C(1861920000),    /* 2029 */
    APR_INT64_C(1893456000),    /* 2030 */
    APR_INT64_C(1924992000),    /* 2031 */
    APR_INT64_C(1956528000),    /* 2032 */
    APR_INT64_C(1988150400),    /* 2033 */
    APR_INT64_C(2019686400),    /* 2034 */
    APR_INT64_C(2051222400),    /* 2035 */
    APR_INT64_C(2082758400),    /* 2036 */
    APR_INT64_C(2114380800),    /* 2037 */
    APR_INT64_C(2145916800)     /* 2038 */
};

const char month_snames[12][4] = {
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

void gm_timestr_822(char *ts, apr_time_t sec)
{
    static const char *const days[7]=
        {"Sun","Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    struct tm *tms;
    time_t ls = (time_t)sec;

    tms = gmtime(&ls);
 
    sprintf(ts, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT", days[tms->tm_wday],
            tms->tm_mday, month_snames[tms->tm_mon], tms->tm_year + 1900,
            tms->tm_hour, tms->tm_min, tms->tm_sec);
}

void gm_timestr_850(char *ts, apr_time_t sec)
{
    static const char *const days[7]=
           {"Sunday","Monday", "Tuesday", "Wednesday", "Thursday", "Friday", 
            "Saturday"};
    struct tm *tms;
    int year;
    time_t ls = (time_t)sec;
 
    tms = gmtime(&ls);

    year = tms->tm_year;
    if (year >= 100) year -= 100;
 
    sprintf(ts, "%s, %.2d-%s-%.2d %.2d:%.2d:%.2d GMT", days[tms->tm_wday],
            tms->tm_mday, month_snames[tms->tm_mon], year,
            tms->tm_hour, tms->tm_min, tms->tm_sec);
}

void gm_timestr_ccc(char *ts, apr_time_t sec)
{
    static const char *const days[7]=
       {"Sun","Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    struct tm *tms;
    time_t ls = (time_t)sec;
 
    tms = gmtime(&ls);
 
    sprintf(ts, "%s %s %2d %.2d:%.2d:%.2d %d", days[tms->tm_wday],
            month_snames[tms->tm_mon], tms->tm_mday, 
            tms->tm_hour, tms->tm_min, tms->tm_sec, tms->tm_year + 1900);
}

int main (void)
{
    int year, i;
    apr_time_t guess;
    apr_time_t offset = 0;
 /* apr_time_t offset = 0; */
 /* apr_time_t offset = ((31 + 28) * 24 * 3600) - 1; */
    apr_time_t secstodate, newsecs;
    char datestr[50];

    for (year = 1970; year < 2038; ++year) {
        secstodate = year2secs[year - 1970] + offset;
        gm_timestr_822(datestr, secstodate);
        secstodate *= APR_USEC_PER_SEC;
        newsecs = apr_date_parse_http(datestr);
        if (secstodate == newsecs)
            printf("Yes %4d %19" APR_TIME_T_FMT " %s\n", year, secstodate, datestr);
        else if (newsecs == APR_DATE_BAD)
            printf("No  %4d %19" APR_TIME_T_FMT " %19" APR_TIME_T_FMT " %s\n",
                   year, secstodate, newsecs, datestr);
        else
            printf("No* %4d %19" APR_TIME_T_FMT " %19" APR_TIME_T_FMT " %s\n",
                   year, secstodate, newsecs, datestr);
    }
    
    srand48(978245L);

    for (i = 0; i < 10000; ++i) {
        guess = (time_t)mrand48();
        if (guess < 0) guess *= -1;
        secstodate = guess + offset;
        gm_timestr_822(datestr, secstodate);
        secstodate *= APR_USEC_PER_SEC;
        newsecs = apr_date_parse_http(datestr);
        if (secstodate == newsecs)
            printf("Yes %" APR_TIME_T_FMT " %s\n", secstodate, datestr);
        else if (newsecs == APR_DATE_BAD)
            printf("No  %" APR_TIME_T_FMT " %" APR_TIME_T_FMT " %s\n", 
                   secstodate, newsecs, datestr);
        else
            printf("No* %" APR_TIME_T_FMT " %" APR_TIME_T_FMT " %s\n", 
                   secstodate, newsecs, datestr);
    }
    exit(0);
}
