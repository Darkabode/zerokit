#ifndef __SHARED_SHA1_H_
#define __SHARED_SHA1_H_

typedef struct _sha1_context
{
    ulong_t total[2];     /*!< number of bytes processed  */
    ulong_t state[5];     /*!< intermediate digest state  */
    uint8_t buffer[64];   /*!< data block being processed */

    uint8_t ipad[64];     /*!< HMAC: inner padding        */
    uint8_t opad[64];     /*!< HMAC: outer padding        */
} sha1_context_t, *psha1_context_t;

#ifndef SHA1_START
#define SHA1_START sha1_start
#endif // SHA1_START

#ifndef SHA1_PROCESS
#define SHA1_PROCESS sha1_process
#endif // SHA1_PROCESS

#ifndef SHA1_UPDATE
#define SHA1_UPDATE sha1_update
#endif // SHA1_UPDATE

#ifndef SHA1_FINISH
#define SHA1_FINISH sha1_finish
#endif // SHA1_FINISH

#ifndef SHA1_SHA1
#define SHA1_SHA1 sha1
#endif // SHA1_SHA1

#endif // __SHARED_SHA1_H_
