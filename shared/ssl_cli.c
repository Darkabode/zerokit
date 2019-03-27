static int ssl_write_client_hello( ssl_context *ssl )
{
    int ret;
    size_t i, n;
    uint8_t *buf;
    uint8_t *p;
    time_t t;

    ssl->major_ver = SSL_MAJOR_VERSION_3;
    ssl->minor_ver = SSL_MINOR_VERSION_0;

    ssl->max_major_ver = SSL_MAJOR_VERSION_3;
    ssl->max_minor_ver = SSL_MINOR_VERSION_0;

    /*
     *     0  .   0   handshake type
     *     1  .   3   handshake length
     *     4  .   5   highest version supported
     *     6  .   9   current UNIX time
     *    10  .  37   random bytes
     */
    buf = ssl->out_msg;
    p = buf + 4;

    *p++ = (uint8_t) ssl->max_major_ver;
    *p++ = (uint8_t) ssl->max_minor_ver;

    t = time( NULL );
    *p++ = (uint8_t)( t >> 24 );
    *p++ = (uint8_t)( t >> 16 );
    *p++ = (uint8_t)( t >>  8 );
    *p++ = (uint8_t)( t       );

    for( i = 28; i > 0; i-- )
        *p++ = (uint8_t) ssl->f_rng( ssl->p_rng );

    memcpy( ssl->randbytes, buf + 6, 32 );

    /*
     *    38  .  38   session id length
     *    39  . 39+n  session id
     *   40+n . 41+n  ciphersuitelist length
     *   42+n . ..    ciphersuitelist
     *   ..   . ..    compression alg. (0)
     *   ..   . ..    extensions (unused)
     */
    n = ssl->session->length;

    if( n < 16 || n > 32 || ssl->resume == 0 ||
        ( ssl->timeout != 0 && t - ssl->session->start > ssl->timeout ) )
        n = 0;

    *p++ = (uint8_t) n;

    for( i = 0; i < n; i++ )
        *p++ = ssl->session->id[i];

    for( n = 0; ssl->ciphersuites[n] != 0; n++ );
    *p++ = (uint8_t)( n >> 7 );
    *p++ = (uint8_t)( n << 1 );

    for( i = 0; i < n; i++ )
    {
        *p++ = (uint8_t)( ssl->ciphersuites[i] >> 8 );
        *p++ = (uint8_t)( ssl->ciphersuites[i]      );
    }

    *p++ = 1;
    *p++ = SSL_COMPRESS_NULL;

    if ( ssl->hostname != NULL )
    {
        *p++ = (uint8_t)( ( (ssl->hostname_len + 9) >> 8 ) & 0xFF );
        *p++ = (uint8_t)( ( (ssl->hostname_len + 9)      ) & 0xFF );

        *p++ = (uint8_t)( ( TLS_EXT_SERVERNAME >> 8 ) & 0xFF );
        *p++ = (uint8_t)( ( TLS_EXT_SERVERNAME      ) & 0xFF );

        *p++ = (uint8_t)( ( (ssl->hostname_len + 5) >> 8 ) & 0xFF );
        *p++ = (uint8_t)( ( (ssl->hostname_len + 5)      ) & 0xFF );

        *p++ = (uint8_t)( ( (ssl->hostname_len + 3) >> 8 ) & 0xFF );
        *p++ = (uint8_t)( ( (ssl->hostname_len + 3)      ) & 0xFF );

        *p++ = (uint8_t)( ( TLS_EXT_SERVERNAME_HOSTNAME ) & 0xFF );
        *p++ = (uint8_t)( ( ssl->hostname_len >> 8 ) & 0xFF );
        *p++ = (uint8_t)( ( ssl->hostname_len      ) & 0xFF );

        memcpy( p, ssl->hostname, ssl->hostname_len );

        p += ssl->hostname_len;
    }

    ssl->out_msglen  = p - buf;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_CLIENT_HELLO;

    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

static int ssl_parse_server_hello( ssl_context *ssl )
{
    time_t t;
    int ret, i;
    size_t n;
    int ext_len;
    uint8_t *buf;

    /*
     *     0  .   0   handshake type
     *     1  .   3   handshake length
     *     4  .   5   protocol version
     *     6  .   9   UNIX time()
     *    10  .  37   random bytes
     */
    buf = ssl->in_msg;

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    if( ssl->in_hslen < 42 ||
        buf[0] != SSL_HS_SERVER_HELLO ||
        buf[4] != SSL_MAJOR_VERSION_3 )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO );
    }

    if( buf[5] > ssl->max_minor_ver )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO );
    }

    ssl->minor_ver = buf[5];

    t = ( (time_t) buf[6] << 24 )
      | ( (time_t) buf[7] << 16 )
      | ( (time_t) buf[8] <<  8 )
      | ( (time_t) buf[9]       );

    memcpy( ssl->randbytes + 32, buf + 6, 32 );

    n = buf[38];

    /*
     *    38  .  38   session id length
     *    39  . 38+n  session id
     *   39+n . 40+n  chosen ciphersuite
     *   41+n . 41+n  chosen compression alg.
     *   42+n . 43+n  extensions length
     *   44+n . 44+n+m extensions
     */
    if( n > 32 || ssl->in_hslen > 42 + n )
    {
        ext_len = ( ( buf[42 + n] <<  8 )
                  | ( buf[43 + n]       ) ) + 2;
    }
    else
    {
        ext_len = 0;
    }

    if( n > 32 || ssl->in_hslen != 42 + n + ext_len )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO );
    }

    i = ( buf[39 + n] << 8 ) | buf[40 + n];

    /*
     * Check if the session can be resumed
     */
    if( ssl->resume == 0 || n == 0 ||
        ssl->session->ciphersuite != i ||
        ssl->session->length != n ||
        memcmp( ssl->session->id, buf + 39, n ) != 0 )
    {
        ssl->state++;
        ssl->resume = 0;
        ssl->session->start = time( NULL );
        ssl->session->ciphersuite = i;
        ssl->session->length = n;
        memcpy( ssl->session->id, buf + 39, n );
    }
    else
    {
        ssl->state = SSL_SERVER_CHANGE_CIPHER_SPEC;

        if( ( ret = ssl_derive_keys( ssl ) ) != 0 )
        {
            return( ret );
        }
    }

    i = 0;
    while( 1 )
    {
        if( ssl->ciphersuites[i] == 0 )
        {
            return( POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO );
        }

        if( ssl->ciphersuites[i++] == ssl->session->ciphersuite )
            break;
    }

    if( buf[41 + n] != SSL_COMPRESS_NULL )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO );
    }

    /* TODO: Process extensions */

    return( 0 );
}

static int ssl_parse_server_key_exchange( ssl_context *ssl )
{
    ssl->state++;
    return( 0 );
}

static int ssl_parse_certificate_request( ssl_context *ssl )
{
    int ret;

    /*
     *     0  .   0   handshake type
     *     1  .   3   handshake length
     *     4  .   5   SSL version
     *     6  .   6   cert type count
     *     7  .. n-1  cert types
     *     n  .. n+1  length of all DNs
     *    n+2 .. n+3  length of DN 1
     *    n+4 .. ...  Distinguished Name #1
     *    ... .. ...  length of DN 2, etc.
     */
    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    ssl->client_auth = 0;
    ssl->state++;

    if( ssl->in_msg[0] == SSL_HS_CERTIFICATE_REQUEST )
        ssl->client_auth++;

    return( 0 );
}

static int ssl_parse_server_hello_done( ssl_context *ssl )
{
    int ret;

    if( ssl->client_auth != 0 )
    {
        if( ( ret = ssl_read_record( ssl ) ) != 0 )
        {
            return( ret );
        }

        if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
        {
            return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
        }
    }

    if( ssl->in_hslen  != 4 ||
        ssl->in_msg[0] != SSL_HS_SERVER_HELLO_DONE )
    {
        return( POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO_DONE );
    }

    ssl->state++;

    return( 0 );
}

static int ssl_write_client_key_exchange( ssl_context *ssl )
{
    int ret;
    size_t i, n;

    /*
     * RSA key exchange -- send rsa_public(pkcs1 v1.5(premaster))
     */
    ssl->premaster[0] = (uint8_t) ssl->max_major_ver;
    ssl->premaster[1] = (uint8_t) ssl->max_minor_ver;
    ssl->pmslen = 48;

    for( i = 2; i < ssl->pmslen; i++ )
        ssl->premaster[i] = (uint8_t) ssl->f_rng( ssl->p_rng );

    i = 4;
    n = ssl->peer_cert->rsa.len;

    ret = rsa_pkcs1_encrypt( &ssl->peer_cert->rsa,
                              ssl->f_rng, ssl->p_rng,
                              RSA_PUBLIC,
                              ssl->pmslen, ssl->premaster,
                              ssl->out_msg + i );
    if( ret != 0 )
    {
        return( ret );
    }

    if( ( ret = ssl_derive_keys( ssl ) ) != 0 )
    {
        return( ret );
    }

    ssl->out_msglen  = i + n;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_CLIENT_KEY_EXCHANGE;

    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

static int ssl_write_certificate_verify( ssl_context *ssl )
{
    int ret = 0;
    size_t n = 0;
    uint8_t hash[36];

    if( ssl->client_auth == 0 || ssl->own_cert == NULL )
    {
        ssl->state++;
        return( 0 );
    }

    if( ssl->rsa_key == NULL )
    {
        return( POLARSSL_ERR_SSL_PRIVATE_KEY_REQUIRED );
    }

    /*
     * Make an RSA signature of the handshake digests
     */
    ssl_calc_verify( ssl, hash );

    if ( ssl->rsa_key )
        n = ssl->rsa_key->len;

    ssl->out_msg[4] = (uint8_t)( n >> 8 );
    ssl->out_msg[5] = (uint8_t)( n      );

    if( ssl->rsa_key )
    {
        ret = rsa_pkcs1_sign( ssl->rsa_key, ssl->f_rng, ssl->p_rng,
                                    RSA_PRIVATE, SIG_RSA_RAW,
                                    36, hash, ssl->out_msg + 6 );
    }

    if (ret != 0)
    {
        return( ret );
    }

    ssl->out_msglen  = 6 + n;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_CERTIFICATE_VERIFY;

    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

/*
 * SSL handshake -- client side
 */
int ssl_handshake_client( ssl_context *ssl )
{
    int ret = 0;

    while( ssl->state != SSL_HANDSHAKE_OVER )
    {
        if( ( ret = ssl_flush_output( ssl ) ) != 0 )
            break;

        switch( ssl->state )
        {
            case SSL_HELLO_REQUEST:
                ssl->state = SSL_CLIENT_HELLO;
                break;

            /*
             *  ==>   ClientHello
             */
            case SSL_CLIENT_HELLO:
                ret = ssl_write_client_hello( ssl );
                break;

            /*
             *  <==   ServerHello
             *        Certificate
             *      ( ServerKeyExchange  )
             *      ( CertificateRequest )
             *        ServerHelloDone
             */
            case SSL_SERVER_HELLO:
                ret = ssl_parse_server_hello( ssl );
                break;

            case SSL_SERVER_CERTIFICATE:
                ret = ssl_parse_certificate( ssl );
                break;

            case SSL_SERVER_KEY_EXCHANGE:
                ret = ssl_parse_server_key_exchange( ssl );
                break;

            case SSL_CERTIFICATE_REQUEST:
                ret = ssl_parse_certificate_request( ssl );
                break;

            case SSL_SERVER_HELLO_DONE:
                ret = ssl_parse_server_hello_done( ssl );
                break;

            /*
             *  ==> ( Certificate/Alert  )
             *        ClientKeyExchange
             *      ( CertificateVerify  )
             *        ChangeCipherSpec
             *        Finished
             */
            case SSL_CLIENT_CERTIFICATE:
                ret = ssl_write_certificate( ssl );
                break;

            case SSL_CLIENT_KEY_EXCHANGE:
                ret = ssl_write_client_key_exchange( ssl );
                break;

            case SSL_CERTIFICATE_VERIFY:
                ret = ssl_write_certificate_verify( ssl );
                break;

            case SSL_CLIENT_CHANGE_CIPHER_SPEC:
                ret = ssl_write_change_cipher_spec( ssl );
                break;

            case SSL_CLIENT_FINISHED:
                ret = ssl_write_finished( ssl );
                break;

            /*
             *  <==   ChangeCipherSpec
             *        Finished
             */
            case SSL_SERVER_CHANGE_CIPHER_SPEC:
                ret = ssl_parse_change_cipher_spec( ssl );
                break;

            case SSL_SERVER_FINISHED:
                ret = ssl_parse_finished( ssl );
                break;

            case SSL_FLUSH_BUFFERS:
                ssl->state = SSL_HANDSHAKE_OVER;
                break;

            default:
                return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
        }

        if( ret != 0 )
            break;
    }

    return( ret );
}