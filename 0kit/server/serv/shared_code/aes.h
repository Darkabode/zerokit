#ifndef __SHARED_AES_H_
#define __SHARED_AES_H_

#define AES_ENCRYPT 1
#define AES_DECRYPT 0

#define ERR_AES_INVALID_KEY_LENGTH      -0x0800
#define ERR_AES_INVALID_INPUT_LENGTH    -0x0810

typedef struct _aes_context
{
    int nr;                 // number of rounds
    ulong_t *rk;      // AES round keys
    ulong_t buf[68];  // unaligned data
} aes_context_t, *paes_context_t;

#ifndef AES_GEN_TABLES
#define AES_GEN_TABLES aes_gen_tables
#endif // AES_GEN_TABLES

#ifndef AES_SETKEY_ENC
#define AES_SETKEY_ENC aes_setkey_enc
#endif // AES_SETKEY_ENC

#ifndef AES_SETKEY_DEC
#define AES_SETKEY_DEC aes_setkey_dec
#endif // AES_SETKEY_DEC

#ifndef AES_CRYPT_ECB
#define AES_CRYPT_ECB aes_crypt_ecb
#endif // AES_CRYPT_ECB

#ifndef AES_CRYPT_CBC
#define AES_CRYPT_CBC aes_crypt_cbc
#endif // AES_CRYPT_CBC

#ifndef AES_CRYPT_CFB128
#define AES_CRYPT_CFB128 aes_crypt_cfb128
#endif // AES_CRYPT_CFB128

#endif // __SHARED_AES_H_
