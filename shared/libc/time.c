#include <sys/types.h>
#include <Windows.h>
#include <sys/timeb.h>
#include <time.h>
#include <errno.h>


/*
 * Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
 */
#define EPOCH_BIAS  116444736000000000i64

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
        unsigned __int64 ft_scalar;
        FILETIME ft_struct;
        } FT;

/*
 * Cache for the minutes count for with DST status was last assessed
 */
static __time64_t elapsed_minutes_cache;

/*
 * Three values of dstflag_cache
 */
#define DAYLIGHT_TIME   1
#define STANDARD_TIME   0
#define UNKNOWN_TIME    -1

/*
 * Cache for the last determined DST status
 */
static int dstflag_cache = UNKNOWN_TIME;

long ___timezone = 8 * 3600L; /* Pacific Time Zone */


errno_t __cdecl _get_timezone(long * _Timezone)
{
    /* This variable is correctly inited at startup, so no need to check if CRT init finished */
    *_Timezone = ___timezone;
    return 0;
}

errno_t __cdecl _ftime64_s (struct __timeb64 *tp)
{
    FT nt_time;
    __time64_t t;
    TIME_ZONE_INFORMATION tzinfo;
    DWORD tzstate;
    long timezone = 0;

//    __tzset();

    _get_timezone(&timezone);
    tp->timezone = (short)(timezone / 60);

    GetSystemTimeAsFileTime( &(nt_time.ft_struct) );

    /*
     * Obtain the current DST status. Note the status is cached and only
     * updated once per minute, if necessary.
     */
    if ( (t = (__time64_t)(nt_time.ft_scalar / 600000000i64))
         != elapsed_minutes_cache )
    {
        if ( (tzstate = GetTimeZoneInformation( &tzinfo )) != 0xFFFFFFFF )
        {
            /*
             * Must be very careful in determining whether or not DST is
             * really in effect.
             */
            if ( (tzstate == TIME_ZONE_ID_DAYLIGHT) &&
                 (tzinfo.DaylightDate.wMonth != 0) &&
                 (tzinfo.DaylightBias != 0) )
                dstflag_cache = DAYLIGHT_TIME;
            else
                /*
                 * When in doubt, assume standard time
                 */
                dstflag_cache = STANDARD_TIME;
        }
        else
            dstflag_cache = UNKNOWN_TIME;

        elapsed_minutes_cache = t;
    }

    tp->dstflag = (short)dstflag_cache;

    tp->millitm = (unsigned short)((nt_time.ft_scalar / 10000i64) %
                  1000i64);

    tp->time = (__time64_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);

    return 0;
}

void __cdecl _ftime64(struct __timeb64 *tp)
{
    _ftime64_s(tp);
}

#define _MAX__TIME64_T 0x793406fffi64 

__time64_t __cdecl _time64 (__time64_t *timeptr)
{
    __time64_t tim;
    FT nt_time;

    GetSystemTimeAsFileTime( &(nt_time.ft_struct) );

    tim = (__time64_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);

    if (tim > _MAX__TIME64_T)
        tim = (__time64_t)(-1);

    if (timeptr)
        *timeptr = tim;         /* store time if requested */

    return tim;
}
