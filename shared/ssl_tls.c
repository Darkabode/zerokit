#if defined _MSC_VER && !defined strcasecmp
#define strcasecmp _stricmp
#endif

int ssl_derive_keys( ssl_context *ssl )
{
    int i;
    md5_context md5;
    sha1_context sha1;
    uint8_t tmp[64];
    uint8_t padding[16];
    uint8_t sha1sum[20];
    uint8_t keyblk[256];
    uint8_t *key1;
    uint8_t *key2;

    /*
     * SSLv3:
     *   master =
     *     MD5( premaster + SHA1( 'A'   + premaster + randbytes ) ) +
     *     MD5( premaster + SHA1( 'BB'  + premaster + randbytes ) ) +
     *     MD5( premaster + SHA1( 'CCC' + premaster + randbytes ) )
     *
     * TLSv1:
     *   master = PRF( premaster, "master secret", randbytes )[0..47]
     */
    if( ssl->resume == 0 )
    {
        size_t len = ssl->pmslen;

        for( i = 0; i < 3; i++ )
        {
            memset( padding, 'A' + i, 1 + i );

            sha1_starts( &sha1 );
            sha1_update( &sha1, padding, 1 + i );
            sha1_update( &sha1, ssl->premaster, len );
            sha1_update( &sha1, ssl->randbytes,  64 );
            sha1_finish( &sha1, sha1sum );

            md5_starts( &md5 );
            md5_update( &md5, ssl->premaster, len );
            md5_update( &md5, sha1sum, 20 );
            md5_finish( &md5, ssl->session->master + i * 16 );
        }

        memset( ssl->premaster, 0, sizeof( ssl->premaster ) );
    }

    /*
     * Swap the client and server random values.
     */
    memcpy( tmp, ssl->randbytes, 64 );
    memcpy( ssl->randbytes, tmp + 32, 32 );
    memcpy( ssl->randbytes + 32, tmp, 32 );
    memset( tmp, 0, sizeof( tmp ) );

    /*
     *  SSLv3:
     *    key block =
     *      MD5( master + SHA1( 'A'    + master + randbytes ) ) +
     *      MD5( master + SHA1( 'BB'   + master + randbytes ) ) +
     *      MD5( master + SHA1( 'CCC'  + master + randbytes ) ) +
     *      MD5( master + SHA1( 'DDDD' + master + randbytes ) ) +
     *      ...
     *
     *  TLSv1:
     *    key block = PRF( master, "key expansion", randbytes )
     */
    for( i = 0; i < 16; i++ )
    {
        memset( padding, 'A' + i, 1 + i );

        sha1_starts( &sha1 );
        sha1_update( &sha1, padding, 1 + i );
        sha1_update( &sha1, ssl->session->master, 48 );
        sha1_update( &sha1, ssl->randbytes, 64 );
        sha1_finish( &sha1, sha1sum );

        md5_starts( &md5 );
        md5_update( &md5, ssl->session->master, 48 );
        md5_update( &md5, sha1sum, 20 );
        md5_finish( &md5, keyblk + i * 16 );
    }

    memset( &md5,  0, sizeof( md5  ) );
    memset( &sha1, 0, sizeof( sha1 ) );

    memset( padding, 0, sizeof( padding ) );
    memset( sha1sum, 0, sizeof( sha1sum ) );

    memset( ssl->randbytes, 0, sizeof( ssl->randbytes ) );

    /*
     * Determine the appropriate key, IV and MAC length.
     */
    switch( ssl->session->ciphersuite )
    {
        case SSL_RSA_RC4_128_SHA:
            ssl->keylen = 16; ssl->minlen = 20;
            ssl->ivlen  =  0; ssl->maclen = 20;
            break;
        default:
            return( POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE );
    }

    /*
     * Finally setup the cipher contexts, IVs and MAC secrets.
     */
    key1 = keyblk + ssl->maclen * 2;
    key2 = keyblk + ssl->maclen * 2 + ssl->keylen;

    memcpy( ssl->mac_enc, keyblk,  ssl->maclen );
    memcpy( ssl->mac_dec, keyblk + ssl->maclen, ssl->maclen );

    /*
     * This is not used in TLS v1.1.
     */
    memcpy( ssl->iv_enc, key2 + ssl->keylen,  ssl->ivlen );
    memcpy( ssl->iv_dec, key2 + ssl->keylen + ssl->ivlen,
            ssl->ivlen );

    switch( ssl->session->ciphersuite )
    {
        case SSL_RSA_RC4_128_SHA:
            arc4_setup( (arc4_context *) ssl->ctx_enc, key1, ssl->keylen );
            arc4_setup( (arc4_context *) ssl->ctx_dec, key2, ssl->keylen );
            break;
        default:
            return( POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE );
    }

    memset( keyblk, 0, sizeof( keyblk ) );

    return( 0 );
}

void ssl_calc_verify( ssl_context *ssl, uint8_t hash[36] )
{
    md5_context md5;
    sha1_context sha1;
    uint8_t pad_1[48];
    uint8_t pad_2[48];

    memcpy( &md5 , &ssl->fin_md5 , sizeof(  md5_context ) );
    memcpy( &sha1, &ssl->fin_sha1, sizeof( sha1_context ) );

    memset( pad_1, 0x36, 48 );
    memset( pad_2, 0x5C, 48 );

    md5_update( &md5, ssl->session->master, 48 );
    md5_update( &md5, pad_1, 48 );
    md5_finish( &md5, hash );

    md5_starts( &md5 );
    md5_update( &md5, ssl->session->master, 48 );
    md5_update( &md5, pad_2, 48 );
    md5_update( &md5, hash,  16 );
    md5_finish( &md5, hash );
    
    sha1_update( &sha1, ssl->session->master, 48 );
    sha1_update( &sha1, pad_1, 40 );
    sha1_finish( &sha1, hash + 16 );

    sha1_starts( &sha1 );
    sha1_update( &sha1, ssl->session->master, 48 );
    sha1_update( &sha1, pad_2, 40 );
    sha1_update( &sha1, hash + 16, 20 );
    sha1_finish( &sha1, hash + 16 );

    return;
}

/*
 * SSLv3.0 MAC functions
 */
static void ssl_mac_md5( uint8_t *secret,
                         uint8_t *buf, size_t len,
                         uint8_t *ctr, int type )
{
    uint8_t header[11];
    uint8_t padding[48];
    md5_context md5;

    memcpy( header, ctr, 8 );
    header[ 8] = (uint8_t)  type;
    header[ 9] = (uint8_t)( len >> 8 );
    header[10] = (uint8_t)( len      );

    memset( padding, 0x36, 48 );
    md5_starts( &md5 );
    md5_update( &md5, secret,  16 );
    md5_update( &md5, padding, 48 );
    md5_update( &md5, header,  11 );
    md5_update( &md5, buf,  len );
    md5_finish( &md5, buf + len );

    memset( padding, 0x5C, 48 );
    md5_starts( &md5 );
    md5_update( &md5, secret,  16 );
    md5_update( &md5, padding, 48 );
    md5_update( &md5, buf + len, 16 );
    md5_finish( &md5, buf + len );
}

static void ssl_mac_sha1( uint8_t *secret,
                          uint8_t *buf, size_t len,
                          uint8_t *ctr, int type )
{
    uint8_t header[11];
    uint8_t padding[40];
    sha1_context sha1;

    memcpy( header, ctr, 8 );
    header[ 8] = (uint8_t)  type;
    header[ 9] = (uint8_t)( len >> 8 );
    header[10] = (uint8_t)( len      );

    memset( padding, 0x36, 40 );
    sha1_starts( &sha1 );
    sha1_update( &sha1, secret,  20 );
    sha1_update( &sha1, padding, 40 );
    sha1_update( &sha1, header,  11 );
    sha1_update( &sha1, buf,  len );
    sha1_finish( &sha1, buf + len );

    memset( padding, 0x5C, 40 );
    sha1_starts( &sha1 );
    sha1_update( &sha1, secret,  20 );
    sha1_update( &sha1, padding, 40 );
    sha1_update( &sha1, buf + len, 20 );
    sha1_finish( &sha1, buf + len );
}

/*
 * Encryption/decryption functions
 */ 
static int ssl_encrypt_buf( ssl_context *ssl )
{
    size_t i, padlen;

    /*
     * Add MAC then encrypt
     */
    if( ssl->maclen == 16 )
         ssl_mac_md5( ssl->mac_enc,
                      ssl->out_msg, ssl->out_msglen,
                      ssl->out_ctr, ssl->out_msgtype );

    if( ssl->maclen == 20 )
        ssl_mac_sha1( ssl->mac_enc,
                      ssl->out_msg, ssl->out_msglen,
                      ssl->out_ctr, ssl->out_msgtype );

    ssl->out_msglen += ssl->maclen;

    for( i = 8; i > 0; i-- )
        if( ++ssl->out_ctr[i - 1] != 0 )
            break;

    if( ssl->ivlen == 0 )
    {
        padlen = 0;

        arc4_crypt( (arc4_context *) ssl->ctx_enc,
                    ssl->out_msglen, ssl->out_msg,
                    ssl->out_msg );
    }
    else
    {
        uint8_t *enc_msg;
        size_t enc_msglen;

        padlen = ssl->ivlen - ( ssl->out_msglen + 1 ) % ssl->ivlen;
        if( padlen == ssl->ivlen )
            padlen = 0;

        for( i = 0; i <= padlen; i++ )
            ssl->out_msg[ssl->out_msglen + i] = (uint8_t) padlen;

        ssl->out_msglen += padlen + 1;

        enc_msglen = ssl->out_msglen;
        enc_msg = ssl->out_msg;

        switch( ssl->ivlen )
        {
            case  8:
            case 16:
            default:
                return( POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE );
        }
    }

    return( 0 );
}

static int ssl_decrypt_buf( ssl_context *ssl )
{
    size_t i, padlen;
    uint8_t tmp[20];

    if( ssl->in_msglen < ssl->minlen )
    {
        return( POLARSSL_ERR_SSL_INVALID_MAC );
    }

    if( ssl->ivlen == 0 )
    {
        padlen = 0;
        arc4_crypt( (arc4_context *) ssl->ctx_dec,
                    ssl->in_msglen, ssl->in_msg,
                    ssl->in_msg );
    }
    else
    {
        uint8_t *dec_msg;
        uint8_t *dec_msg_result;
        size_t dec_msglen;

        /*
         * Decrypt and check the padding
         */
        if( ssl->in_msglen % ssl->ivlen != 0 )
        {
            return( POLARSSL_ERR_SSL_INVALID_MAC );
        }

        dec_msglen = ssl->in_msglen;
        dec_msg = ssl->in_msg;
        dec_msg_result = ssl->in_msg;

        return( POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE );
    }

    /*
     * Always compute the MAC (RFC4346, CBCTIME).
     */
    ssl->in_msglen -= ( ssl->maclen + padlen );

    ssl->in_hdr[3] = (uint8_t)( ssl->in_msglen >> 8 );
    ssl->in_hdr[4] = (uint8_t)( ssl->in_msglen      );

    memcpy( tmp, ssl->in_msg + ssl->in_msglen, 20 );

    if( ssl->maclen == 16 )
         ssl_mac_md5( ssl->mac_dec,
                      ssl->in_msg, ssl->in_msglen,
                      ssl->in_ctr, ssl->in_msgtype );
    else
        ssl_mac_sha1( ssl->mac_dec,
                      ssl->in_msg, ssl->in_msglen,
                      ssl->in_ctr, ssl->in_msgtype );

    if( memcmp( tmp, ssl->in_msg + ssl->in_msglen,
                     ssl->maclen ) != 0 )
    {
        return( POLARSSL_ERR_SSL_INVALID_MAC );
    }

    /*
     * Finally check the padding length; bad padding
     * will produce the same error as an invalid MAC.
     */
    if( ssl->ivlen != 0 && padlen == 0 )
        return( POLARSSL_ERR_SSL_INVALID_MAC );

    if( ssl->in_msglen == 0 )
    {
        ssl->nb_zero++;

        /*
         * Three or more empty messages may be a DoS attack
         * (excessive CPU consumption).
         */
        if( ssl->nb_zero > 3 )
        {
            return( POLARSSL_ERR_SSL_INVALID_MAC );
        }
    }
    else
        ssl->nb_zero = 0;
            
    for( i = 8; i > 0; i-- )
        if( ++ssl->in_ctr[i - 1] != 0 )
            break;

    return( 0 );
}

/*
 * Fill the input message buffer
 */
int ssl_fetch_input( ssl_context *ssl, size_t nb_want )
{
    int ret;
    size_t len;

    while( ssl->in_left < nb_want )
    {
        len = nb_want - ssl->in_left;
        ret = ssl->f_recv( ssl->p_recv, ssl->in_hdr + ssl->in_left, len );

        if( ret == 0 )
            return( POLARSSL_ERR_SSL_CONN_EOF );

        if( ret < 0 )
            return( ret );

        ssl->in_left += ret;
    }

    return( 0 );
}

/*
 * Flush any data not yet written
 */
int ssl_flush_output( ssl_context *ssl )
{
    int ret;
    uint8_t *buf;

    while( ssl->out_left > 0 )
    {
        buf = ssl->out_hdr + 5 + ssl->out_msglen - ssl->out_left;
        ret = ssl->f_send( ssl->p_send, buf, ssl->out_left );

        if( ret <= 0 )
            return( ret );

        ssl->out_left -= ret;
    }

    return( 0 );
}

/*
 * Record layer functions
 */
int ssl_write_record( ssl_context *ssl )
{
    int ret;
    size_t len = ssl->out_msglen;

    ssl->out_hdr[0] = (uint8_t) ssl->out_msgtype;
    ssl->out_hdr[1] = (uint8_t) ssl->major_ver;
    ssl->out_hdr[2] = (uint8_t) ssl->minor_ver;
    ssl->out_hdr[3] = (uint8_t)( len >> 8 );
    ssl->out_hdr[4] = (uint8_t)( len      );

    if( ssl->out_msgtype == SSL_MSG_HANDSHAKE )
    {
        ssl->out_msg[1] = (uint8_t)( ( len - 4 ) >> 16 );
        ssl->out_msg[2] = (uint8_t)( ( len - 4 ) >>  8 );
        ssl->out_msg[3] = (uint8_t)( ( len - 4 )       );

         md5_update( &ssl->fin_md5 , ssl->out_msg, len );
        sha1_update( &ssl->fin_sha1, ssl->out_msg, len );
    }

    if( ssl->do_crypt != 0 )
    {
        if( ( ret = ssl_encrypt_buf( ssl ) ) != 0 )
        {
            return( ret );
        }

        len = ssl->out_msglen;
        ssl->out_hdr[3] = (uint8_t)( len >> 8 );
        ssl->out_hdr[4] = (uint8_t)( len      );
    }

    ssl->out_left = 5 + ssl->out_msglen;

    if( ( ret = ssl_flush_output( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int ssl_read_record( ssl_context *ssl )
{
    int ret;

    if( ssl->in_hslen != 0 &&
        ssl->in_hslen < ssl->in_msglen )
    {
        /*
         * Get next Handshake message in the current record
         */
        ssl->in_msglen -= ssl->in_hslen;

        memmove( ssl->in_msg, ssl->in_msg + ssl->in_hslen,
                ssl->in_msglen );

        ssl->in_hslen  = 4;
        ssl->in_hslen += ( ssl->in_msg[2] << 8 ) | ssl->in_msg[3];

        if( ssl->in_msglen < 4 || ssl->in_msg[1] != 0 )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if( ssl->in_msglen < ssl->in_hslen )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

         md5_update( &ssl->fin_md5 , ssl->in_msg, ssl->in_hslen );
        sha1_update( &ssl->fin_sha1, ssl->in_msg, ssl->in_hslen );

        return( 0 );
    }

    ssl->in_hslen = 0;

    /*
     * Read the record header and validate it
     */
    if( ( ret = ssl_fetch_input( ssl, 5 ) ) != 0 )
    {
        return( ret );
    }

    ssl->in_msgtype =  ssl->in_hdr[0];
    ssl->in_msglen = ( ssl->in_hdr[3] << 8 ) | ssl->in_hdr[4];

    if( ssl->in_hdr[1] != ssl->major_ver )
    {
        return( POLARSSL_ERR_SSL_INVALID_RECORD );
    }

    if( ssl->in_hdr[2] > ssl->max_minor_ver )
    {
        return( POLARSSL_ERR_SSL_INVALID_RECORD );
    }

    /*
     * Make sure the message length is acceptable
     */
    if( ssl->do_crypt == 0 )
    {
        if( ssl->in_msglen < 1 ||
            ssl->in_msglen > SSL_MAX_CONTENT_LEN )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
    }
    else
    {
        if( ssl->in_msglen < ssl->minlen )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if (ssl->in_msglen > ssl->minlen + SSL_MAX_CONTENT_LEN)
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
    }

    /*
     * Read and optionally decrypt the message contents
     */
    if( ( ret = ssl_fetch_input( ssl, 5 + ssl->in_msglen ) ) != 0 )
    {
        return( ret );
    }

    if( ssl->do_crypt != 0 )
    {
        if( ( ret = ssl_decrypt_buf( ssl ) ) != 0 )
        {
            return( ret );
        }

        if( ssl->in_msglen > SSL_MAX_CONTENT_LEN )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
    }

    if( ssl->in_msgtype == SSL_MSG_HANDSHAKE )
    {
        ssl->in_hslen  = 4;
        ssl->in_hslen += ( ssl->in_msg[2] << 8 ) | ssl->in_msg[3];

        /*
         * Additional checks to validate the handshake header
         */
        if( ssl->in_msglen < 4 || ssl->in_msg[1] != 0 )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if( ssl->in_msglen < ssl->in_hslen )
        {
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

         md5_update( &ssl->fin_md5 , ssl->in_msg, ssl->in_hslen );
        sha1_update( &ssl->fin_sha1, ssl->in_msg, ssl->in_hslen );
    }

    if( ssl->in_msgtype == SSL_MSG_ALERT )
    {
        /*
         * Ignore non-fatal alerts, except close_notify
         */
        if( ssl->in_msg[0] == SSL_ALERT_LEVEL_FATAL )
        {
            /**
             * Subtract from error code as ssl->in_msg[1] is 7-bit positive
             * error identifier.
             */
            return( POLARSSL_ERR_SSL_FATAL_ALERT_MESSAGE - ssl->in_msg[1] );
        }

        if( ssl->in_msg[0] == SSL_ALERT_LEVEL_WARNING &&
            ssl->in_msg[1] == SSL_ALERT_MSG_CLOSE_NOTIFY )
        {
            return( POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY );
        }
    }

    ssl->in_left = 0;

    return( 0 );
}

/*
 * Handshake functions
 */
int ssl_write_certificate( ssl_context *ssl )
{
    int ret;
    size_t i, n;
    const x509_cert *crt;

    if( ssl->client_auth == 0 )
    {
        ssl->state++;
        return( 0 );
    }

    /*
     * If using SSLv3 and got no cert, send an Alert message
     * (otherwise an empty Certificate message will be sent).
     */
    if( ssl->own_cert  == NULL)
    {
        ssl->out_msglen  = 2;
        ssl->out_msgtype = SSL_MSG_ALERT;
        ssl->out_msg[0]  = SSL_ALERT_LEVEL_WARNING;
        ssl->out_msg[1]  = SSL_ALERT_MSG_NO_CERT;

        goto write_msg;
    }

    /*
     *     0  .  0    handshake type
     *     1  .  3    handshake length
     *     4  .  6    length of all certs
     *     7  .  9    length of cert. 1
     *    10  . n-1   peer certificate
     *     n  . n+2   length of cert. 2
     *    n+3 . ...   upper level cert, etc.
     */
    i = 7;
    crt = ssl->own_cert;

    while( crt != NULL )
    {
        n = crt->raw.len;
        if( i + 3 + n > SSL_MAX_CONTENT_LEN )
        {
            return( POLARSSL_ERR_SSL_CERTIFICATE_TOO_LARGE );
        }

        ssl->out_msg[i    ] = (uint8_t)( n >> 16 );
        ssl->out_msg[i + 1] = (uint8_t)( n >>  8 );
        ssl->out_msg[i + 2] = (uint8_t)( n       );

        i += 3; memcpy( ssl->out_msg + i, crt->raw.p, n );
        i += n; crt = crt->next;
    }

    ssl->out_msg[4]  = (uint8_t)( ( i - 7 ) >> 16 );
    ssl->out_msg[5]  = (uint8_t)( ( i - 7 ) >>  8 );
    ssl->out_msg[6]  = (uint8_t)( ( i - 7 )       );

    ssl->out_msglen  = i;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_CERTIFICATE;

write_msg:

    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int ssl_parse_certificate( ssl_context *ssl )
{
    int ret;
    size_t i, n;

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    ssl->state++;

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    if( ssl->in_msg[0] != SSL_HS_CERTIFICATE || ssl->in_hslen < 10 )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
    }

    /*
     * Same message structure as in ssl_write_certificate()
     */
    n = ( ssl->in_msg[5] << 8 ) | ssl->in_msg[6];

    if( ssl->in_msg[4] != 0 || ssl->in_hslen != 7 + n )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
    }

    if( ( ssl->peer_cert = (x509_cert *) malloc(
                    sizeof( x509_cert ) ) ) == NULL )
    {
        return( 1 );
    }

    memset( ssl->peer_cert, 0, sizeof( x509_cert ) );

    i = 7;

    while( i < ssl->in_hslen )
    {
        if( ssl->in_msg[i] != 0 )
        {
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
        }

        n = ( (unsigned int) ssl->in_msg[i + 1] << 8 )
            | (unsigned int) ssl->in_msg[i + 2];
        i += 3;

        if( n < 128 || i + n > ssl->in_hslen )
        {
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
        }

        ret = x509parse_crt( ssl->peer_cert, ssl->in_msg + i, n );
        if( ret != 0 )
        {
            return( ret );
        }

        i += n;
    }

    return( ret );
}

int ssl_write_change_cipher_spec( ssl_context *ssl )
{
    int ret;

    ssl->out_msgtype = SSL_MSG_CHANGE_CIPHER_SPEC;
    ssl->out_msglen  = 1;
    ssl->out_msg[0]  = 1;

    ssl->do_crypt = 0;
    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int ssl_parse_change_cipher_spec( ssl_context *ssl )
{
    int ret;

    ssl->do_crypt = 0;

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_CHANGE_CIPHER_SPEC )
    {
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    if( ssl->in_msglen != 1 || ssl->in_msg[0] != 1 )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC );
    }

    ssl->state++;

    return( 0 );
}

static void ssl_calc_finished(
                ssl_context *ssl, uint8_t *buf, int from,
                md5_context *md5, sha1_context *sha1 )
{
    int len = 12;
    char *sender;
    uint8_t padbuf[48];
    uint8_t md5sum[16];
    uint8_t sha1sum[20];

    /*
     * SSLv3:
     *   hash =
     *      MD5( master + pad2 +
     *          MD5( handshake + sender + master + pad1 ) )
     *   + SHA1( master + pad2 +
     *         SHA1( handshake + sender + master + pad1 ) )
     *
     * TLSv1:
     *   hash = PRF( master, finished_label,
     *               MD5( handshake ) + SHA1( handshake ) )[0..11]
     */

    sender = ( from == 0/*SSL_IS_CLIENT*/ ) ? (char *) "CLNT" : (char *) "SRVR";

    memset( padbuf, 0x36, 48 );

    md5_update( md5, (uint8_t *) sender, 4 );
    md5_update( md5, ssl->session->master, 48 );
    md5_update( md5, padbuf, 48 );
    md5_finish( md5, md5sum );

    sha1_update( sha1, (uint8_t *) sender, 4 );
    sha1_update( sha1, ssl->session->master, 48 );
    sha1_update( sha1, padbuf, 40 );
    sha1_finish( sha1, sha1sum );

    memset( padbuf, 0x5C, 48 );

    md5_starts( md5 );
    md5_update( md5, ssl->session->master, 48 );
    md5_update( md5, padbuf, 48 );
    md5_update( md5, md5sum, 16 );
    md5_finish( md5, buf );

    sha1_starts( sha1 );
    sha1_update( sha1, ssl->session->master, 48 );
    sha1_update( sha1, padbuf , 40 );
    sha1_update( sha1, sha1sum, 20 );
    sha1_finish( sha1, buf + 16 );

    len += 24;

    memset(  md5, 0, sizeof(  md5_context ) );
    memset( sha1, 0, sizeof( sha1_context ) );

    memset(  padbuf, 0, sizeof(  padbuf ) );
    memset(  md5sum, 0, sizeof(  md5sum ) );
    memset( sha1sum, 0, sizeof( sha1sum ) );
}

int ssl_write_finished( ssl_context *ssl )
{
    int ret, hash_len;
     md5_context  md5;
    sha1_context sha1;

    memcpy( &md5 , &ssl->fin_md5 , sizeof(  md5_context ) );
    memcpy( &sha1, &ssl->fin_sha1, sizeof( sha1_context ) );

    ssl_calc_finished( ssl, ssl->out_msg + 4, 0, &md5, &sha1 );

    hash_len = 36;

    ssl->out_msglen  = 4 + hash_len;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_FINISHED;

    /*
     * In case of session resuming, invert the client and server
     * ChangeCipherSpec messages order.
     */
    if (ssl->resume != 0) {
        ssl->state = SSL_HANDSHAKE_OVER;
    }
    else
        ssl->state++;

    ssl->do_crypt = 1;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int ssl_parse_finished( ssl_context *ssl )
{
    int ret;
    unsigned int hash_len;
    uint8_t buf[36];
    md5_context  md5;
    sha1_context sha1;

    memcpy( &md5 , &ssl->fin_md5 , sizeof(  md5_context ) );
    memcpy( &sha1, &ssl->fin_sha1, sizeof( sha1_context ) );

    ssl->do_crypt = 1;

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    hash_len = 36;

    if( ssl->in_msg[0] != SSL_HS_FINISHED ||
        ssl->in_hslen  != 4 + hash_len )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_FINISHED );
    }

    ssl_calc_finished( ssl, buf, 1, &md5, &sha1 );

    if( memcmp( ssl->in_msg + 4, buf, hash_len ) != 0 )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_FINISHED );
    }

    if( ssl->resume != 0 )
    {
		ssl->state = SSL_CLIENT_CHANGE_CIPHER_SPEC;
    }
    else
        ssl->state++;

    return( 0 );
}

/*
 * Initialize an SSL context
 */
int ssl_init( ssl_context *ssl )
{
    int len = SSL_BUFFER_LEN;

    memset( ssl, 0, sizeof( ssl_context ) );

    ssl->in_ctr = (uint8_t *) malloc( len );
    ssl->in_hdr = ssl->in_ctr +  8;
    ssl->in_msg = ssl->in_ctr + 13;

    if( ssl->in_ctr == NULL )
    {
        return( 1 );
    }

    ssl->out_ctr = (uint8_t *) malloc( len );
    ssl->out_hdr = ssl->out_ctr +  8;
    ssl->out_msg = ssl->out_ctr + 13;

    if( ssl->out_ctr == NULL )
    {
        free( ssl-> in_ctr );
        return( 1 );
    }

    memset( ssl-> in_ctr, 0, SSL_BUFFER_LEN );
    memset( ssl->out_ctr, 0, SSL_BUFFER_LEN );

    ssl->hostname = NULL;
    ssl->hostname_len = 0;

     md5_starts( &ssl->fin_md5  );
    sha1_starts( &ssl->fin_sha1 );

    return( 0 );
}

void ssl_set_verify( ssl_context *ssl,
                     int (*f_vrfy)(void *, x509_cert *, int, int),
                     void *p_vrfy )
{
    ssl->f_vrfy      = f_vrfy;
    ssl->p_vrfy      = p_vrfy;
}

void ssl_set_rng( ssl_context *ssl,
                  int (*f_rng)(void *),
                  void *p_rng )
{
    ssl->f_rng      = f_rng;
    ssl->p_rng      = p_rng;
}

void ssl_set_bio( ssl_context *ssl,
            int (*f_recv)(void *, uint8_t *, size_t), void *p_recv,
            int (*f_send)(void *, const uint8_t *, size_t), void *p_send )
{
    ssl->f_recv     = f_recv;
    ssl->f_send     = f_send;
    ssl->p_recv     = p_recv;
    ssl->p_send     = p_send;
}

void ssl_set_session( ssl_context *ssl, int resume, int timeout,
                      ssl_session *session )
{
    ssl->resume     = resume;
    ssl->timeout    = timeout;
    ssl->session    = session;
}

void ssl_set_ciphersuites( ssl_context *ssl, int *ciphersuites )
{
    ssl->ciphersuites    = ciphersuites;
}

void ssl_set_ca_chain( ssl_context *ssl, x509_cert *ca_chain,
                       x509_crl *ca_crl, const char *peer_cn )
{
    ssl->ca_chain   = ca_chain;
    ssl->ca_crl     = ca_crl;
    ssl->peer_cn    = peer_cn;
}

void ssl_set_own_cert( ssl_context *ssl, x509_cert *own_cert,
                       rsa_context *rsa_key )
{
    ssl->own_cert   = own_cert;
    ssl->rsa_key    = rsa_key;
}

int ssl_set_hostname( ssl_context *ssl, const char *hostname )
{
    if( hostname == NULL )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    ssl->hostname_len = strlen( hostname );
    ssl->hostname = (uint8_t *) malloc( ssl->hostname_len + 1 );

    memcpy( ssl->hostname, (uint8_t *) hostname,
            ssl->hostname_len );
    
    ssl->hostname[ssl->hostname_len] = '\0';

    return( 0 );
}

/*
 * SSL get accessors
 */
size_t ssl_get_bytes_avail( const ssl_context *ssl )
{
    return( ssl->in_offt == NULL ? 0 : ssl->in_msglen );
}

int ssl_get_verify_result( const ssl_context *ssl )
{
    return( ssl->verify_result );
}

const char *ssl_get_ciphersuite_name( const int ciphersuite_id )
{
    switch( ciphersuite_id )
    {
        case SSL_RSA_RC4_128_SHA:
            return( "SSL-RSA-RC4-128-SHA" );
    default:
        break;
    }

    return( "unknown" );
}

int ssl_get_ciphersuite_id( const char *ciphersuite_name )
{
    if (0 == strcasecmp(ciphersuite_name, "SSL-RSA-RC4-128-SHA"))
        return( SSL_RSA_RC4_128_SHA );
    return( 0 );
}

const char *ssl_get_ciphersuite( const ssl_context *ssl )
{
    return ssl_get_ciphersuite_name( ssl->session->ciphersuite );
}

const char *ssl_get_version( const ssl_context *ssl )
{
	return( "SSLv3.0" );
}

int ssl_default_ciphersuites[] =
{
    SSL_RSA_RC4_128_SHA,
    0
};

/*
 * Receive application data decrypted from the SSL layer
 */
int ssl_read( ssl_context *ssl, uint8_t *buf, size_t len )
{
    int ret;
    size_t n;

    if( ssl->state != SSL_HANDSHAKE_OVER )
    {
        if ((ret = ssl_handshake_client(ssl)) != 0)
        {
            return( ret );
        }
    }

    if( ssl->in_offt == NULL )
    {
        if( ( ret = ssl_read_record( ssl ) ) != 0 )
        {
            if( ret == POLARSSL_ERR_SSL_CONN_EOF )
                return( 0 );

            return( ret );
        }

        if( ssl->in_msglen  == 0 &&
            ssl->in_msgtype == SSL_MSG_APPLICATION_DATA )
        {
            /*
             * OpenSSL sends empty messages to randomize the IV
             */
            if( ( ret = ssl_read_record( ssl ) ) != 0 )
            {
                if( ret == POLARSSL_ERR_SSL_CONN_EOF )
                    return( 0 );

                return( ret );
            }
        }

        if( ssl->in_msgtype != SSL_MSG_APPLICATION_DATA )
        {
            return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
        }

        ssl->in_offt = ssl->in_msg;
    }

    n = ( len < ssl->in_msglen )
        ? len : ssl->in_msglen;

    memcpy( buf, ssl->in_offt, n );
    ssl->in_msglen -= n;

    if( ssl->in_msglen == 0 )
        /* all bytes consumed  */
        ssl->in_offt = NULL;
    else
        /* more data available */
        ssl->in_offt += n;

    return( (int) n );
}

/*
 * Send application data to be encrypted by the SSL layer
 */
int ssl_write( ssl_context *ssl, const uint8_t *buf, size_t len )
{
    int ret;
    size_t n;

    if( ssl->state != SSL_HANDSHAKE_OVER )
    {
        if( ( ret = ssl_handshake_client( ssl ) ) != 0 )
        {
            return( ret );
        }
    }

    n = ( len < SSL_MAX_CONTENT_LEN )
        ? len : SSL_MAX_CONTENT_LEN;

    if( ssl->out_left != 0 )
    {
        if( ( ret = ssl_flush_output( ssl ) ) != 0 )
        {
            return( ret );
        }
    }
    else
    {
        ssl->out_msglen  = n;
        ssl->out_msgtype = SSL_MSG_APPLICATION_DATA;
        memcpy( ssl->out_msg, buf, n );

        if( ( ret = ssl_write_record( ssl ) ) != 0 )
        {
            return( ret );
        }
    }

    return( (int) n );
}

/*
 * Notify the peer that the connection is being closed
 */
int ssl_close_notify( ssl_context *ssl )
{
    int ret;

    if( ( ret = ssl_flush_output( ssl ) ) != 0 )
    {
        return( ret );
    }

    if( ssl->state == SSL_HANDSHAKE_OVER )
    {
        ssl->out_msgtype = SSL_MSG_ALERT;
        ssl->out_msglen  = 2;
        ssl->out_msg[0]  = SSL_ALERT_LEVEL_WARNING;
        ssl->out_msg[1]  = SSL_ALERT_MSG_CLOSE_NOTIFY;

        if( ( ret = ssl_write_record( ssl ) ) != 0 )
        {
            return( ret );
        }
    }

    return( ret );
}

/*
 * Free an SSL context
 */
void ssl_free( ssl_context *ssl )
{
    if( ssl->peer_cert != NULL )
    {
        x509_free( ssl->peer_cert );
        memset( ssl->peer_cert, 0, sizeof( x509_cert ) );
          free( ssl->peer_cert );
    }

    if( ssl->out_ctr != NULL )
    {
        memset( ssl->out_ctr, 0, SSL_BUFFER_LEN );
          free( ssl->out_ctr );
    }

    if( ssl->in_ctr != NULL )
    {
        memset( ssl->in_ctr, 0, SSL_BUFFER_LEN );
          free( ssl->in_ctr );
    }

    if ( ssl->hostname != NULL)
    {
        memset( ssl->hostname, 0, ssl->hostname_len );
        free( ssl->hostname );
        ssl->hostname_len = 0;
    }

    /* Actually free after last debug message */
    memset( ssl, 0, sizeof( ssl_context ) );
}

