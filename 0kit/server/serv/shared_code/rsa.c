void rsa_init(rsa_context_t* ctx, int padding, int hash_id)
{
	USE_GLOBAL_BLOCK

	MEMSET(ctx, 0, sizeof(rsa_context_t));

	ctx->padding = padding;
	ctx->hash_id = hash_id;
}

void rsa_free(rsa_context_t *ctx)
{
	USE_GLOBAL_BLOCK

	MPI_FREE(&ctx->RQ);
	MPI_FREE(&ctx->RP);
	MPI_FREE(&ctx->RN);
	MPI_FREE(&ctx->QP);
	MPI_FREE(&ctx->DQ);
	MPI_FREE(&ctx->DP);
	MPI_FREE(&ctx->Q);
	MPI_FREE(&ctx->P);
	MPI_FREE(&ctx->D);
	MPI_FREE(&ctx->E);
	MPI_FREE(&ctx->N);
}

#ifdef USE_PRIME_NUMBERS
int rsa_gen_key(rsa_context_t *ctx, int (*f_rng)(void *, unsigned char *, size_t), void* p_rng, unsigned int nbits, int exponent)
{
    int ret;
    mpi_t P1, Q1, H, G;

    if (f_rng == NULL || nbits < 128 || exponent < 3)
        return ERR_RSA_BAD_INPUT_DATA;

    MPI_INIT(&P1);
	MPI_INIT(&Q1);
	MPI_INIT(&H);
	MPI_INIT(&G);

    /*
     * find primes P and Q with Q < P so that:
     * GCD( E, (P-1)*(Q-1) ) == 1
     */
    MPI_CHK(mpi_lset(&ctx->E, exponent));

    do {
        MPI_CHK(mpi_gen_prime(&ctx->P, (nbits + 1) >> 1, 0, f_rng, p_rng));

        MPI_CHK(mpi_gen_prime(&ctx->Q, (nbits + 1) >> 1, 0, f_rng, p_rng));

        if (mpi_cmp_mpi(&ctx->P, &ctx->Q) < 0)
            mpi_swap(&ctx->P, &ctx->Q);

        if (mpi_cmp_mpi(&ctx->P, &ctx->Q) == 0)
            continue;

        MPI_CHK(mpi_mul_mpi(&ctx->N, &ctx->P, &ctx->Q));
        if (mpi_msb(&ctx->N) != nbits)
            continue;

        MPI_CHK(mpi_sub_int(&P1, &ctx->P, 1));
        MPI_CHK(mpi_sub_int(&Q1, &ctx->Q, 1));
        MPI_CHK(mpi_mul_mpi(&H, &P1, &Q1));
        MPI_CHK(mpi_gcd(&G, &ctx->E, &H));
    } while (mpi_cmp_int(&G, 1) != 0);

    /*
     * D  = E^-1 mod ((P-1)*(Q-1))
     * DP = D mod (P - 1)
     * DQ = D mod (Q - 1)
     * QP = Q^-1 mod P
     */
    MPI_CHK(mpi_inv_mod(&ctx->D , &ctx->E, &H));
    MPI_CHK(mpi_mod_mpi(&ctx->DP, &ctx->D, &P1));
    MPI_CHK(mpi_mod_mpi(&ctx->DQ, &ctx->D, &Q1));
    MPI_CHK(mpi_inv_mod(&ctx->QP, &ctx->Q, &ctx->P));

    ctx->len = (mpi_msb(&ctx->N) + 7) >> 3;

cleanup:

    MPI_FREE(&P1);
	MPI_FREE(&Q1);
	MPI_FREE(&H);
	MPI_FREE(&G);

    if (ret != 0) {
        rsa_free(ctx);
        return POLARSSL_ERR_RSA_KEY_GEN_FAILED + ret;
    }

    return ERR_OK;
}
#endif // USE_PRIME_NUMBERS

int rsa_check_pubkey( const rsa_context_t *ctx )
{
    if( !ctx->N.p || !ctx->E.p )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( ( ctx->N.p[0] & 1 ) == 0 || 
        ( ctx->E.p[0] & 1 ) == 0 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( mpi_msb(&ctx->N) < 128 ||
        mpi_msb(&ctx->N) > POLARSSL_MPI_MAX_BITS)
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( mpi_msb(&ctx->E) < 2 ||
        mpi_msb(&ctx->E) > 64)
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    return( 0 );
}

int rsa_check_privkey( const rsa_context_t *ctx )
{
    int ret;
    mpi_t PQ, DE, P1, Q1, H, I, G, G2, L1, L2;
	USE_GLOBAL_BLOCK

    if( ( ret = rsa_check_pubkey( ctx ) ) != 0 )
        return( ret );

    if( !ctx->P.p || !ctx->Q.p || !ctx->D.p )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    MPI_INIT(&PQ);
	MPI_INIT(&DE);
	MPI_INIT(&P1);
	MPI_INIT(&Q1);
    MPI_INIT(&H);
	MPI_INIT(&I);
	MPI_INIT(&G);
	MPI_INIT(&G2);
    MPI_INIT(&L1);
	MPI_INIT(&L2);

    MPI_CHK( mpi_mul_mpi( &PQ, &ctx->P, &ctx->Q ) );
    MPI_CHK( mpi_mul_mpi( &DE, &ctx->D, &ctx->E ) );
    MPI_CHK( mpi_sub_int( &P1, &ctx->P, 1 ) );
    MPI_CHK( mpi_sub_int( &Q1, &ctx->Q, 1 ) );
    MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );
    MPI_CHK( mpi_gcd( &G, &ctx->E, &H  ) );

    MPI_CHK( mpi_gcd( &G2, &P1, &Q1 ) );
    MPI_CHK( mpi_div_mpi( &L1, &L2, &H, &G2 ) );  
    MPI_CHK( mpi_mod_mpi( &I, &DE, &L1  ) );

    /*
     * Check for a valid PKCS1v2 private key
     */
    if( mpi_cmp_mpi( &PQ, &ctx->N ) != 0 ||
        mpi_cmp_int( &L2, 0 ) != 0 ||
        mpi_cmp_int( &I, 1 ) != 0 ||
        mpi_cmp_int( &G, 1 ) != 0 )
    {
        ret = POLARSSL_ERR_RSA_KEY_CHECK_FAILED;
    }

    
cleanup:

    MPI_FREE(&PQ);
	MPI_FREE(&DE);
	MPI_FREE(&P1);
	MPI_FREE(&Q1);
    MPI_FREE(&H);
	MPI_FREE(&I);
	MPI_FREE(&G);
	MPI_FREE(&G2);
    MPI_FREE(&L1);
	MPI_FREE(&L2);

    if( ret == POLARSSL_ERR_RSA_KEY_CHECK_FAILED )
        return( ret );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED + ret );

    return( 0 );
}

int rsa_public(rsa_context_t* ctx, const uint8_t* input, uint8_t* output)
{
	int ret;
	uint32_t olen;
	mpi_t T;
	USE_GLOBAL_BLOCK

	MPI_INIT(&T);

	MPI_CHK(MPI_READ_BINARY(&T, input, ctx->len));

	if (MPI_CMP_MPI(&T, &ctx->N) >= 0) {
		MPI_FREE(&T);
		return ERR_RSA_BAD_INPUT_DATA;
	}

	olen = ctx->len;
	MPI_CHK(MPI_EXP_MOD(&T, &T, &ctx->E, &ctx->N, &ctx->RN));
	MPI_CHK(MPI_WRITE_BINARY(&T, output, olen));

cleanup:
	MPI_FREE(&T);

	if (ret != 0)
		return POLARSSL_ERR_RSA_PUBLIC_FAILED + ret;

	return 0;
}

int rsa_private(rsa_context_t* ctx, const uint8_t* input, uint8_t* output)
{
	int ret;
	uint32_t olen;
	mpi_t T, T1, T2;
	USE_GLOBAL_BLOCK

	MPI_INIT(&T);
	MPI_INIT(&T1);
	MPI_INIT(&T2);

	MPI_CHK(MPI_READ_BINARY(&T, input, ctx->len));

	if (MPI_CMP_MPI(&T, &ctx->N) >= 0) {
		MPI_FREE(&T);
		return ERR_RSA_BAD_INPUT_DATA;
	}

#if defined(RSA_NO_CRT)
	MPI_CHK(MPI_EXP_MOD(&T, &T, &ctx->D, &ctx->N, &ctx->RN));
#else
	/*
	 * faster decryption using the CRT
	 *
	 * T1 = input ^ dP mod P
	 * T2 = input ^ dQ mod Q
	 */
	MPI_CHK(MPI_EXP_MOD(&T1, &T, &ctx->DP, &ctx->P, &ctx->RP));
	MPI_CHK(MPI_EXP_MOD(&T2, &T, &ctx->DQ, &ctx->Q, &ctx->RQ));

	/*
	 * T = (T1 - T2) * (Q^-1 mod P) mod P
	 */
	MPI_CHK(MPI_SUB_MPI(&T, &T1, &T2));
	MPI_CHK(MPI_MUL_MPI(&T1, &T, &ctx->QP));
	MPI_CHK(MPI_MOD_MPI(&T, &T1, &ctx->P));
	/*
	 * output = T2 + T * Q
	 */
	MPI_CHK(MPI_MUL_MPI(&T1, &T, &ctx->Q));
	MPI_CHK(MPI_ADD_MPI(&T, &T2, &T1));
#endif

	olen = ctx->len;
	MPI_CHK(MPI_WRITE_BINARY(&T, output, olen));
cleanup:

	MPI_FREE(&T);
	MPI_FREE(&T1);
	MPI_FREE(&T2);

	if (ret != 0)
		return POLARSSL_ERR_RSA_PRIVATE_FAILED + ret;

    return 0;
}

int rsa_pkcs1_encrypt( rsa_context_t *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, uint32_t ilen,
                       const uint8_t *input,
                       uint8_t *output )
{
    uint32_t nb_pad, olen, ret;
    uint8_t *p = output;
	USE_GLOBAL_BLOCK

    olen = ctx->len;

    if( f_rng == NULL )
        return( ERR_RSA_BAD_INPUT_DATA );

    switch( ctx->padding )
    {
        case RSA_PKCS_V15:

            if( olen < ilen + 11 )
                return( ERR_RSA_BAD_INPUT_DATA );

            nb_pad = olen - 3 - ilen;

            *p++ = 0;
            *p++ = RSA_CRYPT;

            while( nb_pad-- > 0 )
            {
                int rng_dl = 100;

                do {
                    ret = f_rng( p_rng, p, 1 );
                } while( *p == 0 && --rng_dl && ret == 0 );

                // Check if RNG failed to generate data
                //
                if( rng_dl == 0 || ret != 0)
                    return POLARSSL_ERR_RSA_RNG_FAILED + ret;

                p++;
            }
            *p++ = 0;
            MEMCPY(p, input, ilen);
            break;
        default:

            return( ERR_RSA_INVALID_PADDING );
    }

    return ((mode == RSA_PUBLIC_MODE) ? RSA_PUBLIC(ctx, output, output) : RSA_PRIVATE(ctx, output, output));
}

/*
 * Do an RSA operation, then remove the message padding
 */
int rsa_pkcs1_decrypt( rsa_context_t *ctx,
                       int mode, uint32_t *olen,
                       const uint8_t *input,
                       uint8_t *output,
                       uint32_t output_max_len)
{
    int ret;
    uint32_t ilen;
    uint8_t *p;
    uint8_t buf[1024];
	USE_GLOBAL_BLOCK

    ilen = ctx->len;

    if( ilen < 16 || ilen > sizeof( buf ) )
        return( ERR_RSA_BAD_INPUT_DATA );

    ret = (mode == RSA_PUBLIC_MODE) ? RSA_PUBLIC(ctx, input, buf) : RSA_PRIVATE(ctx, input, buf);

    if (ret != 0)
        return ret;

    p = buf;

    switch (ctx->padding) {
        case RSA_PKCS_V15:

            if( *p++ != 0 || *p++ != RSA_CRYPT )
                return( ERR_RSA_INVALID_PADDING );

            while (*p != 0) {
                if (p >= buf + ilen - 1)
                    return ERR_RSA_INVALID_PADDING;
                p++;
            }
            p++;
            break;
        default:

            return( ERR_RSA_INVALID_PADDING );
    }

    if (ilen - (p - buf) > output_max_len)
        return POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE;

    *olen = ilen - (uint32_t)(p - buf);
    MEMCPY(output, p, *olen);

    return 0;
}

// Do an RSA operation to sign the message digest.
int rsa_pkcs1_sign(rsa_context_t* ctx, int mode, int hash_id, unsigned int hashlen, const uint8_t *hash, uint8_t* sig)
{
    uint32_t nb_pad, olen;
    uint8_t *p = sig;
	USE_GLOBAL_BLOCK

    olen = ctx->len;

    switch (hash_id) {
        case SIG_RSA_RAW:
            nb_pad = olen - 3 - hashlen;
            break;
        case SIG_RSA_SHA1:
            nb_pad = olen - 3 - 35;
            break;
        default:
            return ERR_RSA_BAD_INPUT_DATA;
    }

    if (nb_pad < 8)
        return ERR_RSA_BAD_INPUT_DATA;

    *p++ = 0;
    *p++ = RSA_SIGN;
    MEMSET(p, 0xFF, nb_pad);
    p += nb_pad;
    *p++ = 0;

    switch (hash_id) {
        case SIG_RSA_RAW:
            MEMCPY(p, hash, hashlen);
            break;
        case SIG_RSA_SHA1:
            MEMCPY(p, RSA_ANS1_HASH_SHA1, 15);
            MEMCPY(p + 15, hash, 20);
            break;
        default:
            return ERR_RSA_BAD_INPUT_DATA;
    }

    return ((mode == RSA_PUBLIC_MODE) ? RSA_PUBLIC(ctx, sig, sig) : RSA_PRIVATE(ctx, sig, sig));
}

// Do an RSA operation and check the message digest.
int rsa_pkcs1_verify(rsa_context_t* ctx, int mode, int hash_id, unsigned int hashlen, const uint8_t* hash, uint8_t* sig)
{
    int ret;
    uint32_t len, siglen;
    uint8_t *p;
	uint8_t buf[1024];    
    USE_GLOBAL_BLOCK

    siglen = ctx->len;
	

    if (siglen < 16 || siglen > sizeof(buf))
        return ERR_RSA_BAD_INPUT_DATA;

    ret = (mode == RSA_PUBLIC_MODE) ? RSA_PUBLIC(ctx, sig, buf) : RSA_PRIVATE(ctx, sig, buf);

    if (ret != 0)
        return ret;

    p = buf;

    if (*p++ != 0 || *p++ != RSA_SIGN)
        return ERR_RSA_INVALID_PADDING;

    while (*p != 0) {
        if (p >= buf + siglen - 1 || *p != 0xFF)
            return ERR_RSA_INVALID_PADDING;
        p++;
    }
    p++;

    len = siglen - (uint32_t)(p - buf);

    if (len == 35 && hash_id == SIG_RSA_SHA1) {
        if (MEMCMP(p, RSA_ANS1_HASH_SHA1, 15) && MEMCMP(p + 15, hash, 20))
            return 0;
        else
            return ERR_RSA_VERIFY_FAILED;
    }

    if (len == hashlen && hash_id == SIG_RSA_RAW) {
        if (MEMCMP(p, hash, hashlen))
            return 0;
        else
            return ERR_RSA_VERIFY_FAILED;
    }

    return ERR_RSA_INVALID_PADDING;
}

#ifdef USE_RSA_PRIVATE_DECRYPT_HASH
int rsa_private_decrypt_hash(rsa_context_t* ctx, uint8_t* sig, uint8_t* hash, int* pHashSize)
{
	int ret;
	uint32_t len, siglen;
	uint8_t *p;
	uint8_t buf[128];
	USE_GLOBAL_BLOCK

	siglen = ctx->len;

	if (siglen < 16 || siglen > sizeof(buf))
		return ERR_RSA_BAD_INPUT_DATA;

	ret = rsa_private(ctx, sig, buf);
	if (ret != 0) {
		return ret;
	}

	p = buf;

	if (*p++ != 0 || *p++ != RSA_SIGN) {
		return ERR_RSA_INVALID_PADDING;
	}

	while (*p != 0) {
		if (p >= buf + siglen - 1 || *p != 0xFF)
			return ERR_RSA_INVALID_PADDING;
		p++;
	}
	p++;

	len = siglen - (p - buf);

	if (len == 35) {
		if (MEMCMP(p, RSA_ANS1_HASH_SHA1, 15)) {
			MEMCPY(hash, p + 15, 20);
			*pHashSize = 20;
			return ERR_OK;
		}
		else
			return ERR_RSA_VERIFY_FAILED;
	} 

	return ERR_RSA_INVALID_PADDING;
}
#endif // USE_RSA_PRIVATE_DECRYPT_HASH

// Do an RSA operation and check the message digest.
int rsa_public_decrypt_hash(rsa_context_t* ctx, uint8_t* sig, uint8_t* hash, int* pHashSize)
{
	int ret = ERR_RSA_INVALID_PADDING;
	uint32_t len, siglen;
	uint8_t* p;
	uint8_t* buf;
    USE_GLOBAL_BLOCK
	
	buf = SYS_ALLOCATOR(512);
    if (buf == NULL)
        return ERR_RSA_INVALID_PADDING;

	siglen = ctx->len;
    do {
        if (siglen < 16 || siglen > 512)
            break;

        ret = RSA_PUBLIC(ctx, sig, buf);
        if (ret != 0)
            break;
 
        p = buf;
        ret = ERR_RSA_INVALID_PADDING;
        if (*p++ != 0 || *p++ != RSA_SIGN)
            break;
 
        ret = ERR_OK;
 
        while (*p != 0) {
            if (p >= buf + siglen - 1 || *p != 0xFF) {
                ret = ERR_RSA_INVALID_PADDING;
                break;                
            }
            p++;
        }
        p++;
 
        if (ret != ERR_OK)
            break;

        len = siglen - (uint32_t)(p - buf);

        if (len == 35) {
            if (MEMCMP(p, RSA_ANS1_HASH_SHA1, 15)) {
                MEMCPY(hash, p + 15, 20);
                *pHashSize = 20;
                ret = ERR_OK;
            }
            else
                ret = ERR_RSA_VERIFY_FAILED;
        }
    } while (0);

    SYS_DEALLOCATOR(buf);

	return ret;
}
