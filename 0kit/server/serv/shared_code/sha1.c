/*
Описание:

	Данный файл содежит SHA1 хеш функции.
   
	Перед подключением файла нужно определить следующие функции специфические для среды исполнения:
	 * HASH_PADDING - 64-байтный массив.
	 * MEMSET(dest, val, size) - Инициализация данных значением val.
	 * MEMCPY(dest, src, size) - копирование из источника в приёмник.
  
Зависимости:

   * shared_code/types.h
*/

// 32-bit integer manipulation macros (big endian).
#ifndef GET_ULONG_BE
#define GET_ULONG_BE(n,b,i)                             \
{                                                       \
    (n) = ( (ulong_t) (b)[(i)    ] << 24 )        \
        | ( (ulong_t) (b)[(i) + 1] << 16 )        \
        | ( (ulong_t) (b)[(i) + 2] <<  8 )        \
        | ( (ulong_t) (b)[(i) + 3]       );       \
}
#endif

#ifndef PUT_ULONG_BE
#define PUT_ULONG_BE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (uint8_t) ( (n) >> 24 );       \
    (b)[(i) + 1] = (uint8_t) ( (n) >> 16 );       \
    (b)[(i) + 2] = (uint8_t) ( (n) >>  8 );       \
    (b)[(i) + 3] = (uint8_t) ( (n)       );       \
}
#endif

// SHA-1 context setup.
void sha1_start(sha1_context_t* ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
}

void sha1_process(sha1_context_t* ctx, const uint8_t data[64])
{
    ulong_t temp, W[16], A, B, C, D, E;

    GET_ULONG_BE( W[ 0], data,  0 );
    GET_ULONG_BE( W[ 1], data,  4 );
    GET_ULONG_BE( W[ 2], data,  8 );
    GET_ULONG_BE( W[ 3], data, 12 );
    GET_ULONG_BE( W[ 4], data, 16 );
    GET_ULONG_BE( W[ 5], data, 20 );
    GET_ULONG_BE( W[ 6], data, 24 );
    GET_ULONG_BE( W[ 7], data, 28 );
    GET_ULONG_BE( W[ 8], data, 32 );
    GET_ULONG_BE( W[ 9], data, 36 );
    GET_ULONG_BE( W[10], data, 40 );
    GET_ULONG_BE( W[11], data, 44 );
    GET_ULONG_BE( W[12], data, 48 );
    GET_ULONG_BE( W[13], data, 52 );
    GET_ULONG_BE( W[14], data, 56 );
    GET_ULONG_BE( W[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
    temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^     \
           W[(t - 14) & 0x0F] ^ W[ t      & 0x0F],      \
    ( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a,b,c,d,e,x)                                  \
{                                                       \
    e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

    P( A, B, C, D, E, W[0]  );
    P( E, A, B, C, D, W[1]  );
    P( D, E, A, B, C, W[2]  );
    P( C, D, E, A, B, W[3]  );
    P( B, C, D, E, A, W[4]  );
    P( A, B, C, D, E, W[5]  );
    P( E, A, B, C, D, W[6]  );
    P( D, E, A, B, C, W[7]  );
    P( C, D, E, A, B, W[8]  );
    P( B, C, D, E, A, W[9]  );
    P( A, B, C, D, E, W[10] );
    P( E, A, B, C, D, W[11] );
    P( D, E, A, B, C, W[12] );
    P( C, D, E, A, B, W[13] );
    P( B, C, D, E, A, W[14] );
    P( A, B, C, D, E, W[15] );
    P( E, A, B, C, D, R(16) );
    P( D, E, A, B, C, R(17) );
    P( C, D, E, A, B, R(18) );
    P( B, C, D, E, A, R(19) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

    P( A, B, C, D, E, R(20) );
    P( E, A, B, C, D, R(21) );
    P( D, E, A, B, C, R(22) );
    P( C, D, E, A, B, R(23) );
    P( B, C, D, E, A, R(24) );
    P( A, B, C, D, E, R(25) );
    P( E, A, B, C, D, R(26) );
    P( D, E, A, B, C, R(27) );
    P( C, D, E, A, B, R(28) );
    P( B, C, D, E, A, R(29) );
    P( A, B, C, D, E, R(30) );
    P( E, A, B, C, D, R(31) );
    P( D, E, A, B, C, R(32) );
    P( C, D, E, A, B, R(33) );
    P( B, C, D, E, A, R(34) );
    P( A, B, C, D, E, R(35) );
    P( E, A, B, C, D, R(36) );
    P( D, E, A, B, C, R(37) );
    P( C, D, E, A, B, R(38) );
    P( B, C, D, E, A, R(39) );

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

    P( A, B, C, D, E, R(40) );
    P( E, A, B, C, D, R(41) );
    P( D, E, A, B, C, R(42) );
    P( C, D, E, A, B, R(43) );
    P( B, C, D, E, A, R(44) );
    P( A, B, C, D, E, R(45) );
    P( E, A, B, C, D, R(46) );
    P( D, E, A, B, C, R(47) );
    P( C, D, E, A, B, R(48) );
    P( B, C, D, E, A, R(49) );
    P( A, B, C, D, E, R(50) );
    P( E, A, B, C, D, R(51) );
    P( D, E, A, B, C, R(52) );
    P( C, D, E, A, B, R(53) );
    P( B, C, D, E, A, R(54) );
    P( A, B, C, D, E, R(55) );
    P( E, A, B, C, D, R(56) );
    P( D, E, A, B, C, R(57) );
    P( C, D, E, A, B, R(58) );
    P( B, C, D, E, A, R(59) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

    P( A, B, C, D, E, R(60) );
    P( E, A, B, C, D, R(61) );
    P( D, E, A, B, C, R(62) );
    P( C, D, E, A, B, R(63) );
    P( B, C, D, E, A, R(64) );
    P( A, B, C, D, E, R(65) );
    P( E, A, B, C, D, R(66) );
    P( D, E, A, B, C, R(67) );
    P( C, D, E, A, B, R(68) );
    P( B, C, D, E, A, R(69) );
    P( A, B, C, D, E, R(70) );
    P( E, A, B, C, D, R(71) );
    P( D, E, A, B, C, R(72) );
    P( C, D, E, A, B, R(73) );
    P( B, C, D, E, A, R(74) );
    P( A, B, C, D, E, R(75) );
    P( E, A, B, C, D, R(76) );
    P( D, E, A, B, C, R(77) );
    P( C, D, E, A, B, R(78) );
    P( B, C, D, E, A, R(79) );

#undef K
#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
}

// SHA-1 process buffer.
void sha1_update(sha1_context_t* ctx, const uint8_t* input, size_t ilen)
{
    size_t fill;
    ulong_t left;
	USE_GLOBAL_BLOCK

    if (ilen <= 0)
        return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (ulong_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (ulong_t)ilen)
        ctx->total[1]++;

    if (left && ilen >= fill) {
        MEMCPY((void*)(ctx->buffer + left), (void*)input, fill);
        SHA1_PROCESS(ctx, ctx->buffer);
        input += fill;
        ilen -= fill;
        left = 0;
    }

    while (ilen >= 64) {
        SHA1_PROCESS(ctx, input);
        input += 64;
        ilen -= 64;
    }

    if (ilen > 0) {
        MEMCPY((void*)(ctx->buffer + left), (void*)input, ilen);
    }
}

#ifndef HASH_PADDING

static const unsigned char sha1_padding[64] =
{
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define HASH_PADDING sha1_padding

#endif // HASH_PADDING

// SHA-1 final digest.
void sha1_finish(sha1_context_t *ctx, uint8_t output[20])
{
    ulong_t last, padn;
    ulong_t high, low;
    uint8_t msglen[8];
	USE_GLOBAL_BLOCK

    high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
    low  = ctx->total[0] << 3;

    PUT_ULONG_BE(high, msglen, 0);
    PUT_ULONG_BE(low, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    SHA1_UPDATE(ctx, (uint8_t*)HASH_PADDING, padn);
    SHA1_UPDATE(ctx, msglen, 8);

    PUT_ULONG_BE(ctx->state[0], output, 0);
    PUT_ULONG_BE(ctx->state[1], output, 4);
    PUT_ULONG_BE(ctx->state[2], output, 8);
    PUT_ULONG_BE(ctx->state[3], output, 12);
    PUT_ULONG_BE(ctx->state[4], output, 16);
}

// output = SHA-1(input buffer).
void sha1(const uint8_t* input, size_t ilen, uint8_t output[20])
{
    sha1_context_t ctx;
	USE_GLOBAL_BLOCK

    SHA1_START(&ctx);
    SHA1_UPDATE(&ctx, input, ilen);
    SHA1_FINISH(&ctx, output);

    MEMSET(&ctx, 0, sizeof(sha1_context_t));
}
// 
// // SHA-1 HMAC context setup.
// void sha1_hmac_start(sha1_context_t* ctx, const uint8_t* key, size_t keylen)
// {
//     size_t i;
//     uint8_t sum[20];
// 	USE_GLOBAL_BLOCK
// 
//     if (keylen > 64) {
//         sha1(key, keylen, sum);
//         keylen = 20;
//         key = sum;
//     }
// 
//     MEMSET(ctx->ipad, 0x36, 64);
//     MEMSET(ctx->opad, 0x5C, 64);
// 
//     for (i = 0; i < keylen; ++i) {
//         ctx->ipad[i] = (uint8_t)(ctx->ipad[i] ^ key[i]);
//         ctx->opad[i] = (uint8_t)(ctx->opad[i] ^ key[i]);
//     }
// 
//     sha1_start(ctx);
//     sha1_update(ctx, ctx->ipad, 64);
// 
//     MEMSET( sum, 0, sizeof( sum ) );
// }
// 
// /*
//  * SHA-1 HMAC process buffer
//  */
// void sha1_hmac_update( sha1_context_t *ctx, const uint8_t *input, size_t ilen )
// {
// 	USE_GLOBAL_BLOCK
// 
//     sha1_update( ctx, input, ilen );
// }
// 
// /*
//  * SHA-1 HMAC final digest
//  */
// void sha1_hmac_finish( sha1_context_t *ctx, uint8_t output[20] )
// {
//     uint8_t tmpbuf[20];
// 	USE_GLOBAL_BLOCK
// 
//     sha1_finish( ctx, tmpbuf );
//     sha1_start( ctx );
//     sha1_update( ctx, ctx->opad, 64 );
//     sha1_update( ctx, tmpbuf, 20 );
//     sha1_finish( ctx, output );
// 
//     MEMSET( tmpbuf, 0, sizeof( tmpbuf ) );
// }
// 
// /*
//  * SHA1 HMAC context reset
//  */
// void sha1_hmac_reset( sha1_context_t *ctx )
// {
// 	USE_GLOBAL_BLOCK
// 
//     sha1_start( ctx );
//     sha1_update( ctx, ctx->ipad, 64 );
// }
// 
// /*
//  * output = HMAC-SHA-1( hmac key, input buffer )
//  */
// void sha1_hmac( const uint8_t *key, size_t keylen,
//                 const uint8_t *input, size_t ilen,
//                 uint8_t output[20] )
// {
//     sha1_context_t ctx;
// 	USE_GLOBAL_BLOCK
// 
//     sha1_hmac_start(&ctx, key, keylen);
//     sha1_hmac_update(&ctx, input, ilen);
//     sha1_hmac_finish(&ctx, output);
// 
//     MEMSET(&ctx, 0, sizeof(sha1_context_t));
// }
