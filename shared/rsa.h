#ifndef __SHARED_RSA_H_
#define __SHARED_RSA_H_

/*
 * RSA Error codes
 */
#define ERR_RSA_BAD_INPUT_DATA    -7    // Bad input parameters to function.
#define ERR_RSA_INVALID_PADDING    -8    // Input data contains invalid padding and is rejected.
#define ERR_RSA_VERIFY_FAILED    -9    // The PKCS#1 verification failed.

#define POLARSSL_ERR_RSA_KEY_GEN_FAILED                    -0x4180  /**< Something failed during generation of a key. */
#define POLARSSL_ERR_RSA_KEY_CHECK_FAILED                  -0x4200  /**< Key failed to pass the libraries validity check. */
#define POLARSSL_ERR_RSA_PUBLIC_FAILED                     -0x4280  /**< The public key operation failed. */
#define POLARSSL_ERR_RSA_PRIVATE_FAILED                    -0x4300  /**< The private key operation failed. */
#define POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE                  -0x4400  /**< The output buffer for decryption is not large enough. */
#define POLARSSL_ERR_RSA_RNG_FAILED                        -0x4480  /**< The random generator failed to generate non-zeros. */

/*
 * PKCS#1 constants
 */
#define SIG_RSA_RAW     0
#define SIG_RSA_SHA1    5


#define RSA_PUBLIC_MODE      0
#define RSA_PRIVATE_MODE     1

#define RSA_PKCS_V15    0
//#define RSA_PKCS_V21    1

#define RSA_SIGN        1
#define RSA_CRYPT       2

#define ASN1_STR_CONSTRUCTED_SEQUENCE   "\x30"
#define ASN1_STR_NULL                   "\x05"
#define ASN1_STR_OID                    "\x06"
#define ASN1_STR_OCTET_STRING           "\x04"

#define OID_DIGEST_ALG_MDX              "\x2A\x86\x48\x86\xF7\x0D\x02\x00"
#define OID_HASH_ALG_SHA1               "\x2b\x0e\x03\x02\x1a"
#define OID_HASH_ALG_SHA2X              "\x60\x86\x48\x01\x65\x03\x04\x02\x00"

#define OID_ISO_MEMBER_BODIES           "\x2a"
#define OID_ISO_IDENTIFIED_ORG          "\x2b"

/*
 * ISO Member bodies OID parts
 */
#define OID_COUNTRY_US                  "\x86\x48"
#define OID_RSA_DATA_SECURITY           "\x86\xf7\x0d"

/*
 * ISO Identified organization OID parts
 */
#define OID_OIW_SECSIG_SHA1             "\x0e\x03\x02\x1a"

/*
 * DigestInfo ::= SEQUENCE {
 *   digestAlgorithm DigestAlgorithmIdentifier,
 *   digest Digest }
 *
 * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
 *
 * Digest ::= OCTET STRING
 */
#ifndef RSA_ANS1_HASH_SHA1
#define RSA_ANS1_HASH_SHA1                      \
    ASN1_STR_CONSTRUCTED_SEQUENCE "\x21"        \
      ASN1_STR_CONSTRUCTED_SEQUENCE "\x09"      \
        ASN1_STR_OID "\x05"                     \
      OID_HASH_ALG_SHA1                         \
        ASN1_STR_NULL "\x00"                    \
      ASN1_STR_OCTET_STRING "\x14"
#endif // RSA_ANS1_HASH_SHA1
/**
 * \brief          RSA context structure
 */
typedef struct _rsa_context
{
    int ver;                    /*!<  always 0          */
    uint32_t len;                 /*!<  size(N) in chars  */

    mpi_t N;                      /*!<  public modulus    */
    mpi_t E;                      /*!<  public exponent   */

    mpi_t D;                      /*!<  private exponent  */
    mpi_t P;                      /*!<  1st prime factor  */
    mpi_t Q;                      /*!<  2nd prime factor  */
    mpi_t DP;                     /*!<  D % (P - 1)       */
    mpi_t DQ;                     /*!<  D % (Q - 1)       */
    mpi_t QP;                     /*!<  1 / (Q % P)       */

    mpi_t RN;                     /*!<  cached R^2 mod N  */
    mpi_t RP;                     /*!<  cached R^2 mod P  */
    mpi_t RQ;                     /*!<  cached R^2 mod Q  */

    int padding;                /*!<  RSA_PKCS_V15 for 1.5 padding and
                                      RSA_PKCS_v21 for OAEP/PSS         */
    int hash_id;                /*!<  Hash identifier of md_type_t as
                                      specified in the md.h header file
                                      for the EME-OAEP and EMSA-PSS
                                      encoding                          */
} rsa_context_t, *prsa_context_t;


#ifndef RSA_INIT
#define RSA_INIT rsa_init
#endif // RSA_INIT

#ifndef RSA_FREE
#define RSA_FREE rsa_free
#endif // RSA_FREE

#ifndef RSA_PUBLIC
#define RSA_PUBLIC rsa_public
#endif // RSA_PUBLIC

#ifndef RSA_PRIVATE
#define RSA_PRIVATE rsa_private
#endif // RSA_PRIVATE

#ifndef RSA_PUBLIC_DECRYPT_HASH
#define RSA_PUBLIC_DECRYPT_HASH rsa_public_decrypt_hash
#endif // RSA_PUBLIC_DECRYPT_HASH

int rsa_pkcs1_sign(rsa_context_t* ctx, int mode, int hash_id, unsigned int hashlen, const uint8_t *hash, uint8_t* sig);

#endif // __SHARED_RSA_H_
