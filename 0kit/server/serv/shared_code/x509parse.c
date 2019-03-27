
/*
 * ASN.1 DER decoding routines
 */
static int asn1_get_len( uint8_t **p,
                         const uint8_t *end,
                         size_t *len )
{
    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( ( **p & 0x80 ) == 0 )
        *len = *(*p)++;
    else
    {
        switch( **p & 0x7F )
        {
        case 1:
            if( ( end - *p ) < 2 )
                return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

            *len = (*p)[1];
            (*p) += 2;
            break;

        case 2:
            if( ( end - *p ) < 3 )
                return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

            *len = ( (*p)[1] << 8 ) | (*p)[2];
            (*p) += 3;
            break;

        default:
            return( POLARSSL_ERR_ASN1_INVALID_LENGTH );
        }
    }

    if( *len > (size_t) ( end - *p ) )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    return( 0 );
}

static int asn1_get_tag( uint8_t **p,
                         const uint8_t *end,
                         size_t *len, int tag )
{
    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( **p != tag )
        return( POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    (*p)++;

    return( asn1_get_len( p, end, len ) );
}

static int asn1_get_bool( uint8_t **p,
                          const uint8_t *end,
                          int *val )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_BOOLEAN ) ) != 0 )
        return( ret );

    if( len != 1 )
        return( POLARSSL_ERR_ASN1_INVALID_LENGTH );

    *val = ( **p != 0 ) ? 1 : 0;
    (*p)++;

    return( 0 );
}

static int asn1_get_int( uint8_t **p,
                         const uint8_t *end,
                         int *val )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_INTEGER ) ) != 0 )
        return( ret );

    if( len > sizeof( int ) || ( **p & 0x80 ) != 0 )
        return( POLARSSL_ERR_ASN1_INVALID_LENGTH );

    *val = 0;

    while( len-- > 0 )
    {
        *val = ( *val << 8 ) | **p;
        (*p)++;
    }

    return( 0 );
}

static int asn1_get_mpi( uint8_t **p,
                         const uint8_t *end,
                         mpi_t *X )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_INTEGER ) ) != 0 )
        return( ret );

    ret = mpi_read_binary( X, *p, len );

    *p += len;

    return( ret );
}

static int asn1_get_bitstring( uint8_t **p, const uint8_t *end,
        x509_bitstring *bs)
{
    int ret;

    /* Certificate type is a single byte bitstring */
    if( ( ret = asn1_get_tag( p, end, &bs->len, ASN1_BIT_STRING ) ) != 0 )
        return( ret );

    /* Check length, subtract one for actual bit string length */
    if ( bs->len < 1 )
        return( POLARSSL_ERR_ASN1_OUT_OF_DATA );
    bs->len -= 1;

    /* Get number of unused bits, ensure unused bits <= 7 */
    bs->unused_bits = **p;
    if( bs->unused_bits > 7 )
        return( POLARSSL_ERR_ASN1_INVALID_LENGTH );
    (*p)++;

    /* Get actual bitstring */
    bs->p = *p;
    *p += bs->len;

    if( *p != end )
        return( POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return 0;
}


/*
 *  Parses and splits an ASN.1 "SEQUENCE OF <tag>"
 */
static int asn1_get_sequence_of( uint8_t **p,
                          const uint8_t *end,
                          x509_sequence *cur,
                          int tag)
{
    int ret;
    size_t len;
    x509_buf *buf;

    /* Get main sequence tag */
    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( ret );

    if( *p + len != end )
        return( POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    while( *p < end )
    {
        buf = &(cur->buf);
        buf->tag = **p;

        if( ( ret = asn1_get_tag( p, end, &buf->len, tag ) ) != 0 )
            return( ret );

        buf->p = *p;
        *p += buf->len;

        /* Allocate and assign next pointer */
        if (*p < end)
        {
            cur->next = (x509_sequence *) malloc(
                 sizeof( x509_sequence ) );

            if( cur->next == NULL )
                return( 1 );

            cur = cur->next;
        }
    }

    /* Set final sequence entry's next pointer to NULL */
    cur->next = NULL;

    if( *p != end )
        return( POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 *  Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
 */
static int x509_get_version( uint8_t **p,
                             const uint8_t *end,
                             int *ver )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0 ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( *ver = 0 );

        return( ret );
    }

    end = *p + len;

    if( ( ret = asn1_get_int( p, end, ver ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_VERSION + ret );

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_VERSION +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 *  CertificateSerialNumber  ::=  INTEGER
 */
static int x509_get_serial( uint8_t **p,
                            const uint8_t *end,
                            x509_buf *serial )
{
    int ret;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_CERT_INVALID_SERIAL +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( **p != ( ASN1_CONTEXT_SPECIFIC | ASN1_PRIMITIVE | 2 ) &&
        **p !=   ASN1_INTEGER )
        return( POLARSSL_ERR_X509_CERT_INVALID_SERIAL +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    serial->tag = *(*p)++;

    if( ( ret = asn1_get_len( p, end, &serial->len ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_SERIAL + ret );

    serial->p = *p;
    *p += serial->len;

    return( 0 );
}

/*
 *  AlgorithmIdentifier  ::=  SEQUENCE  {
 *       algorithm               OBJECT IDENTIFIER,
 *       parameters              ANY DEFINED BY algorithm OPTIONAL  }
 */
static int x509_get_alg( uint8_t **p,
                         const uint8_t *end,
                         x509_buf *alg )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_ALG + ret );

    end = *p + len;
    alg->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &alg->len, ASN1_OID ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_ALG + ret );

    alg->p = *p;
    *p += alg->len;

    if( *p == end )
        return( 0 );

    /*
     * assume the algorithm parameters must be NULL
     */
    if( ( ret = asn1_get_tag( p, end, &len, ASN1_NULL ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_ALG + ret );

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_ALG +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 */
static int x509_get_attr_type_value( uint8_t **p,
                                     const uint8_t *end,
                                     x509_name *cur )
{
    int ret;
    size_t len;
    x509_buf *oid;
    x509_buf *val;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_NAME + ret );

    oid = &cur->oid;
    oid->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &oid->len, ASN1_OID ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_NAME + ret );

    oid->p = *p;
    *p += oid->len;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_CERT_INVALID_NAME +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( **p != ASN1_BMP_STRING && **p != ASN1_UTF8_STRING      &&
        **p != ASN1_T61_STRING && **p != ASN1_PRINTABLE_STRING &&
        **p != ASN1_IA5_STRING && **p != ASN1_UNIVERSAL_STRING )
        return( POLARSSL_ERR_X509_CERT_INVALID_NAME +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    val = &cur->val;
    val->tag = *(*p)++;

    if( ( ret = asn1_get_len( p, end, &val->len ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_NAME + ret );

    val->p = *p;
    *p += val->len;

    cur->next = NULL;

    return( 0 );
}

/*
 *  RelativeDistinguishedName ::=
 *    SET OF AttributeTypeAndValue
 *
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 */
static int x509_get_name( uint8_t **p,
                          const uint8_t *end,
                          x509_name *cur )
{
    int ret;
    size_t len;
    const uint8_t *end2;
    x509_name *use; 
    
    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SET ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_NAME + ret );

    end2 = end;
    end  = *p + len;
    use = cur;

    do
    {
        if( ( ret = x509_get_attr_type_value( p, end, use ) ) != 0 )
            return( ret );
        
        if( *p != end )
        {
            use->next = (x509_name *) malloc(
                    sizeof( x509_name ) );

            if( use->next == NULL )
                return( 1 );
            
            memset( use->next, 0, sizeof( x509_name ) );

            use = use->next;
        }
    }
    while( *p != end );

    /*
     * recurse until end of SEQUENCE is reached
     */
    if( *p == end2 )
        return( 0 );

    cur->next = (x509_name *) malloc(
         sizeof( x509_name ) );

    if( cur->next == NULL )
        return( 1 );

    return( x509_get_name( p, end2, cur->next ) );
}

/*
 *  Time ::= CHOICE {
 *       utcTime        UTCTime,
 *       generalTime    GeneralizedTime }
 */
static int x509_get_time( uint8_t **p,
                          const uint8_t *end,
                          x509_time *time )
{
    int ret;
    size_t len;
    char date[64];
    uint8_t tag;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_CERT_INVALID_DATE +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    tag = **p;

    if ( tag == ASN1_UTC_TIME )
    {
        (*p)++;
        ret = asn1_get_len( p, end, &len );
        
        if( ret != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_DATE + ret );

        memset( date,  0, sizeof( date ) );
        memcpy( date, *p, ( len < sizeof( date ) - 1 ) ?
                len : sizeof( date ) - 1 );

        if( sscanf( date, "%2d%2d%2d%2d%2d%2d",
                    &time->year, &time->mon, &time->day,
                    &time->hour, &time->min, &time->sec ) < 5 )
            return( POLARSSL_ERR_X509_CERT_INVALID_DATE );

        time->year +=  100 * ( time->year < 50 );
        time->year += 1900;

        *p += len;

        return( 0 );
    }
    else if ( tag == ASN1_GENERALIZED_TIME )
    {
        (*p)++;
        ret = asn1_get_len( p, end, &len );
        
        if( ret != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_DATE + ret );

        memset( date,  0, sizeof( date ) );
        memcpy( date, *p, ( len < sizeof( date ) - 1 ) ?
                len : sizeof( date ) - 1 );

        if( sscanf( date, "%4d%2d%2d%2d%2d%2d",
                    &time->year, &time->mon, &time->day,
                    &time->hour, &time->min, &time->sec ) < 5 )
            return( POLARSSL_ERR_X509_CERT_INVALID_DATE );

        *p += len;

        return( 0 );
    }
    else
        return( POLARSSL_ERR_X509_CERT_INVALID_DATE + POLARSSL_ERR_ASN1_UNEXPECTED_TAG );
}


/*
 *  Validity ::= SEQUENCE {
 *       notBefore      Time,
 *       notAfter       Time }
 */
static int x509_get_dates( uint8_t **p,
                           const uint8_t *end,
                           x509_time *from,
                           x509_time *to )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_DATE + ret );

    end = *p + len;

    if( ( ret = x509_get_time( p, end, from ) ) != 0 )
        return( ret );

    if( ( ret = x509_get_time( p, end, to ) ) != 0 )
        return( ret );

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_DATE +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 *  SubjectPublicKeyInfo  ::=  SEQUENCE  {
 *       algorithm            AlgorithmIdentifier,
 *       subjectPublicKey     BIT STRING }
 */
static int x509_get_pubkey( uint8_t **p,
                            const uint8_t *end,
                            x509_buf *pk_alg_oid,
                            mpi_t *N, mpi_t *E )
{
    int ret, can_handle;
    size_t len;
    uint8_t *end2;

    if( ( ret = x509_get_alg( p, end, pk_alg_oid ) ) != 0 )
        return( ret );

    /*
     * only RSA public keys handled at this time
     */
    can_handle = 0;

    if( pk_alg_oid->len == 9 &&
        memcmp( pk_alg_oid->p, OID_PKCS1_RSA, 9 ) == 0 )
        can_handle = 1;

    if( pk_alg_oid->len == 9 &&
        memcmp( pk_alg_oid->p, OID_PKCS1, 8 ) == 0 )
    {
        if( pk_alg_oid->p[8] >= 2 && pk_alg_oid->p[8] <= 5 )
            can_handle = 1;

        if ( pk_alg_oid->p[8] >= 11 && pk_alg_oid->p[8] <= 14 )
            can_handle = 1;
    }

    if( pk_alg_oid->len == 5 &&
        memcmp( pk_alg_oid->p, OID_RSA_SHA_OBS, 5 ) == 0 )
        can_handle = 1;

    if( can_handle == 0 )
        return( POLARSSL_ERR_X509_UNKNOWN_PK_ALG );

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_BIT_STRING ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY + ret );

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    end2 = *p + len;

    if( *(*p)++ != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY );

    /*
     *  RSAPublicKey ::= SEQUENCE {
     *      modulus           INTEGER,  -- n
     *      publicExponent    INTEGER   -- e
     *  }
     */
    if( ( ret = asn1_get_tag( p, end2, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY + ret );

    if( *p + len != end2 )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    if( ( ret = asn1_get_mpi( p, end2, N ) ) != 0 ||
        ( ret = asn1_get_mpi( p, end2, E ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY + ret );

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_PUBKEY +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

static int x509_get_sig( uint8_t **p,
                         const uint8_t *end,
                         x509_buf *sig )
{
    int ret;
    size_t len;

    sig->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &len, ASN1_BIT_STRING ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_SIGNATURE + ret );


    if( --len < 1 || *(*p)++ != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_SIGNATURE );

    sig->len = len;
    sig->p = *p;

    *p += len;

    return( 0 );
}

/*
 * X.509 v2/v3 unique identifier (not parsed)
 */
static int x509_get_uid( uint8_t **p,
                         const uint8_t *end,
                         x509_buf *uid, int n )
{
    int ret;

    if( *p == end )
        return( 0 );

    uid->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &uid->len,
            ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | n ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( 0 );

        return( ret );
    }

    uid->p = *p;
    *p += uid->len;

    return( 0 );
}

/*
 * X.509 Extensions (No parsing of extensions, pointer should
 * be either manually updated or extensions should be parsed!
 */
static int x509_get_ext( uint8_t **p,
                         const uint8_t *end,
                         x509_buf *ext )
{
    int ret;
    size_t len;

    if( *p == end )
        return( 0 );

    ext->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &ext->len,
            ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 3 ) ) != 0 )
        return( ret );

    ext->p = *p;
    end = *p + ext->len;

    /*
     * Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension
     *
     * Extension  ::=  SEQUENCE  {
     *      extnID      OBJECT IDENTIFIER,
     *      critical    BOOLEAN DEFAULT FALSE,
     *      extnValue   OCTET STRING  }
     */
    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

    if( end != *p + len )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 * X.509 CRL v2 extensions (no extensions parsed yet.)
 */
static int x509_get_crl_ext( uint8_t **p,
                             const uint8_t *end,
                             x509_buf *ext )
{
    int ret;
    size_t len;

    if( ( ret = x509_get_ext( p, end, ext ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( 0 );

        return( ret );
    }

    while( *p < end )
    {
        if( ( ret = asn1_get_tag( p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

        *p += len;
    }

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

static int x509_get_basic_constraints( uint8_t **p,
                                       const uint8_t *end,
                                       int *ca_istrue,
                                       int *max_pathlen )
{
    int ret;
    size_t len;

    /*
     * BasicConstraints ::= SEQUENCE {
     *      cA                      BOOLEAN DEFAULT FALSE,
     *      pathLenConstraint       INTEGER (0..MAX) OPTIONAL }
     */
    *ca_istrue = 0; /* DEFAULT FALSE */
    *max_pathlen = 0; /* endless */

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

    if( *p == end )
        return 0;

    if( ( ret = asn1_get_bool( p, end, ca_istrue ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            ret = asn1_get_int( p, end, ca_istrue );

        if( ret != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

        if( *ca_istrue != 0 )
            *ca_istrue = 1;
    }

    if( *p == end )
        return 0;

    if( ( ret = asn1_get_int( p, end, max_pathlen ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    (*max_pathlen)++;

    return 0;
}

static int x509_get_ns_cert_type( uint8_t **p,
                                       const uint8_t *end,
                                       uint8_t *ns_cert_type)
{
    int ret;
    x509_bitstring bs = { 0, 0, NULL };

    if( ( ret = asn1_get_bitstring( p, end, &bs ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

    if( bs.len != 1 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_INVALID_LENGTH );

    /* Get actual bitstring */
    *ns_cert_type = *bs.p;
    return 0;
}

static int x509_get_key_usage( uint8_t **p,
                               const uint8_t *end,
                               uint8_t *key_usage)
{
    int ret;
    x509_bitstring bs = { 0, 0, NULL };

    if( ( ret = asn1_get_bitstring( p, end, &bs ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

    if( bs.len != 1 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_INVALID_LENGTH );

    /* Get actual bitstring */
    *key_usage = *bs.p;
    return 0;
}

/*
 * ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId
 *
 * KeyPurposeId ::= OBJECT IDENTIFIER
 */
static int x509_get_ext_key_usage( uint8_t **p,
                               const uint8_t *end,
                               x509_sequence *ext_key_usage)
{
    int ret;

    if( ( ret = asn1_get_sequence_of( p, end, ext_key_usage, ASN1_OID ) ) != 0 )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

    /* Sequence length must be >= 1 */
    if( ext_key_usage->buf.p == NULL )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_INVALID_LENGTH );

    return 0;
}

/*
 * X.509 v3 extensions
 *
 * TODO: Perform all of the basic constraints tests required by the RFC
 * TODO: Set values for undetected extensions to a sane default?
 *
 */
static int x509_get_crt_ext( uint8_t **p,
                             const uint8_t *end,
                             x509_cert *crt )
{
    int ret;
    size_t len;
    uint8_t *end_ext_data, *end_ext_octet;

    if( ( ret = x509_get_ext( p, end, &crt->v3_ext ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( 0 );

        return( ret );
    }

    while( *p < end )
    {
        /*
         * Extension  ::=  SEQUENCE  {
         *      extnID      OBJECT IDENTIFIER,
         *      critical    BOOLEAN DEFAULT FALSE,
         *      extnValue   OCTET STRING  }
         */
        x509_buf extn_oid = {0, 0, NULL};
        int is_critical = 0; /* DEFAULT FALSE */

        if( ( ret = asn1_get_tag( p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

        end_ext_data = *p + len;

        /* Get extension ID */
        extn_oid.tag = **p;

        if( ( ret = asn1_get_tag( p, end, &extn_oid.len, ASN1_OID ) ) != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

        extn_oid.p = *p;
        *p += extn_oid.len;

        if( ( end - *p ) < 1 )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                    POLARSSL_ERR_ASN1_OUT_OF_DATA );

        /* Get optional critical */
        if( ( ret = asn1_get_bool( p, end_ext_data, &is_critical ) ) != 0 &&
            ( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG ) )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

        /* Data should be octet string type */
        if( ( ret = asn1_get_tag( p, end_ext_data, &len,
                ASN1_OCTET_STRING ) ) != 0 )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS + ret );

        end_ext_octet = *p + len;

        if( end_ext_octet != end_ext_data )
            return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                    POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

        /*
         * Detect supported extensions
         */
        if( ( OID_SIZE( OID_BASIC_CONSTRAINTS ) == extn_oid.len ) &&
                memcmp( extn_oid.p, OID_BASIC_CONSTRAINTS, extn_oid.len ) == 0 )
        {
            /* Parse basic constraints */
            if( ( ret = x509_get_basic_constraints( p, end_ext_octet,
                    &crt->ca_istrue, &crt->max_pathlen ) ) != 0 )
                return ( ret );
            crt->ext_types |= EXT_BASIC_CONSTRAINTS;
        }
        else if( ( OID_SIZE( OID_NS_CERT_TYPE ) == extn_oid.len ) &&
                memcmp( extn_oid.p, OID_NS_CERT_TYPE, extn_oid.len ) == 0 )
        {
            /* Parse netscape certificate type */
            if( ( ret = x509_get_ns_cert_type( p, end_ext_octet,
                    &crt->ns_cert_type ) ) != 0 )
                return ( ret );
            crt->ext_types |= EXT_NS_CERT_TYPE;
        }
        else if( ( OID_SIZE( OID_KEY_USAGE ) == extn_oid.len ) &&
                memcmp( extn_oid.p, OID_KEY_USAGE, extn_oid.len ) == 0 )
        {
            /* Parse key usage */
            if( ( ret = x509_get_key_usage( p, end_ext_octet,
                    &crt->key_usage ) ) != 0 )
                return ( ret );
            crt->ext_types |= EXT_KEY_USAGE;
        }
        else if( ( OID_SIZE( OID_EXTENDED_KEY_USAGE ) == extn_oid.len ) &&
                memcmp( extn_oid.p, OID_EXTENDED_KEY_USAGE, extn_oid.len ) == 0 )
        {
            /* Parse extended key usage */
            if( ( ret = x509_get_ext_key_usage( p, end_ext_octet,
                    &crt->ext_key_usage ) ) != 0 )
                return ( ret );
            crt->ext_types |= EXT_EXTENDED_KEY_USAGE;
        }
        else
        {
            /* No parser found, skip extension */
            *p = end_ext_octet;

#if !defined(POLARSSL_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION)
            if( is_critical )
            {
                /* Data is marked as critical: fail */
                return ( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                        POLARSSL_ERR_ASN1_UNEXPECTED_TAG );
            }
#endif
        }
    }

    if( *p != end )
        return( POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

static int x509_get_sig_alg( const x509_buf *sig_oid, int *sig_alg )
{
    if( sig_oid->len == 9 &&
        memcmp( sig_oid->p, OID_PKCS1, 8 ) == 0 )
    {
        if( sig_oid->p[8] >= 2 && sig_oid->p[8] <= 5 )
        {
            *sig_alg = sig_oid->p[8];
            return( 0 );
        }

        if ( sig_oid->p[8] >= 11 && sig_oid->p[8] <= 14 )
        {
            *sig_alg = sig_oid->p[8];
            return( 0 );
        }

        return( POLARSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG );
    }
    if( sig_oid->len == 5 &&
        memcmp( sig_oid->p, OID_RSA_SHA_OBS, 5 ) == 0 )
    {
        *sig_alg = SIG_RSA_SHA1;
        return( 0 );
    }

    return( POLARSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG );
}

/*
 * Unallocate all certificate data
 */
void x509_free( x509_cert *crt )
{
    x509_cert *cert_cur = crt;
    x509_cert *cert_prv;
    x509_name *name_cur;
    x509_name *name_prv;
    x509_sequence *seq_cur;
    x509_sequence *seq_prv;

    if( crt == NULL )
        return;

    do
    {
        rsa_free( &cert_cur->rsa );

        name_cur = cert_cur->issuer.next;
        while( name_cur != NULL )
        {
            name_prv = name_cur;
            name_cur = name_cur->next;
            memset( name_prv, 0, sizeof( x509_name ) );
            free( name_prv );
        }

        name_cur = cert_cur->subject.next;
        while( name_cur != NULL )
        {
            name_prv = name_cur;
            name_cur = name_cur->next;
            memset( name_prv, 0, sizeof( x509_name ) );
            free( name_prv );
        }

        seq_cur = cert_cur->ext_key_usage.next;
        while( seq_cur != NULL )
        {
            seq_prv = seq_cur;
            seq_cur = seq_cur->next;
            memset( seq_prv, 0, sizeof( x509_sequence ) );
            free( seq_prv );
        }

        if( cert_cur->raw.p != NULL )
        {
            memset( cert_cur->raw.p, 0, cert_cur->raw.len );
            free( cert_cur->raw.p );
        }

        cert_cur = cert_cur->next;
    }
    while( cert_cur != NULL );

    cert_cur = crt;
    do
    {
        cert_prv = cert_cur;
        cert_cur = cert_cur->next;

        memset( cert_prv, 0, sizeof( x509_cert ) );
        if( cert_prv != crt )
            free( cert_prv );
    }
    while( cert_cur != NULL );
}


/*
 * Parse one or more certificates and add them to the chained list
 */
int x509parse_crt( x509_cert *chain, const uint8_t *buf, size_t buflen )
{
    int ret;
    size_t len;
    uint8_t *p, *end;
    x509_cert *crt;

    crt = chain;

    /*
     * Check for valid input
     */
    if( crt == NULL || buf == NULL )
        return( 1 );

    while( crt->version != 0 && crt->next != NULL )
        crt = crt->next;

    /*
     * Add new certificate on the end of the chain if needed.
     */
    if ( crt->version != 0 && crt->next == NULL)
    {
        crt->next = (x509_cert *) malloc( sizeof( x509_cert ) );

        if( crt->next == NULL )
        {
            x509_free( crt );
            return( 1 );
        }

        crt = crt->next;
        memset( crt, 0, sizeof( x509_cert ) );
    }

    p = (uint8_t *) malloc( len = buflen );

    if( p == NULL )
        return( 1 );

    memcpy( p, buf, buflen );

    buflen = 0;

    crt->raw.p = p;
    crt->raw.len = len;
    end = p + len;

    /*
     * Certificate  ::=  SEQUENCE  {
     *      tbsCertificate       TBSCertificate,
     *      signatureAlgorithm   AlgorithmIdentifier,
     *      signatureValue       BIT STRING  }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT );
    }

    if( len != (size_t) ( end - p ) )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    /*
     * TBSCertificate  ::=  SEQUENCE  {
     */
    crt->tbs.p = p;

    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT + ret );
    }

    end = p + len;
    crt->tbs.len = end - crt->tbs.p;

    /*
     * Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     *
     * CertificateSerialNumber  ::=  INTEGER
     *
     * signature            AlgorithmIdentifier
     */
    if( ( ret = x509_get_version( &p, end, &crt->version ) ) != 0 ||
        ( ret = x509_get_serial(  &p, end, &crt->serial  ) ) != 0 ||
        ( ret = x509_get_alg(  &p, end, &crt->sig_oid1   ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    crt->version++;

    if( crt->version > 3 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_UNKNOWN_VERSION );
    }

    if( ( ret = x509_get_sig_alg( &crt->sig_oid1, &crt->sig_alg ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    /*
     * issuer               Name
     */
    crt->issuer_raw.p = p;

    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT + ret );
    }

    if( ( ret = x509_get_name( &p, p + len, &crt->issuer ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    crt->issuer_raw.len = p - crt->issuer_raw.p;

    /*
     * Validity ::= SEQUENCE {
     *      notBefore      Time,
     *      notAfter       Time }
     *
     */
    if( ( ret = x509_get_dates( &p, end, &crt->valid_from,
                                         &crt->valid_to ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    /*
     * subject              Name
     */
    crt->subject_raw.p = p;

    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT + ret );
    }

    if( ( ret = x509_get_name( &p, p + len, &crt->subject ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    crt->subject_raw.len = p - crt->subject_raw.p;

    /*
     * SubjectPublicKeyInfo  ::=  SEQUENCE
     *      algorithm            AlgorithmIdentifier,
     *      subjectPublicKey     BIT STRING  }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT + ret );
    }

    if( ( ret = x509_get_pubkey( &p, p + len, &crt->pk_oid,
                                 &crt->rsa.N, &crt->rsa.E ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    if( ( ret = rsa_check_pubkey( &crt->rsa ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    crt->rsa.len = mpi_size( &crt->rsa.N );

    /*
     *  issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
     *                       -- If present, version shall be v2 or v3
     *  subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
     *                       -- If present, version shall be v2 or v3
     *  extensions      [3]  EXPLICIT Extensions OPTIONAL
     *                       -- If present, version shall be v3
     */
    if( crt->version == 2 || crt->version == 3 )
    {
        ret = x509_get_uid( &p, end, &crt->issuer_id,  1 );
        if( ret != 0 )
        {
            x509_free( crt );
            return( ret );
        }
    }

    if( crt->version == 2 || crt->version == 3 )
    {
        ret = x509_get_uid( &p, end, &crt->subject_id,  2 );
        if( ret != 0 )
        {
            x509_free( crt );
            return( ret );
        }
    }

    if( crt->version == 3 )
    {
        ret = x509_get_crt_ext( &p, end, crt);
        if( ret != 0 )
        {
            x509_free( crt );
            return( ret );
        }
    }

    if( p != end )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    end = crt->raw.p + crt->raw.len;

    /*
     *  signatureAlgorithm   AlgorithmIdentifier,
     *  signatureValue       BIT STRING
     */
    if( ( ret = x509_get_alg( &p, end, &crt->sig_oid2 ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    if( memcmp( crt->sig_oid1.p, crt->sig_oid2.p, crt->sig_oid1.len ) != 0 )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_SIG_MISMATCH );
    }

    if( ( ret = x509_get_sig( &p, end, &crt->sig ) ) != 0 )
    {
        x509_free( crt );
        return( ret );
    }

    if( p != end )
    {
        x509_free( crt );
        return( POLARSSL_ERR_X509_CERT_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    if( buflen > 0 )
    {
        crt->next = (x509_cert *) malloc( sizeof( x509_cert ) );

        if( crt->next == NULL )
        {
            x509_free( crt );
            return( 1 );
        }

        crt = crt->next;
        memset( crt, 0, sizeof( x509_cert ) );

        return( x509parse_crt( crt, buf, buflen ) );
    }

    return( 0 );
}

/*
 * Unallocate all CRL data
 */
void x509_crl_free( x509_crl *crl )
{
    x509_crl *crl_cur = crl;
    x509_crl *crl_prv;
    x509_name *name_cur;
    x509_name *name_prv;
    x509_crl_entry *entry_cur;
    x509_crl_entry *entry_prv;

    if( crl == NULL )
        return;

    do
    {
        name_cur = crl_cur->issuer.next;
        while( name_cur != NULL )
        {
            name_prv = name_cur;
            name_cur = name_cur->next;
            memset( name_prv, 0, sizeof( x509_name ) );
            free( name_prv );
        }

        entry_cur = crl_cur->entry.next;
        while( entry_cur != NULL )
        {
            entry_prv = entry_cur;
            entry_cur = entry_cur->next;
            memset( entry_prv, 0, sizeof( x509_crl_entry ) );
            free( entry_prv );
        }

        if( crl_cur->raw.p != NULL )
        {
            memset( crl_cur->raw.p, 0, crl_cur->raw.len );
            free( crl_cur->raw.p );
        }

        crl_cur = crl_cur->next;
    }
    while( crl_cur != NULL );

    crl_cur = crl;
    do
    {
        crl_prv = crl_cur;
        crl_cur = crl_cur->next;

        memset( crl_prv, 0, sizeof( x509_crl ) );
        if( crl_prv != crl )
            free( crl_prv );
    }
    while( crl_cur != NULL );
}

