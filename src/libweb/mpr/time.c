#include "libmpr.h"

/**
    time.c - Date and Time handling

 */

/********************************* Includes ***********************************/



/********************************** Defines ***********************************/

#define MS_PER_SEC  (TPS)
#define MS_PER_HOUR (60 * 60 * TPS)
#define MS_PER_MIN  (60 * TPS)
#define MS_PER_DAY  (86400 * TPS)
#define MS_PER_YEAR (INT64(31556952000))

/*
    On some platforms, time_t is only 32 bits (linux-32) and on some 64 bit systems, time calculations
    outside the range of 32 bits are unreliable. This means there is a minimum and maximum year that
    can be analysed using the O/S localtime routines. However, we really want to use the O/S
    calculations for daylight savings time, so when a date is outside a 32 bit time_t range, we use
    some trickery to remap the year to a temporary (current) year so localtime can be used.
    FYI: 32 bit time_t expires at: 03:14:07 UTC on Tuesday, 19 January 2038
 */
#define MIN_YEAR    1901
#define MAX_YEAR    2037

/*
    MacOSX cannot handle MIN_TIME == -0x7FFFFFFF
 */
#define MAX_TIME    0x7FFFFFFF
#define MIN_TIME    -0xFFFFFFF

/*
    Token types or'd into the TimeToken value
 */
#define TOKEN_DAY       0x01000000
#define TOKEN_MONTH     0x02000000
#define TOKEN_ZONE      0x04000000
#define TOKEN_OFFSET    0x08000000
#define TOKEN_MASK      0xFF000000

typedef struct TimeToken {
    char    *name;
    int     value;
} TimeToken;

static TimeToken days[] = {
    { "sun",  0 | TOKEN_DAY },
    { "mon",  1 | TOKEN_DAY },
    { "tue",  2 | TOKEN_DAY },
    { "wed",  3 | TOKEN_DAY },
    { "thu",  4 | TOKEN_DAY },
    { "fri",  5 | TOKEN_DAY },
    { "sat",  6 | TOKEN_DAY },
    { 0, 0 },
};

static TimeToken fullDays[] = {
    { "sunday",     0 | TOKEN_DAY },
    { "monday",     1 | TOKEN_DAY },
    { "tuesday",    2 | TOKEN_DAY },
    { "wednesday",  3 | TOKEN_DAY },
    { "thursday",   4 | TOKEN_DAY },
    { "friday",     5 | TOKEN_DAY },
    { "saturday",   6 | TOKEN_DAY },
    { 0, 0 },
};

/*
    Make origin 1 to correspond to user date entries 10/28/2014
 */
static TimeToken months[] = {
    { "jan",  1 | TOKEN_MONTH },
    { "feb",  2 | TOKEN_MONTH },
    { "mar",  3 | TOKEN_MONTH },
    { "apr",  4 | TOKEN_MONTH },
    { "may",  5 | TOKEN_MONTH },
    { "jun",  6 | TOKEN_MONTH },
    { "jul",  7 | TOKEN_MONTH },
    { "aug",  8 | TOKEN_MONTH },
    { "sep",  9 | TOKEN_MONTH },
    { "oct", 10 | TOKEN_MONTH },
    { "nov", 11 | TOKEN_MONTH },
    { "dec", 12 | TOKEN_MONTH },
    { 0, 0 },
};

static TimeToken fullMonths[] = {
    { "january",    1 | TOKEN_MONTH },
    { "february",   2 | TOKEN_MONTH },
    { "march",      3 | TOKEN_MONTH },
    { "april",      4 | TOKEN_MONTH },
    { "may",        5 | TOKEN_MONTH },
    { "june",       6 | TOKEN_MONTH },
    { "july",       7 | TOKEN_MONTH },
    { "august",     8 | TOKEN_MONTH },
    { "september",  9 | TOKEN_MONTH },
    { "october",   10 | TOKEN_MONTH },
    { "november",  11 | TOKEN_MONTH },
    { "december",  12 | TOKEN_MONTH },
    { 0, 0 }
};

static TimeToken ampm[] = {
    { "am", 0 | TOKEN_OFFSET },
    { "pm", (12 * 3600) | TOKEN_OFFSET },
    { 0, 0 },
};


static TimeToken zones[] = {
    { "ut",      0 | TOKEN_ZONE},
    { "utc",     0 | TOKEN_ZONE},
    { "gmt",     0 | TOKEN_ZONE},
    { "edt",  -240 | TOKEN_ZONE},
    { "est",  -300 | TOKEN_ZONE},
    { "cdt",  -300 | TOKEN_ZONE},
    { "cst",  -360 | TOKEN_ZONE},
    { "mdt",  -360 | TOKEN_ZONE},
    { "mst",  -420 | TOKEN_ZONE},
    { "pdt",  -420 | TOKEN_ZONE},
    { "pst",  -480 | TOKEN_ZONE},
    { 0, 0 },
};


static TimeToken offsets[] = {
    { "tomorrow",    86400 | TOKEN_OFFSET},
    { "yesterday",  -86400 | TOKEN_OFFSET},
    { "next week",   (86400 * 7) | TOKEN_OFFSET},
    { "last week",  -(86400 * 7) | TOKEN_OFFSET},
    { 0, 0 },
};

static int timeSep = ':';

/*
    Formats for mprFormatTime
 */
#define VALID_FMT "AaBbCcDdEeFGgHhIjklMmnOPpRrSsTtUuVvWwXxYyZz+%"

#define HAS_STRFTIME 1

#if !HAS_STRFTIME
static char *abbrevDay[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static char *day[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static char *abbrevMonth[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char *month[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
#endif /* !HAS_STRFTIME */


static int normalMonthStart[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 0,
};
static int leapMonthStart[] = {
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 0
};

static MprTime daysSinceEpoch(int year);
static void decodeTime(struct tm *tp, MprTime when, bool local);
static int getTimeZoneOffsetFromTm(struct tm *tp);
static int leapYear(int year);
static int localTime(struct tm *timep, MprTime time);
static MprTime makeTime(struct tm *tp);
static void validateTime(struct tm *tm, struct tm *defaults);

/************************************ Code ************************************/
/*
    Initialize the time service
 */
PUBLIC int mprCreateTimeService()
{
    Mpr                 *mpr;
    TimeToken           *tt;

    mpr = MPR;
    mpr->timeTokens = mprCreateHash(59, MPR_HASH_STATIC_KEYS | MPR_HASH_STATIC_VALUES | MPR_HASH_STABLE);
    for (tt = days; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = fullDays; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = months; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = fullMonths; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = ampm; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = zones; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = offsets; tt->name; tt++) {
        mprAddKey(mpr->timeTokens, tt->name, (void*) tt);
    }
    return 0;
}


PUBLIC int mprCompareTime(MprTime t1, MprTime t2)
{
    if (t1 < t2) {
        return -1;
    } else if (t1 == t2) {
        return 0;
    }
    return 1;
}


PUBLIC void mprDecodeLocalTime(struct tm *tp, MprTime when)
{
    decodeTime(tp, when, 1);
}


PUBLIC void mprDecodeUniversalTime(struct tm *tp, MprTime when)
{
    decodeTime(tp, when, 0);
}


PUBLIC char *mprGetDate(char *format)
{
    struct tm   tm;

    mprDecodeLocalTime(&tm, mprGetTime());
    if (format == 0 || *format == '\0') {
        format = MPR_DEFAULT_DATE;
    }
    return mprFormatTm(format, &tm);
}


PUBLIC char *mprFormatLocalTime(cchar *format, MprTime time)
{
    struct tm   tm;
    if (format == 0) {
        format = MPR_DEFAULT_DATE;
    }
    mprDecodeLocalTime(&tm, time);
    return mprFormatTm(format, &tm);
}


PUBLIC char *mprFormatUniversalTime(cchar *format, MprTime time)
{
    struct tm   tm;
    if (format == 0) {
        format = MPR_DEFAULT_DATE;
    }
    mprDecodeUniversalTime(&tm, time);
    return mprFormatTm(format, &tm);
}


/*
    Returns time in milliseconds since the epoch: 0:0:0 UTC Jan 1 1970.
 */
PUBLIC MprTime mprGetTime()
{
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (MprTime) (((MprTime) tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}


/*
    High resolution timer
 */
#if MPR_HIGH_RES_TIMER
    #if LINUX && (ME_CPU_ARCH == ME_CPU_X86 || ME_CPU_ARCH == ME_CPU_X64)
        uint64 mprGetHiResTicks() {
            uint64  now;
            __asm__ __volatile__ ("rdtsc" : "=A" (now));
            return now;
        }
    #endif
#else
    uint64 mprGetHiResTicks() {
        return (uint64) mprGetTicks();
    }
#endif

/*
    Ugh! Apparently monotonic clocks are broken on VxWorks prior to 6.7
 */
#if CLOCK_MONOTONIC_RAW
    #define HAS_MONOTONIC_RAW 1
#endif

#if CLOCK_MONOTONIC
    #define HAS_MONOTONIC 1
#endif

/*
    Return time in milliseconds that never goes backwards. This is used for timers and not for time of day.
    The actual value returned is system dependant and does not represent time since Jan 1 1970.
    It may drift from real-time over time.
 */
PUBLIC MprTicks mprGetTicks()
{
#if HAS_MONOTONIC_RAW
    /*
        Monotonic raw is the local oscillator. This may over time diverge from real time ticks.
        We prefer this to monotonic because the ticks will always increase by the same regular amount without adjustments.
     */
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
    return (MprTicks) (((MprTicks) tv.tv_sec) * 1000) + (tv.tv_nsec / (1000 * 1000));
#elif HAS_MONOTONIC
    /*
        Monotonic is the local oscillator with NTP gradual adjustments as NTP learns the differences between the local
        oscillator and NTP measured real clock time. i.e. it will adjust ticks over time to not lose ticks.
     */
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (MprTicks) (((MprTicks) tv.tv_sec) * 1000) + (tv.tv_nsec / (1000 * 1000));
#else
    /*
        Last chance. Need to resort to mprGetTime which is subject to user and seasonal adjustments.
        This code will prevent it going backwards, but may suffer large jumps forward.
     */
    static MprTime lastTicks = 0;
    static MprTime adjustTicks = 0;
    static MprSpin ticksSpin;
    MprTime     result, diff;

    if (lastTicks == 0) {
        /* This will happen at init time when single threaded */
        lastTicks = mprGetTime();
        mprInitSpinLock(&ticksSpin);
    }
    mprSpinLock(&ticksSpin);

    result = mprGetTime() + adjustTicks;
    if ((diff = (result - lastTicks)) < 0) {
        /*
            Handle time reversals. Don't handle jumps forward. Sorry.
            Note: this is not time day, so it should not matter.
         */
        adjustTicks -= diff;
        result -= diff;
    }
    lastTicks = result;
    mprSpinUnlock(&ticksSpin);
    return result;
#endif
}


/*
    Return the number of milliseconds until the given timeout has expired.
 */
PUBLIC MprTicks mprGetRemainingTicks(MprTicks mark, MprTicks timeout)
{
    MprTicks    now, diff;

    now = mprGetTicks();
    diff = (now - mark);
    if (diff < 0) {
        /*
            Detect time going backwards. Should never happen now.
         */
        assert(diff >= 0);
        diff = 0;
    }
    return (timeout - diff);
}


PUBLIC MprTicks mprGetElapsedTicks(MprTicks mark)
{
    return mprGetTicks() - mark;
}


PUBLIC MprTime mprGetElapsedTime(MprTime mark)
{
    return mprGetTime() - mark;
}


/*
    Get the timezone offset including DST
    Return the timezone offset (including DST) in msec. local == (UTC + offset)
 */
PUBLIC int mprGetTimeZoneOffset(MprTime when)
{
    MprTime     alternate, secs;
    struct tm   t;
    int         offset;

    alternate = when;
    secs = when / MS_PER_SEC;
    if (secs < MIN_TIME || secs > MAX_TIME) {
        /* secs overflows time_t on this platform. Need to map to an alternate valid year */
        decodeTime(&t, when, 0);
        t.tm_year = 111;
        alternate = makeTime(&t);
    }
    t.tm_isdst = -1;
    if (localTime(&t, alternate) < 0) {
        localTime(&t, time(0) * MS_PER_SEC);
    }
    offset = getTimeZoneOffsetFromTm(&t);
    return offset;
}


/*
    Make a time value interpreting "tm" as a local time
 */
PUBLIC MprTime mprMakeTime(struct tm *tp)
{
    MprTime     when, alternate;
    struct tm   t;
    int         offset, year;

    when = makeTime(tp);
    year = tp->tm_year;
    if (MIN_YEAR <= year && year <= MAX_YEAR) {
        localTime(&t, when);
        offset = getTimeZoneOffsetFromTm(&t);
    } else {
        t = *tp;
        t.tm_year = 111;
        alternate = makeTime(&t);
        localTime(&t, alternate);
        offset = getTimeZoneOffsetFromTm(&t);
    }
    return when - offset;
}


PUBLIC MprTime mprMakeUniversalTime(struct tm *tp)
{
    return makeTime(tp);
}


/*************************************** O/S Layer ***********************************/

static int localTime(struct tm *timep, MprTime time)
{
    time_t when = (time_t) (time / MS_PER_SEC);
    if (localtime_r(&when, timep) == 0) {
        return MPR_ERR;
    }

    return 0;
}


struct tm *universalTime(struct tm *timep, MprTime time)
{
    time_t when = (time_t) (time / MS_PER_SEC);
    return gmtime_r(&when, timep);
}


/*
    Return the timezone offset (including DST) in msec. local == (UTC + offset)
    Assumes a valid (local) "tm" with isdst correctly set.
 */
static int getTimeZoneOffsetFromTm(struct tm *tp)
{
    return (int) tp->tm_gmtoff * MS_PER_SEC;
}

/********************************* Calculations *********************************/
/*
    Convert "struct tm" to MprTime. This ignores GMT offset and DST.
 */
static MprTime makeTime(struct tm *tp)
{
    MprTime     days;
    int         year, month;

    year = tp->tm_year + 1900 + tp->tm_mon / 12;
    month = tp->tm_mon % 12;
    if (month < 0) {
        month += 12;
        --year;
    }
    days = daysSinceEpoch(year);
    days += leapYear(year) ? leapMonthStart[month] : normalMonthStart[month];
    days += tp->tm_mday - 1;
    return (days * MS_PER_DAY) + ((((((tp->tm_hour * 60)) + tp->tm_min) * 60) + tp->tm_sec) * MS_PER_SEC);
}


static MprTime daysSinceEpoch(int year)
{
    MprTime     days;

    days = ((MprTime) 365) * (year - 1970);
    days += ((year-1) / 4) - (1970 / 4);
    days -= ((year-1) / 100) - (1970 / 100);
    days += ((year-1) / 400) - (1970 / 400);
    return days;
}


static int leapYear(int year)
{
    if (year % 4) {
        return 0;
    } else if (year % 400 == 0) {
        return 1;
    } else if (year % 100 == 0) {
        return 0;
    }
    return 1;
}


static int getMonth(int year, int day)
{
    int     *days, i;

    days = leapYear(year) ? leapMonthStart : normalMonthStart;
    for (i = 1; days[i]; i++) {
        if (day < days[i]) {
            return i - 1;
        }
    }
    return 11;
}


static int getYear(MprTime when)
{
    MprTime     ms;
    int         year;

    year = 1970 + (int) (when / MS_PER_YEAR);
    ms = daysSinceEpoch(year) * MS_PER_DAY;
    if (ms > when) {
        return year - 1;
    } else if (ms + (((MprTime) MS_PER_DAY) * (365 + leapYear(year))) <= when) {
        return year + 1;
    }
    return year;
}


PUBLIC MprTime floorDiv(MprTime x, MprTime divisor)
{
    if (x < 0) {
        return (x - divisor + 1) / divisor;
    } else {
        return x / divisor;
    }
}


/*
    Decode an MprTime into components in a "struct tm"
 */
static void decodeTime(struct tm *tp, MprTime when, bool local)
{
    MprTime     timeForZoneCalc, secs;
    struct tm   t;
    int         year, offset, dst;
    char        *zoneName = 0;

    offset = dst = 0;

    if (local) {
        timeForZoneCalc = when;
        secs = when / MS_PER_SEC;
        if (secs < MIN_TIME || secs > MAX_TIME) {
            /*
                On some systems, localTime won't work for very small (negative) or very large times.
                Cannot be certain localTime will work for all O/Ss with this year.  Map to a date with a valid year.
             */
            decodeTime(&t, when, 0);
            t.tm_year = 111;
            timeForZoneCalc = makeTime(&t);
        }
        t.tm_isdst = -1;
        if (localTime(&t, timeForZoneCalc) == 0) {
            offset = getTimeZoneOffsetFromTm(&t);
            dst = t.tm_isdst;
        } else {
            printf("ERROR: Cannot get local time\n");
        }

        zoneName = (char*) t.tm_zone;
        when += offset;
    }
    year = getYear(when);

    tp->tm_year     = year - 1900;
    tp->tm_hour     = (int) (floorDiv(when, MS_PER_HOUR) % 24);
    tp->tm_min      = (int) (floorDiv(when, MS_PER_MIN) % 60);
    tp->tm_sec      = (int) (floorDiv(when, MS_PER_SEC) % 60);
    tp->tm_wday     = (int) ((floorDiv(when, MS_PER_DAY) + 4) % 7);
    tp->tm_yday     = (int) (floorDiv(when, MS_PER_DAY) - daysSinceEpoch(year));
    tp->tm_mon      = getMonth(year, tp->tm_yday);
    tp->tm_isdst    = dst != 0;
    tp->tm_gmtoff   = offset / MS_PER_SEC;
    tp->tm_zone     = zoneName;

    if (tp->tm_hour < 0) {
        tp->tm_hour += 24;
    }
    if (tp->tm_min < 0) {
        tp->tm_min += 60;
    }
    if (tp->tm_sec < 0) {
        tp->tm_sec += 60;
    }
    if (tp->tm_wday < 0) {
        tp->tm_wday += 7;
    }
    if (tp->tm_yday < 0) {
        tp->tm_yday += 365;
    }
    if (leapYear(year)) {
        tp->tm_mday = tp->tm_yday - leapMonthStart[tp->tm_mon] + 1;
    } else {
        tp->tm_mday = tp->tm_yday - normalMonthStart[tp->tm_mon] + 1;
    }
    assert(tp->tm_hour >= 0);
    assert(tp->tm_min >= 0);
    assert(tp->tm_sec >= 0);
    assert(tp->tm_wday >= 0);
    assert(tp->tm_mon >= 0);
    /* This asserts with some calculating some intermediate dates <= year 100 */
    assert(tp->tm_yday >= 0);
    assert(tp->tm_yday < 365 || (tp->tm_yday < 366 && leapYear(year)));
    assert(tp->tm_mday >= 1);
}


/********************************* Formatting **********************************/
/*
    Format a time string. This uses strftime if available and so the supported formats vary from platform to platform.
    Strftime should supports some of these these formats:

     %A      full weekday name (Monday)
     %a      abbreviated weekday name (Mon)
     %B      full month name (January)
     %b      abbreviated month name (Jan)
     %C      century. Year / 100. (0-N)
     %c      standard date and time representation
     %D      date (%m/%d/%y)
     %d      day-of-month (01-31)
     %e      day-of-month with a leading space if only one digit ( 1-31)
     %F      same as %Y-%m-%d
     %H      hour (24 hour clock) (00-23)
     %h      same as %b
     %I      hour (12 hour clock) (01-12)
     %j      day-of-year (001-366)
     %k      hour (24 hour clock) (0-23)
     %l      the hour (12-hour clock) as a decimal number (1-12); single digits are preceded by a blank.
     %M      minute (00-59)
     %m      month (01-12)
     %n      a newline
     %P      lower case am / pm
     %p      AM / PM
     %R      same as %H:%M
     %r      same as %H:%M:%S %p
     %S      second (00-59)
     %s      seconds since epoch
     %T      time (%H:%M:%S)
     %t      a tab.
     %U      week-of-year, first day sunday (00-53)
     %u      the weekday (Monday as the first day of the week) as a decimal number (1-7).
     %v      is equivalent to ``%e-%b-%Y''.
     %W      week-of-year, first day monday (00-53)
     %w      weekday (0-6, sunday is 0)
     %X      standard time representation
     %x      standard date representation
     %Y      year with century
     %y      year without century (00-99)
     %Z      timezone name
     %z      offset from UTC (-hhmm or +hhmm)
     %+      national representation of the date and time (the format is similar to that produced by date(1)).
     %%      percent sign

     Some platforms may also support the following format extensions:
     %E*     POSIX locale extensions. Where "*" is one of the characters: c, C, x, X, y, Y.
             representations.
     %G      a year as a decimal number with century. This year is the one that contains the greater part of
             the week (Monday as the first day of the week).
     %g      the same year as in ``%G'', but as a decimal number without century (00-99).
     %O*     POSIX locale extensions. Where "*" is one of the characters: d, e, H, I, m, M, S, u, U, V, w, W, y.
             Additionly %OB implemented to represent alternative months names (used standalone, without day mentioned).
     %V      the week number of the year (Monday as the first day of the week) as a decimal number (01-53). If the week
             containing January 1 has four or more days in the new year, then it is week 1; otherwise it is the last
             week of the previous year, and the next week is week 1.

    Useful formats:
        RFC822:  "%a, %d %b %Y %H:%M:%S %Z           "Fri, 07 Jan 2003 12:12:21 PDT"
                 "%T %F                              "12:12:21 2007-01-03"
                 "%v                                 "07-Jul-2003"
        RFC3399: "%FT%TZ"                            "1985-04-12T23:20:50.52Z"
 */

#if HAS_STRFTIME
/*
    Preferred implementation as strftime() will be localized
 */
PUBLIC char *mprFormatTm(cchar *format, struct tm *tp)
{
    struct tm       tm;
    cchar           *cp;
    char            localFmt[256], buf[256], *dp, *endp, *sign;
    ssize           size;
    int             value;

    dp = localFmt;
    if (format == 0) {
        format = MPR_DEFAULT_DATE;
    }
    if (tp == 0) {
        mprDecodeLocalTime(&tm, mprGetTime());
        tp = &tm;
    }
    endp = &localFmt[sizeof(localFmt) - 1];
    size = sizeof(localFmt) - 1;
    for (cp = format; *cp && dp < &localFmt[sizeof(localFmt) - 32]; size = (int) (endp - dp - 1)) {
        if (*cp == '%') {
            *dp++ = *cp++;
        again:
            switch (*cp) {
            case '+':
                if (tp->tm_mday < 10) {
                    /* Some platforms don't support 'e' so avoid it here. Put a double space before %d */
                    fmt(dp, size, "%s  %d %s", "a %b", tp->tm_mday, "%H:%M:%S %Z %Y");
                } else {
                    strcpy(dp, "a %b %d %H:%M:%S %Z %Y");
                }
                dp += slen(dp);
                cp++;
                break;

            case 'C':
                dp--;
                itosbuf(dp, size, (1900 + tp->tm_year) / 100, 10);
                dp += slen(dp);
                cp++;
                break;

            case 'D':
                strcpy(dp, "m/%d/%y");
                dp += 7;
                cp++;
                break;

            case 'e':                       /* day of month (1-31). Single digits preceeded by blanks */
                dp--;
                if (tp->tm_mday < 10) {
                    *dp++ = ' ';
                }
                itosbuf(dp, size - 1, (int64) tp->tm_mday, 10);
                dp += slen(dp);
                cp++;
                break;

            case 'E':
                /* Skip the 'E' */
                cp++;
                goto again;

            case 'f':
                strcpy(--dp, "000");
                dp += slen(dp);
                cp++;
                break;

            case 'F':
                strcpy(dp, "Y-%m-%d");
                dp += 7;
                cp++;
                break;

            case 'h':
                *dp++ = 'b';
                cp++;
                break;

            case 'k':
                dp--;
                if (tp->tm_hour < 10) {
                    *dp++ = ' ';
                }
                itosbuf(dp, size - 1, (int64) tp->tm_hour, 10);
                dp += slen(dp);
                cp++;
                break;

            case 'l':
                dp--;
                value = tp->tm_hour;
                if (value < 10) {
                    *dp++ = ' ';
                }
                if (value > 12) {
                    value -= 12;
                }
                itosbuf(dp, size - 1, (int64) value, 10);
                dp += slen(dp);
                cp++;
                break;

            case 'n':
                dp[-1] = '\n';
                cp++;
                break;

            case 'O':
                /* Skip the 'O' */
                cp++;
                goto again;

            case 'P':
                dp--;
                strcpy(dp, (tp->tm_hour > 11) ? "pm" : "am");
                dp += 2;
                cp++;
                break;

            case 'R':
                strcpy(dp, "H:%M");
                dp += 4;
                cp++;
                break;

            case 'r':
                strcpy(dp, "I:%M:%S %p");
                dp += 10;
                cp++;
                break;

            case 's':
                dp--;
                itosbuf(dp, size, (int64) mprMakeTime(tp) / MS_PER_SEC, 10);
                dp += slen(dp);
                cp++;
                break;

            case 'T':
                strcpy(dp, "H:%M:%S");
                dp += 7;
                cp++;
                break;

            case 't':
                dp[-1] = '\t';
                cp++;
                break;

            case 'u':
                dp--;
                value = tp->tm_wday;
                if (value == 0) {
                    value = 7;
                }
                itosbuf(dp, size, (int64) value, 10);
                dp += slen(dp);
                cp++;
                break;

            case 'v':
                /* Inline '%e' */
                dp--;
                if (tp->tm_mday < 10) {
                    *dp++ = ' ';
                }
                itosbuf(dp, size - 1, (int64) tp->tm_mday, 10);
                dp += slen(dp);
                cp++;
                strcpy(dp, "-%b-%Y");
                dp += 6;
                break;

            case 'z':
                dp--;
                value = mprGetTimeZoneOffset(makeTime(tp)) / (MS_PER_SEC * 60);
                sign = (value < 0) ? "-" : "";
                if (value < 0) {
                    value = -value;
                }
                fmt(dp, size, "%s%02d%02d", sign, value / 60, value % 60);
                dp += slen(dp);
                cp++;
                break;

            default:
                if (strchr(VALID_FMT, (int) *cp) != 0) {
                    *dp++ = *cp++;
                } else {
                    dp--;
                    cp++;
                }
                break;
            }
        } else {
            *dp++ = *cp++;
        }
    }
    *dp = '\0';
    format = localFmt;
    if (*format == '\0') {
        format = "%a %b %d %H:%M:%S %Z %Y";
    }

    if (strftime(buf, sizeof(buf) - 1, format, tp) > 0) {
        buf[sizeof(buf) - 1] = '\0';
        return sclone(buf);
    }
    return 0;
}


#else /* !HAS_STRFTIME */
/*
    This implementation is used only on platforms that don't support strftime. This version is not localized.
 */
static void digits(MprBuf *buf, int count, int fill, int value)
{
    char    tmp[32];
    int     i, j;

    if (value < 0) {
        mprPutCharToBuf(buf, '-');
        value = -value;
    }
    for (i = 0; value && i < count; i++) {
        tmp[i] = '0' + value % 10;
        value /= 10;
    }
    if (fill) {
        for (j = i; j < count; j++) {
            mprPutCharToBuf(buf, fill);
        }
    }
    while (i-- > 0) {
        mprPutCharToBuf(buf, tmp[i]);
    }
}


static char *getTimeZoneName(struct tm *tp)
{
    tzset();
    return sclone(tp->tm_zone);
}


PUBLIC char *mprFormatTm(cchar *format, struct tm *tp)
{
    struct tm       tm;
    MprBuf          *buf;
    char            *zone;
    int             w, value;

    if (format == 0) {
        format = MPR_DEFAULT_DATE;
    }
    if (tp == 0) {
        mprDecodeLocalTime(&tm, mprGetTime());
        tp = &tm;
    }
    if ((buf = mprCreateBuf(64, -1)) == 0) {
        return 0;
    }
    while ((*format != '\0')) {
        if (*format++ != '%') {
            mprPutCharToBuf(buf, format[-1]);
            continue;
        }
    again:
        switch (*format++) {
        case '%' :                                      /* percent */
            mprPutCharToBuf(buf, '%');
            break;

        case '+' :                                      /* date (Mon May 18 23:29:50 PDT 2014) */
            mprPutStringToBuf(buf, abbrevDay[tp->tm_wday]);
            mprPutCharToBuf(buf, ' ');
            mprPutStringToBuf(buf, abbrevMonth[tp->tm_mon]);
            mprPutCharToBuf(buf, ' ');
            digits(buf, 2, ' ', tp->tm_mday);
            mprPutCharToBuf(buf, ' ');
            digits(buf, 2, '0', tp->tm_hour);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_min);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_sec);
            mprPutCharToBuf(buf, ' ');
            zone = getTimeZoneName(tp);
            mprPutStringToBuf(buf, zone);
            mprPutCharToBuf(buf, ' ');
            digits(buf, 4, 0, tp->tm_year + 1900);
            break;

        case 'A' :                                      /* full weekday (Sunday) */
            mprPutStringToBuf(buf, day[tp->tm_wday]);
            break;

        case 'a' :                                      /* abbreviated weekday (Sun) */
            mprPutStringToBuf(buf, abbrevDay[tp->tm_wday]);
            break;

        case 'B' :                                      /* full month (January) */
            mprPutStringToBuf(buf, month[tp->tm_mon]);
            break;

        case 'b' :                                      /* abbreviated month (Jan) */
            mprPutStringToBuf(buf, abbrevMonth[tp->tm_mon]);
            break;

        case 'C' :                                      /* century number (19, 20) */
            digits(buf, 2, '0', (1900 + tp->tm_year) / 100);
            break;

        case 'c' :                                      /* preferred date+time in current locale */
            mprPutStringToBuf(buf, abbrevDay[tp->tm_wday]);
            mprPutCharToBuf(buf, ' ');
            mprPutStringToBuf(buf, abbrevMonth[tp->tm_mon]);
            mprPutCharToBuf(buf, ' ');
            digits(buf, 2, ' ', tp->tm_mday);
            mprPutCharToBuf(buf, ' ');
            digits(buf, 2, '0', tp->tm_hour);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_min);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_sec);
            mprPutCharToBuf(buf, ' ');
            digits(buf, 4, 0, tp->tm_year + 1900);
            break;

        case 'D' :                                      /* mm/dd/yy */
            digits(buf, 2, '0', tp->tm_mon + 1);
            mprPutCharToBuf(buf, '/');
            digits(buf, 2, '0', tp->tm_mday);
            mprPutCharToBuf(buf, '/');
            digits(buf, 2, '0', tp->tm_year - 100);
            break;

        case 'd' :                                      /* day of month (01-31) */
            digits(buf, 2, '0', tp->tm_mday);
            break;

        case 'E':
            /* Skip the 'E' */
            goto again;

        case 'e':                                       /* day of month (1-31). Single digits preceeded by a blank */
            digits(buf, 2, ' ', tp->tm_mday);
            break;

        case 'f':                                       /* milliseconds */
            mprPutStringToBuf(buf, "000");
            break;

        case 'F':                                       /* %m/%d/%y */
            digits(buf, 4, 0, tp->tm_year + 1900);
            mprPutCharToBuf(buf, '-');
            digits(buf, 2, '0', tp->tm_mon + 1);
            mprPutCharToBuf(buf, '-');
            digits(buf, 2, '0', tp->tm_mday);
            break;

        case 'H':                                       /* hour using 24 hour clock (00-23) */
            digits(buf, 2, '0', tp->tm_hour);
            break;

        case 'h':                                       /* Same as %b */
            mprPutStringToBuf(buf, abbrevMonth[tp->tm_mon]);
            break;

        case 'I':                                       /* hour using 12 hour clock (00-01) */
            digits(buf, 2, '0', (tp->tm_hour % 12) ? tp->tm_hour % 12 : 12);
            break;

        case 'j':                                       /* julian day (001-366) */
            digits(buf, 3, '0', tp->tm_yday+1);
            break;

        case 'k':                                       /* hour (0-23). Single digits preceeded by a blank */
            digits(buf, 2, ' ', tp->tm_hour);
            break;

        case 'l':                                       /* hour (1-12). Single digits preceeded by a blank */
            digits(buf, 2, ' ', tp->tm_hour < 12 ? tp->tm_hour : (tp->tm_hour - 12));
            break;

        case 'M':                                       /* minute as a number (00-59) */
            digits(buf, 2, '0', tp->tm_min);
            break;

        case 'm':                                       /* month as a number (01-12) */
            digits(buf, 2, '0', tp->tm_mon + 1);
            break;

        case 'n':                                       /* newline */
            mprPutCharToBuf(buf, '\n');
            break;

        case 'O':
            /* Skip the 'O' */
            goto again;

        case 'p':                                       /* AM/PM */
            mprPutStringToBuf(buf, (tp->tm_hour > 11) ? "PM" : "AM");
            break;

        case 'P':                                       /* am/pm */
            mprPutStringToBuf(buf, (tp->tm_hour > 11) ? "pm" : "am");
            break;

        case 'R':
            digits(buf, 2, '0', tp->tm_hour);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_min);
            break;

        case 'r':
            digits(buf, 2, '0', (tp->tm_hour % 12) ? tp->tm_hour % 12 : 12);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_min);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_sec);
            mprPutCharToBuf(buf, ' ');
            mprPutStringToBuf(buf, (tp->tm_hour > 11) ? "PM" : "AM");
            break;

        case 'S':                                       /* seconds as a number (00-60) */
            digits(buf, 2, '0', tp->tm_sec);
            break;

        case 's':                                       /* seconds since epoch */
            mprPutToBuf(buf, "%d", mprMakeTime(tp) / MS_PER_SEC);
            break;

        case 'T':
            digits(buf, 2, '0', tp->tm_hour);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_min);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_sec);
            break;

        case 't':                                       /* Tab */
            mprPutCharToBuf(buf, '\t');
            break;

        case 'U':                                       /* week number (00-53. Staring with first Sunday */
            w = tp->tm_yday / 7;
            if (tp->tm_yday % 7 > tp->tm_wday) {
                w++;
            }
            digits(buf, 2, '0', w);
            break;

        case 'u':                                       /* Week day (1-7) */
            value = tp->tm_wday;
            if (value == 0) {
                value = 7;
            }
            digits(buf, 1, 0, tp->tm_wday == 0 ? 7 : tp->tm_wday);
            break;

        case 'v':                                       /* %e-%b-%Y */
            digits(buf, 2, ' ', tp->tm_mday);
            mprPutCharToBuf(buf, '-');
            mprPutStringToBuf(buf, abbrevMonth[tp->tm_mon]);
            mprPutCharToBuf(buf, '-');
            digits(buf, 4, '0', tp->tm_year + 1900);
            break;

        case 'W':                                       /* week number (00-53). Staring with first Monday */
            w = (tp->tm_yday + 7 - (tp->tm_wday ?  (tp->tm_wday - 1) : (7 - 1))) / 7;
            digits(buf, 2, '0', w);
            break;

        case 'w':                                       /* day of week (0-6) */
            digits(buf, 1, '0', tp->tm_wday);
            break;

        case 'X':                                       /* preferred time without date */
            digits(buf, 2, '0', tp->tm_hour);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_min);
            mprPutCharToBuf(buf, ':');
            digits(buf, 2, '0', tp->tm_sec);
            break;

        case 'x':                                      /* preferred date without time */
            digits(buf, 2, '0', tp->tm_mon + 1);
            mprPutCharToBuf(buf, '/');
            digits(buf, 2, '0', tp->tm_mday);
            mprPutCharToBuf(buf, '/');
            digits(buf, 2, '0', tp->tm_year + 1900);
            break;

        case 'Y':                                       /* year as a decimal including century (1900) */
            digits(buf, 4, '0', tp->tm_year + 1900);
            break;

        case 'y':                                       /* year without century (00-99) */
            digits(buf, 2, '0', tp->tm_year % 100);
            break;

        case 'Z':                                       /* Timezone */
            zone = getTimeZoneName(tp);
            mprPutStringToBuf(buf, zone);
            break;

        case 'z':
            value = mprGetTimeZoneOffset(makeTime(tp)) / (MS_PER_SEC * 60);
            if (value < 0) {
                mprPutCharToBuf(buf, '-');
                value = -value;
            }
            digits(buf, 2, '0', value / 60);
            digits(buf, 2, '0', value % 60);
            break;

        case 'g':
        case 'G':
        case 'V':
            break;

        default:
            mprPutCharToBuf(buf, '%');
            mprPutCharToBuf(buf, format[-1]);
            break;
        }
    }
    mprAddNullToBuf(buf);
    return sclone(mprGetBufStart(buf));
}
#endif /* HAS_STRFTIME */


/*************************************** Parsing ************************************/

static int lookupSym(cchar *token, int kind)
{
    TimeToken   *tt;

    if ((tt = (TimeToken*) mprLookupKey(MPR->timeTokens, token)) == 0) {
        return -1;
    }
    if (kind != (tt->value & TOKEN_MASK)) {
        return -1;
    }
    return tt->value & ~TOKEN_MASK;
}


static int getNum(char **token, int sep)
{
    int     num;

    if (*token == 0) {
        return 0;
    }

    num = atoi(*token);
    *token = strchr(*token, sep);
    if (*token) {
        *token += 1;
    }
    return num;
}


static int getNumOrSym(char **token, int sep, int kind, int *isAlpah)
{
    char    *cp;
    int     num;

    assert(token && *token);

    if (token == NULL || *token == NULL || **token == '\0') {
        return 0;
    }
    if (isalpha((uchar) **token)) {
        *isAlpah = 1;
        cp = strchr(*token, sep);
        if (cp) {
            *cp++ = '\0';
        }
        num = lookupSym(*token, kind);
        *token = cp;
        return num;
    }
    num = atoi(*token);
    *token = strchr(*token, sep);
    if (*token) {
        *token += 1;
    }
    *isAlpah = 0;
    return num;
}


static void swapDayMonth(struct tm *tp)
{
    int     tmp;

    tmp = tp->tm_mday;
    tp->tm_mday = tp->tm_mon;
    tp->tm_mon = tmp;
}


/*
    Parse the a date/time string according to the given zoneFlags and return the result in *time. Missing date items
    may be provided via the defaults argument.
 */
PUBLIC int mprParseTime(MprTime *time, cchar *dateString, int zoneFlags, struct tm *defaults)
{
    TimeToken       *tt;
    struct tm       tm;
    char            *str, *next, *token, *cp, *sep;
    int64           value;
    int             kind, hour, min, negate, value1, value2, value3, alpha, alpha2, alpha3;
    int             dateSep, offset, zoneOffset, explicitZone, fullYear;

    if (!dateString) {
        dateString = "";
    }
    offset = 0;
    zoneOffset = 0;
    explicitZone = 0;
    sep = ", \t";
    cp = 0;
    next = 0;
    fullYear = 0;

    /*
        Set these mandatory values to -1 so we can tell if they are set to valid values
        WARNING: all the calculations use tm_year with origin 0, not 1900. It is fixed up below.
     */
    tm.tm_year = -MAXINT;
    tm.tm_mon = tm.tm_mday = tm.tm_hour = tm.tm_sec = tm.tm_min = tm.tm_wday = -1;
    tm.tm_min = tm.tm_sec = tm.tm_yday = -1;

    tm.tm_gmtoff = 0;
    tm.tm_zone = 0;

    /*
        Set to -1 to try to determine if DST is in effect
     */
    tm.tm_isdst = -1;
    str = slower(dateString);

    /*
        Handle ISO dates: 2009-05-21t16:06:05.000z
     */
    if (strchr(str, ' ') == 0 && strchr(str, '-') && str[slen(str) - 1] == 'z') {
        for (cp = str; *cp; cp++) {
            if (*cp == '-') {
                *cp = '/';
            } else if (*cp == 't' && cp > str && isdigit((uchar) cp[-1]) && isdigit((uchar) cp[1]) ) {
                *cp = ' ';
            }
        }
    }
    token = stok(str, sep, &next);

    while (token && *token) {
        if (snumber(token)) {
            /*
                Parse either day of month or year. Priority to day of month. Format: <29> Jan <15> <2014>
             */
            value = stoi(token);
            if (value > 3000) {
                *time = value;
                return 0;
            } else if (value > 32 || (tm.tm_mday >= 0 && tm.tm_year == -MAXINT)) {
                if (value >= 1000) {
                    fullYear = 1;
                }
                tm.tm_year = (int) value - 1900;
            } else if (tm.tm_mday < 0) {
                tm.tm_mday = (int) value;
            }

        } else if (strncmp(token, "gmt", 3) == 0 || strncmp(token, "utc", 3) == 0) {
            /*
                Timezone format: GMT|UTC[+-]NN[:]NN
                GMT-08:00, UTC-0800
             */
            token += 3;
            if (token[0] == '-' || token[0] == '+') {
                negate = *token == '-' ? -1 : 1;
                token++;
            } else {
                negate = 1;
            }
            if (schr(token, ':')) {
                hour = getNum(&token, timeSep);
                min = getNum(&token, timeSep);
            } else {
                hour = atoi(token);
                min = hour % 100;
                hour /= 100;
            }
            zoneOffset = negate * (hour * 60 + min);
            explicitZone = 1;

        } else if (isalpha((uchar) *token)) {
            if ((tt = (TimeToken*) mprLookupKey(MPR->timeTokens, token)) != 0) {
                kind = tt->value & TOKEN_MASK;
                value = tt->value & ~TOKEN_MASK;
                switch (kind) {

                case TOKEN_DAY:
                    tm.tm_wday = (int) value;
                    break;

                case TOKEN_MONTH:
                    tm.tm_mon = (int) value;
                    break;

                case TOKEN_OFFSET:
                    /* Named timezones or symbolic names like: tomorrow, yesterday, next week ... */
                    /* Units are seconds */
                    offset += (int) value;
                    break;

                case TOKEN_ZONE:
                    zoneOffset = (int) value;
                    explicitZone = 1;
                    break;

                default:
                    /* Just ignore unknown values */
                    break;
                }
            }

        } else if ((cp = strchr(token, timeSep)) != 0 && isdigit((uchar) token[0])) {
            /*
                Time:  10:52[:23]
                Must not parse GMT-07:30
             */
            tm.tm_hour = getNum(&token, timeSep);
            tm.tm_min = getNum(&token, timeSep);
            tm.tm_sec = getNum(&token, timeSep);

        } else {
            dateSep = '/';
            if (strchr(token, dateSep) == 0) {
                dateSep = '-';
                if (strchr(token, dateSep) == 0) {
                    dateSep = '.';
                    if (strchr(token, dateSep) == 0) {
                        dateSep = 0;
                    }
                }
            }
            if (dateSep) {
                /*
                    Date:  07/28/2014, 07/28/08, Jan/28/2014, Jaunuary-28-2014, 28-jan-2014
                    Support order: dd/mm/yy, mm/dd/yy and yyyy/mm/dd
                    Support separators "/", ".", "-"
                 */
                value1 = getNumOrSym(&token, dateSep, TOKEN_MONTH, &alpha);
                value2 = getNumOrSym(&token, dateSep, TOKEN_MONTH, &alpha2);
                value3 = getNumOrSym(&token, dateSep, TOKEN_MONTH, &alpha3);

                if (value1 > 31) {
                    /* yy/mm/dd */
                    tm.tm_year = value1;
                    tm.tm_mon = value2;
                    tm.tm_mday = value3;

                } else if (value1 > 12 || alpha2) {
                    /*
                        dd/mm/yy
                        Cannot detect 01/02/03  This will be evaluated as Jan 2 2003 below.
                     */
                    tm.tm_mday = value1;
                    tm.tm_mon = value2;
                    tm.tm_year = value3;

                } else {
                    /*
                        The default to parse is mm/dd/yy unless the mm value is out of range
                     */
                    tm.tm_mon = value1;
                    tm.tm_mday = value2;
                    tm.tm_year = value3;
                }
            }
        }
        token = stok(NULL, sep, &next);
    }

    /*
        Y2K fix and rebias
     */
    if (0 <= tm.tm_year && tm.tm_year < 100 && !fullYear) {
        if (tm.tm_year < 50) {
            tm.tm_year += 2000;
        } else {
            tm.tm_year += 1900;
        }
    }
    if (tm.tm_year >= 1900) {
        tm.tm_year -= 1900;
    }

    /*
        Convert back to origin 0 for months
     */
    if (tm.tm_mon > 0) {
        tm.tm_mon--;
    }

    /*
        Validate and fill in missing items with defaults
     */
    validateTime(&tm, defaults);

    if (zoneFlags == MPR_LOCAL_TIMEZONE && !explicitZone) {
        *time = mprMakeTime(&tm);
    } else {
        *time = mprMakeUniversalTime(&tm);
        *time += -(zoneOffset * MS_PER_MIN);
    }
    *time += (offset * MS_PER_SEC);
    return 0;
}


static void validateTime(struct tm *tp, struct tm *defaults)
{
    struct tm   empty;

    /*
        Fix apparent day-mon-year ordering issues. Cannot fix everything!
     */
    if ((12 <= tp->tm_mon && tp->tm_mon <= 31) && 0 <= tp->tm_mday && tp->tm_mday <= 11) {
        /*
            Looks like day month are swapped
         */
        swapDayMonth(tp);
    }

    /*
        Check for overflow. Underflow validated below.
        tm_sec can be 61 for leap seconds.
     */
    if (tp->tm_sec > 61) {
        tp->tm_sec = -1;
    }
    if (tp->tm_min > 59) {
        tp->tm_sec = -1;
    }
    if (tp->tm_hour > 23) {
        tp->tm_sec = -1;
    }
    if (tp->tm_mday > 31) {
        tp->tm_sec = -1;
    }
    if (tp->tm_mon > 11) {
        tp->tm_sec = -1;
    }
    if (tp->tm_wday > 6) {
        tp->tm_sec = -1;
    }
    if (tp->tm_yday > 366) {
        tp->tm_sec = -1;
    }

    /*
        Use empty time if defaults missing
     */
    if (defaults == NULL) {
        memset(&empty, 0, sizeof(empty));
        defaults = &empty;
        empty.tm_mday = 1;
        empty.tm_year = 70;
    }
    if (tp->tm_hour < 0 && tp->tm_min < 0 && tp->tm_sec < 0) {
        tp->tm_hour = defaults->tm_hour;
        tp->tm_min = defaults->tm_min;
        tp->tm_sec = defaults->tm_sec;
    }

    /*
        Get weekday, if before today then make next week
     */
    if (tp->tm_wday >= 0 && tp->tm_year == -MAXINT && tp->tm_mon < 0 && tp->tm_mday < 0) {
        tp->tm_mday = defaults->tm_mday + (tp->tm_wday - defaults->tm_wday + 7) % 7;
        tp->tm_mon = defaults->tm_mon;
        tp->tm_year = defaults->tm_year;
    }

    /*
        Get month, if before this month then make next year
     */
    if (tp->tm_mon >= 0 && tp->tm_mon <= 11 && tp->tm_mday < 0) {
        if (tp->tm_year == -MAXINT) {
            tp->tm_year = defaults->tm_year + (((tp->tm_mon - defaults->tm_mon) < 0) ? 1 : 0);
        }
        tp->tm_mday = defaults->tm_mday;
    }

    /*
        Get date, if before current time then make tomorrow
     */
    if (tp->tm_hour >= 0 && tp->tm_year == -MAXINT && tp->tm_mon < 0 && tp->tm_mday < 0) {
        tp->tm_mday = defaults->tm_mday + ((tp->tm_hour - defaults->tm_hour) < 0 ? 1 : 0);
        tp->tm_mon = defaults->tm_mon;
        tp->tm_year = defaults->tm_year;
    }
    if (tp->tm_year == -MAXINT) {
        tp->tm_year = defaults->tm_year;
    }
    if (tp->tm_mon < 0) {
        tp->tm_mon = defaults->tm_mon;
    }
    if (tp->tm_mday < 0) {
        tp->tm_mday = defaults->tm_mday;
    }
    if (tp->tm_yday < 0) {
        if (tp->tm_mon <= 11) {
            tp->tm_yday = (leapYear(tp->tm_year + 1900) ?
                leapMonthStart[tp->tm_mon] : normalMonthStart[tp->tm_mon]) + tp->tm_mday - 1;
        } else {
            tp->tm_yday = defaults->tm_yday;
        }
    }
    if (tp->tm_hour < 0) {
        tp->tm_hour = defaults->tm_hour;
    }
    if (tp->tm_min < 0) {
        tp->tm_min = defaults->tm_min;
    }
    if (tp->tm_sec < 0) {
        tp->tm_sec = defaults->tm_sec;
    }
}





