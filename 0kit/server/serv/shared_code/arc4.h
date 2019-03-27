#ifndef __SHARED_ARC4_H_
#define __SHARED_ARC4_H_

typedef struct _arc4_context
{
    int x;			// permutation index.
    int y;			// permutation index.
    uint8_t m[256];	// permutation table.
} arc4_context_t, *parc4_context_t;

#ifdef _ALL_IN_ONE_MODE

#ifndef ARC4_SETUP
#define ARC4_SETUP arc4_setup
#endif // ARC4_SETUP

#ifndef ARC4_CRYPT
#define ARC4_CRYPT arc4_crypt
#endif // ARC4_CRYPT

#ifndef ARC4_CRYPT_SELF
#define ARC4_CRYPT_SELF arc4_crypt_self
#endif // ARC4_CRYPT_SELF

#else

void arc4_setup(parc4_context_t pContext, const uint8_t* key, uint32_t keylen);
void arc4_crypt(parc4_context_t pContext, uint32_t length, const uint8_t* input, uint8_t* output);
void arc4_crypt_self(uint8_t* buffer, uint32_t length, const uint8_t* key, uint32_t keylen);

#endif // _ALL_IN_ONE_MODE

#endif // __SHARED_ARC4_H_
