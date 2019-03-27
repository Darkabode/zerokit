#ifndef POLARSSL_TIMING_H
#define POLARSSL_TIMING_H

/**
 * \brief          timer structure
 */
struct hr_time
{
    uint8_t opaque[32];
};

extern int alarmed;

#ifndef FN_HARDCLOCK
#define FN_HARDCLOCK hardclock
#endif // FN_HARDCLOCK

#endif /* timing.h */
