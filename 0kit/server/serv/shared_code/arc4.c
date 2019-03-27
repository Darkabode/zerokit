#ifndef _ALL_IN_ONE_MODE
#include "platform.h"
#include "types.h"
#include "arc4.h"
#endif // _ALL_IN_ONE_MODE

void arc4_setup(parc4_context_t pContext, const uint8_t* key, uint32_t keylen)
{
    uint32_t k;
    int i, j, a;
    uint8_t *m;
	USE_GLOBAL_BLOCK

    pContext->x = 0;
    pContext->y = 0;
    m = pContext->m;

    for (i = 0; i < 256; i++)
        m[i] = (uint8_t)i;

    j = k = 0;

    for (i = 0; i < 256; i++, k++) {
        if (k >= keylen)
			k = 0;

        a = m[i];
        j = (j + a + key[k]) & 0xFF;
        m[i] = m[j];
        m[j] = (uint8_t) a;
    }
}

void arc4_crypt(parc4_context_t pContext, uint32_t length, const uint8_t* input, uint8_t* output)
{
    uint32_t i;
    int x, y, a, b;
    uint8_t* m;
	USE_GLOBAL_BLOCK

    x = pContext->x;
    y = pContext->y;
    m = pContext->m;

    for (i = 0; i < length; ++i) {
        x = (x + 1) & 0xFF; a = m[x];
        y = (y + a) & 0xFF; b = m[y];

        m[x] = (uint8_t) b;
        m[y] = (uint8_t) a;

        output[i] = (uint8_t)(input[i] ^ m[(uint8_t)( a + b )]);
    }

    pContext->x = x;
    pContext->y = y;
}

void arc4_crypt_self(uint8_t* buffer, uint32_t length, const uint8_t* key, uint32_t keylen)
{
    uint32_t i, j = 0, k = 0;
    int a, b;
    uint8_t m[256];
    USE_GLOBAL_BLOCK

    for (i = 0; i < 256; i++)
        m[i] = (uint8_t)i;

    for (i = 0; i < 256; i++, k++) {
        if (k >= keylen)
            k = 0;

        a = m[i];
        j = (j + a + key[k]) & 0xFF;
        m[i] = m[j];
        m[j] = (uint8_t)a;
    }

    k = j = 0;

    for (i = 0; i < length; ++i) {
        k = (k + 1) & 0xFF;
        a = m[k];
        j = (j + a) & 0xFF;
        b = m[j];

        m[k] = (uint8_t)b;
        m[j] = (uint8_t)a;

        buffer[i] = (uint8_t)(buffer[i] ^ m[(uint8_t)(a + b)]);
    }
}
