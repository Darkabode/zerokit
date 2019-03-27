#ifndef POLARSSL_X509_H
#define POLARSSL_X509_H

#include "rsa.h"

/** 
 * \addtogroup x509_module
 * \{ 
 */
 
/**
 * \name ASN1 Error codes
 * These error codes are OR'ed to X509 error codes for
 * higher error granularity. 
 * ASN1 is a standard to specify data structures.
 * \{
 */
#define POLARSSL_ERR_ASN1_OUT_OF_DATA                      -0x0014  /**< Out of data when parsing an ASN1 data structure. */
#define POLARSSL_ERR_ASN1_UNEXPECTED_TAG                   -0x0016  /**< ASN1 tag was of an unexpected value. */
#define POLARSSL_ERR_ASN1_INVALID_LENGTH                   -0x0018  /**< Error when trying to determine the length or invalid length. */
#define POLARSSL_ERR_ASN1_LENGTH_MISMATCH                  -0x001A  /**< Actual length differs from expected length. */
#define POLARSSL_ERR_ASN1_INVALID_DATA                     -0x001C  /**< Data is invalid. (not used) */
/* \} name */

/** 
 * \name X509 Error codes
 * \{
 */
#define POLARSSL_ERR_X509_FEATURE_UNAVAILABLE              -0x2080  /**< Unavailable feature, e.g. RSA hashing/encryption combination. */
#define POLARSSL_ERR_X509_CERT_INVALID_PEM                 -0x2100  /**< The PEM-encoded certificate contains invalid elements, e.g. invalid character. */ 
#define POLARSSL_ERR_X509_CERT_INVALID_FORMAT              -0x2180  /**< The certificate format is invalid, e.g. different type expected. */
#define POLARSSL_ERR_X509_CERT_INVALID_VERSION             -0x2200  /**< The certificate version element is invalid. */
#define POLARSSL_ERR_X509_CERT_INVALID_SERIAL              -0x2280  /**< The serial tag or value is invalid. */
#define POLARSSL_ERR_X509_CERT_INVALID_ALG                 -0x2300  /**< The algorithm tag or value is invalid. */
#define POLARSSL_ERR_X509_CERT_INVALID_NAME                -0x2380  /**< The name tag or value is invalid. */
#define POLARSSL_ERR_X509_CERT_INVALID_DATE                -0x2400  /**< The date tag or value is invalid. */
#define POLARSSL_ERR_X509_CERT_INVALID_PUBKEY              -0x2480  /**< The pubkey tag or value is invalid (only RSA is supported). */
#define POLARSSL_ERR_X509_CERT_INVALID_SIGNATURE           -0x2500  /**< The signature tag or value invalid. */
#define POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS          -0x2580  /**< The extension tag or value is invalid. */
#define POLARSSL_ERR_X509_CERT_UNKNOWN_VERSION             -0x2600  /**< Certificate or CRL has an unsupported version number. */
#define POLARSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG             -0x2680  /**< Signature algorithm (oid) is unsupported. */
#define POLARSSL_ERR_X509_UNKNOWN_PK_ALG                   -0x2700  /**< Key algorithm is unsupported (only RSA is supported). */
#define POLARSSL_ERR_X509_CERT_SIG_MISMATCH                -0x2780  /**< Certificate signature algorithms do not match. (see \c ::x509_cert sig_oid) */
#define POLARSSL_ERR_X509_CERT_VERIFY_FAILED               -0x2800  /**< Certificate verification failed, e.g. CRL, CA or signature check failed. */
#define POLARSSL_ERR_X509_KEY_INVALID_VERSION              -0x2880  /**< Unsupported RSA key version */
#define POLARSSL_ERR_X509_KEY_INVALID_FORMAT               -0x2900  /**< Invalid RSA key tag or value. */
#define POLARSSL_ERR_X509_POINT_ERROR                      -0x2980  /**< Not used. */
#define POLARSSL_ERR_X509_VALUE_TO_LENGTH                  -0x2A00  /**< Not used. */
/* \} name */


/**
 * \name X509 Verify codes
 * \{
 */
#define BADCERT_EXPIRED             0x01  /**< The certificate validity has expired. */
#define BADCERT_REVOKED             0x02  /**< The certificate has been revoked (is on a CRL). */
#define BADCERT_CN_MISMATCH         0x04  /**< The certificate Common Name (CN) does not match with the expected CN. */
#define BADCERT_NOT_TRUSTED         0x08  /**< The certificate is not correctly signed by the trusted CA. */
#define BADCRL_NOT_TRUSTED          0x10  /**< CRL is not correctly signed by the trusted CA. */
#define BADCRL_EXPIRED              0x20  /**< CRL is expired. */
#define BADCERT_MISSING             0x40  /**< Certificate was missing. */
#define BADCERT_SKIP_VERIFY         0x80  /**< Certificate verification was skipped. */
/* \} name */


/**
 * \name DER constants
 * These constants comply with DER encoded the ANS1 type tags.
 * DER encoding uses hexadecimal representation.
 * An example DER sequence is:\n
 * - 0x02 -- tag indicating INTEGER
 * - 0x01 -- length in octets
 * - 0x05 -- value
 * Such sequences are typically read into \c ::x509_buf.
 * \{
 */
#define ASN1_BOOLEAN                 0x01
#define ASN1_INTEGER                 0x02
#define ASN1_BIT_STRING              0x03
#define ASN1_OCTET_STRING            0x04
#define ASN1_NULL                    0x05
#define ASN1_OID                     0x06
#define ASN1_UTF8_STRING             0x0C
#define ASN1_SEQUENCE                0x10
#define ASN1_SET                     0x11
#define ASN1_PRINTABLE_STRING        0x13
#define ASN1_T61_STRING              0x14
#define ASN1_IA5_STRING              0x16
#define ASN1_UTC_TIME                0x17
#define ASN1_GENERALIZED_TIME        0x18
#define ASN1_UNIVERSAL_STRING        0x1C
#define ASN1_BMP_STRING              0x1E
#define ASN1_PRIMITIVE               0x00
#define ASN1_CONSTRUCTED             0x20
#define ASN1_CONTEXT_SPECIFIC        0x80
/* \} name */
/* \} addtogroup x509_module */

/*
 * various object identifiers
 */
#define X520_COMMON_NAME                3
#define X520_COUNTRY                    6
#define X520_LOCALITY                   7
#define X520_STATE                      8
#define X520_ORGANIZATION              10
#define X520_ORG_UNIT                  11
#define PKCS9_EMAIL                     1

#define X509_OUTPUT_DER              0x01
#define X509_OUTPUT_PEM              0x02
#define PEM_LINE_LENGTH                72
#define X509_ISSUER                  0x01
#define X509_SUBJECT                 0x02

/** Returns the size of the binary string, without the trailing \\0 */
#define OID_SIZE(x) (sizeof(x) - 1)

#define OID_X520                "\x55\x04"
#define OID_CN                  OID_X520 "\x03"

#define OID_PKCS1               "\x2A\x86\x48\x86\xF7\x0D\x01\x01"
#define OID_PKCS1_RSA           OID_PKCS1 "\x01"

#define OID_RSA_SHA_OBS         "\x2B\x0E\x03\x02\x1D"

#define OID_PKCS9               "\x2A\x86\x48\x86\xF7\x0D\x01\x09"
#define OID_PKCS9_EMAIL         OID_PKCS9 "\x01"

/** ISO arc for standard certificate and CRL extensions */
#define OID_ID_CE               "\x55\x1D" /**< id-ce OBJECT IDENTIFIER  ::=  {joint-iso-ccitt(2) ds(5) 29} */

/**
 * Private Internet Extensions
 * { iso(1) identified-organization(3) dod(6) internet(1)
 *                      security(5) mechanisms(5) pkix(7) }
 */
#define OID_PKIX                "\x2B\x06\x01\x05\x05\x07"

/*
 * OIDs for standard certificate extensions
 */
#define OID_AUTHORITY_KEY_IDENTIFIER    OID_ID_CE "\x23" /**< id-ce-authorityKeyIdentifier OBJECT IDENTIFIER ::=  { id-ce 35 } */
#define OID_SUBJECT_KEY_IDENTIFIER      OID_ID_CE "\x0E" /**< id-ce-subjectKeyIdentifier OBJECT IDENTIFIER ::=  { id-ce 14 } */
#define OID_KEY_USAGE                   OID_ID_CE "\x0F" /**< id-ce-keyUsage OBJECT IDENTIFIER ::=  { id-ce 15 } */
#define OID_CERTIFICATE_POLICIES        OID_ID_CE "\x20" /**< id-ce-certificatePolicies OBJECT IDENTIFIER ::=  { id-ce 32 } */
#define OID_POLICY_MAPPINGS             OID_ID_CE "\x21" /**< id-ce-policyMappings OBJECT IDENTIFIER ::=  { id-ce 33 } */
#define OID_SUBJECT_ALT_NAME            OID_ID_CE "\x11" /**< id-ce-subjectAltName OBJECT IDENTIFIER ::=  { id-ce 17 } */
#define OID_ISSUER_ALT_NAME             OID_ID_CE "\x12" /**< id-ce-issuerAltName OBJECT IDENTIFIER ::=  { id-ce 18 } */
#define OID_SUBJECT_DIRECTORY_ATTRS     OID_ID_CE "\x09" /**< id-ce-subjectDirectoryAttributes OBJECT IDENTIFIER ::=  { id-ce 9 } */
#define OID_BASIC_CONSTRAINTS           OID_ID_CE "\x13" /**< id-ce-basicConstraints OBJECT IDENTIFIER ::=  { id-ce 19 } */
#define OID_NAME_CONSTRAINTS            OID_ID_CE "\x1E" /**< id-ce-nameConstraints OBJECT IDENTIFIER ::=  { id-ce 30 } */
#define OID_POLICY_CONSTRAINTS          OID_ID_CE "\x24" /**< id-ce-policyConstraints OBJECT IDENTIFIER ::=  { id-ce 36 } */
#define OID_EXTENDED_KEY_USAGE          OID_ID_CE "\x25" /**< id-ce-extKeyUsage OBJECT IDENTIFIER ::= { id-ce 37 } */
#define OID_CRL_DISTRIBUTION_POINTS     OID_ID_CE "\x1F" /**< id-ce-cRLDistributionPoints OBJECT IDENTIFIER ::=  { id-ce 31 } */
#define OID_INIHIBIT_ANYPOLICY          OID_ID_CE "\x36" /**< id-ce-inhibitAnyPolicy OBJECT IDENTIFIER ::=  { id-ce 54 } */
#define OID_FRESHEST_CRL                OID_ID_CE "\x2E" /**< id-ce-freshestCRL OBJECT IDENTIFIER ::=  { id-ce 46 } */

/*
 * X.509 v3 Key Usage Extension flags
 */
#define KU_DIGITAL_SIGNATURE            (0x80)  /* bit 0 */
#define KU_NON_REPUDIATION              (0x40)  /* bit 1 */
#define KU_KEY_ENCIPHERMENT             (0x20)  /* bit 2 */
#define KU_DATA_ENCIPHERMENT            (0x10)  /* bit 3 */
#define KU_KEY_AGREEMENT                (0x08)  /* bit 4 */
#define KU_KEY_CERT_SIGN                (0x04)  /* bit 5 */
#define KU_CRL_SIGN                     (0x02)  /* bit 6 */

/*
 * X.509 v3 Extended key usage OIDs
 */
#define OID_ANY_EXTENDED_KEY_USAGE      OID_EXTENDED_KEY_USAGE "\x00" /**< anyExtendedKeyUsage OBJECT IDENTIFIER ::= { id-ce-extKeyUsage 0 } */

#define OID_KP                          OID_PKIX "\x03" /**< id-kp OBJECT IDENTIFIER ::= { id-pkix 3 } */
#define OID_SERVER_AUTH                 OID_KP "\x01" /**< id-kp-serverAuth OBJECT IDENTIFIER ::= { id-kp 1 } */
#define OID_CLIENT_AUTH                 OID_KP "\x02" /**< id-kp-clientAuth OBJECT IDENTIFIER ::= { id-kp 2 } */
#define OID_CODE_SIGNING                OID_KP "\x03" /**< id-kp-codeSigning OBJECT IDENTIFIER ::= { id-kp 3 } */
#define OID_EMAIL_PROTECTION            OID_KP "\x04" /**< id-kp-emailProtection OBJECT IDENTIFIER ::= { id-kp 4 } */
#define OID_TIME_STAMPING               OID_KP "\x08" /**< id-kp-timeStamping OBJECT IDENTIFIER ::= { id-kp 8 } */
#define OID_OCSP_SIGNING                OID_KP "\x09" /**< id-kp-OCSPSigning OBJECT IDENTIFIER ::= { id-kp 9 } */

#define STRING_SERVER_AUTH              "TLS Web Server Authentication"
#define STRING_CLIENT_AUTH              "TLS Web Client Authentication"
#define STRING_CODE_SIGNING             "Code Signing"
#define STRING_EMAIL_PROTECTION         "E-mail Protection"
#define STRING_TIME_STAMPING            "Time Stamping"
#define STRING_OCSP_SIGNING             "OCSP Signing"

/*
 * OIDs for CRL extensions
 */
#define OID_PRIVATE_KEY_USAGE_PERIOD    OID_ID_CE "\x10"
#define OID_CRL_NUMBER                  OID_ID_CE "\x14" /**< id-ce-cRLNumber OBJECT IDENTIFIER ::= { id-ce 20 } */

/*
 * Netscape certificate extensions
 */
#define OID_NETSCAPE                "\x60\x86\x48\x01\x86\xF8\x42" /**< Netscape OID */
#define OID_NS_CERT                 OID_NETSCAPE "\x01"
#define OID_NS_CERT_TYPE            OID_NS_CERT  "\x01"
#define OID_NS_BASE_URL             OID_NS_CERT  "\x02"
#define OID_NS_REVOCATION_URL       OID_NS_CERT  "\x03"
#define OID_NS_CA_REVOCATION_URL    OID_NS_CERT  "\x04"
#define OID_NS_RENEWAL_URL          OID_NS_CERT  "\x07"
#define OID_NS_CA_POLICY_URL        OID_NS_CERT  "\x08"
#define OID_NS_SSL_SERVER_NAME      OID_NS_CERT  "\x0C"
#define OID_NS_COMMENT              OID_NS_CERT  "\x0D"
#define OID_NS_DATA_TYPE            OID_NETSCAPE "\x02"
#define OID_NS_CERT_SEQUENCE        OID_NS_DATA_TYPE "\x05"

/*
 * Netscape certificate types
 * (http://www.mozilla.org/projects/security/pki/nss/tech-notes/tn3.html)
 */

#define NS_CERT_TYPE_SSL_CLIENT         (0x80)  /* bit 0 */
#define NS_CERT_TYPE_SSL_SERVER         (0x40)  /* bit 1 */
#define NS_CERT_TYPE_EMAIL              (0x20)  /* bit 2 */
#define NS_CERT_TYPE_OBJECT_SIGNING     (0x10)  /* bit 3 */
#define NS_CERT_TYPE_RESERVED           (0x08)  /* bit 4 */
#define NS_CERT_TYPE_SSL_CA             (0x04)  /* bit 5 */
#define NS_CERT_TYPE_EMAIL_CA           (0x02)  /* bit 6 */
#define NS_CERT_TYPE_OBJECT_SIGNING_CA  (0x01)  /* bit 7 */

#define EXT_AUTHORITY_KEY_IDENTIFIER    (1 << 0)
#define EXT_SUBJECT_KEY_IDENTIFIER      (1 << 1)
#define EXT_KEY_USAGE                   (1 << 2)
#define EXT_CERTIFICATE_POLICIES        (1 << 3)
#define EXT_POLICY_MAPPINGS             (1 << 4)
#define EXT_SUBJECT_ALT_NAME            (1 << 5)
#define EXT_ISSUER_ALT_NAME             (1 << 6)
#define EXT_SUBJECT_DIRECTORY_ATTRS     (1 << 7)
#define EXT_BASIC_CONSTRAINTS           (1 << 8)
#define EXT_NAME_CONSTRAINTS            (1 << 9)
#define EXT_POLICY_CONSTRAINTS          (1 << 10)
#define EXT_EXTENDED_KEY_USAGE          (1 << 11)
#define EXT_CRL_DISTRIBUTION_POINTS     (1 << 12)
#define EXT_INIHIBIT_ANYPOLICY          (1 << 13)
#define EXT_FRESHEST_CRL                (1 << 14)

#define EXT_NS_CERT_TYPE                (1 << 16)

/** 
 * \addtogroup x509_module
 * \{ */

/**
 * \name Structures for parsing X.509 certificates and CRLs
 * \{
 */
 
/** 
 * Type-length-value structure that allows for ASN1 using DER.
 */
typedef struct _x509_buf
{
    int tag;                /**< ASN1 type, e.g. ASN1_UTF8_STRING. */
    size_t len;             /**< ASN1 length, e.g. in octets. */
    uint8_t *p;       /**< ASN1 data, e.g. in ASCII. */
}
x509_buf;

/**
 * Container for ASN1 bit strings.
 */
typedef struct _x509_bitstring
{
    size_t len;                 /**< ASN1 length, e.g. in octets. */
    uint8_t unused_bits;  /**< Number of unused bits at the end of the string */
    uint8_t *p;           /**< Raw ASN1 data for the bit string */
}
x509_bitstring;

/**
 * Container for ASN1 named information objects. 
 * It allows for Relative Distinguished Names (e.g. cn=polarssl,ou=code,etc.).
 */
typedef struct _x509_name
{
    x509_buf oid;               /**< The object identifier. */
    x509_buf val;               /**< The named value. */
    struct _x509_name *next;    /**< The next named information object. */
}
x509_name;

/**
 * Container for a sequence of ASN.1 items
 */
typedef struct _x509_sequence
{
    x509_buf buf;                   /**< Buffer containing the given ASN.1 item. */
    struct _x509_sequence *next;    /**< The next entry in the sequence. */
}
x509_sequence;

/** Container for date and time (precision in seconds). */
typedef struct _x509_time
{
    int year, mon, day;         /**< Date. */
    int hour, min, sec;         /**< Time. */
}
x509_time;

/** 
 * Container for an X.509 certificate. The certificate may be chained.
 */
typedef struct _x509_cert
{
    x509_buf raw;               /**< The raw certificate data (DER). */
    x509_buf tbs;               /**< The raw certificate body (DER). The part that is To Be Signed. */

    int version;                /**< The X.509 version. (0=v1, 1=v2, 2=v3) */
    x509_buf serial;            /**< Unique id for certificate issued by a specific CA. */
    x509_buf sig_oid1;          /**< Signature algorithm, e.g. sha1RSA */

    x509_buf issuer_raw;        /**< The raw issuer data (DER). Used for quick comparison. */
    x509_buf subject_raw;       /**< The raw subject data (DER). Used for quick comparison. */

    x509_name issuer;           /**< The parsed issuer data (named information object). */
    x509_name subject;          /**< The parsed subject data (named information object). */

    x509_time valid_from;       /**< Start time of certificate validity. */
    x509_time valid_to;         /**< End time of certificate validity. */

    x509_buf pk_oid;            /**< Subject public key info. Includes the public key algorithm and the key itself. */
    rsa_context rsa;            /**< Container for the RSA context. Only RSA is supported for public keys at this time. */

    x509_buf issuer_id;         /**< Optional X.509 v2/v3 issuer unique identifier. */
    x509_buf subject_id;        /**< Optional X.509 v2/v3 subject unique identifier. */
    x509_buf v3_ext;            /**< Optional X.509 v3 extensions. Only Basic Contraints are supported at this time. */

    int ext_types;              /**< Bit string containing detected and parsed extensions */
    int ca_istrue;              /**< Optional Basic Constraint extension value: 1 if this certificate belongs to a CA, 0 otherwise. */
    int max_pathlen;            /**< Optional Basic Constraint extension value: The maximum path length to the root certificate. */

    uint8_t key_usage;    /**< Optional key usage extension value: See the values below */

    x509_sequence ext_key_usage; /**< Optional list of extended key usage OIDs. */

    uint8_t ns_cert_type; /**< Optional Netscape certificate type extension value: See the values below */

    x509_buf sig_oid2;          /**< Signature algorithm. Must match sig_oid1. */
    x509_buf sig;               /**< Signature: hash of the tbs part signed with the private key. */
    int sig_alg;                /**< Internal representation of the signature algorithm, e.g. SIG_RSA_MD2 */

    struct _x509_cert *next;    /**< Next certificate in the CA-chain. */ 
}
x509_cert;

/** 
 * Certificate revocation list entry. 
 * Contains the CA-specific serial numbers and revocation dates.
 */
typedef struct _x509_crl_entry
{
    x509_buf raw;

    x509_buf serial;

    x509_time revocation_date;

    x509_buf entry_ext;

    struct _x509_crl_entry *next;
}
x509_crl_entry;

/** 
 * Certificate revocation list structure. 
 * Every CRL may have multiple entries.
 */
typedef struct _x509_crl
{
    x509_buf raw;           /**< The raw certificate data (DER). */
    x509_buf tbs;           /**< The raw certificate body (DER). The part that is To Be Signed. */

    int version;
    x509_buf sig_oid1;

    x509_buf issuer_raw;    /**< The raw issuer data (DER). */

    x509_name issuer;       /**< The parsed issuer data (named information object). */

    x509_time this_update;  
    x509_time next_update;

    x509_crl_entry entry;   /**< The CRL entries containing the certificate revocation times for this CA. */

    x509_buf crl_ext;

    x509_buf sig_oid2;
    x509_buf sig;
    int sig_alg;

    struct _x509_crl *next; 
} x509_crl;
/** \} name Structures for parsing X.509 certificates and CRLs */
/** \} addtogroup x509_module */

#endif /* x509.h */
