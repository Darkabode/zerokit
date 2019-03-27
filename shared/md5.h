#ifndef __SHARED_MD5_H_
#define __SHARED_MD5_H_

typedef struct _md5_context
{
    ulong_t total[2];    // number of bytes processed.
    ulong_t state[4];    // intermediate digest state.
    uint8_t buffer[64];        // data block being processed.

    uint8_t ipad[64];        // HMAC: inner padding.
    uint8_t opad[64];        // HMAC: outer padding.
} md5_context_t, *pmd5_context_t;

#ifndef MD5_PROCESS
#define MD5_PROCESS md5_process
#endif // MD5_PROCESS

#ifndef MD5_START
#define MD5_START md5_start
#endif // MD5_START

#ifndef MD5_UPDATE
#define MD5_UPDATE md5_update
#endif // MD5_UPDATE

#ifndef MD5_FINISH
#define MD5_FINISH md5_finish
#endif // MD5_FINISH

#ifndef MD5_MD5
#define MD5_MD5 md5
#endif // MD5_MD5

#endif // __SHARED_MD5_H_
