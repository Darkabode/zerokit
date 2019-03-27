/*
Описание:

    Данный файл содежит реализацию арифметики больших чисел.
   
    Перед подключением файла нужно определить следующие функции специфические для среды исполнения:
     * SYS_ALLOCATOR(sz) - выделяет память указанного размера.
     * SYS_DEALLOCATOR(ptr) - освобождает память по указанному адресу.
     * MEMSET(dest, val, size) - Инициализация данных значением val.
     * MEMCPY(dest, src, size) - копирование из источника в приёмник.
  
Зависимости:

   * shared_code/types.h
*/

#define ciL (sizeof(uint32_t))    // chars in limb.
#define biL (ciL << 3)            // bits  in limb.
#define biH (ciL << 2)            // half limb size.

// Convert between bits/chars and number of limbs
#define BITS_TO_LIMBS(i)  (((i) + biL - 1) / biL)
#define CHARS_TO_LIMBS(i) (((i) + ciL - 1) / ciL)

// Initialize one MPI
void mpi_init(mpi_t* X)
{
    X->s = 1;
    X->n = 0;
    X->p = NULL;
}

/*
 * Unallocate one MPI
 */
void mpi_free(mpi_t* X)
{
    USE_GLOBAL_BLOCK

    if (X->p != NULL) {
        SYS_DEALLOCATOR(X->p);
    }

    X->s = 1;
    X->n = 0;
    X->p = NULL;
}

// Enlarge to the specified number of limbs.
int mpi_grow(mpi_t* X, uint32_t nblimbs)
{
    uint32_t* p;
    USE_GLOBAL_BLOCK

    if (nblimbs > POLARSSL_MPI_MAX_LIMBS)
        return POLARSSL_ERR_MPI_MALLOC_FAILED;

    if (X->n < nblimbs) {
        if ((p = (uint32_t*)SYS_ALLOCATOR(nblimbs * ciL)) == NULL)
            return POLARSSL_ERR_MPI_MALLOC_FAILED;

        MEMSET(p, 0, nblimbs * ciL);

        if (X->p != NULL) {
            MEMCPY(p, X->p, X->n * ciL);
            SYS_DEALLOCATOR(X->p);
        }

        X->n = nblimbs;
        X->p = p;
    }

    return 0;
}

// Copy the contents of Y into X.
int mpi_copy(mpi_t* X, const mpi_t* Y)
{
    int ret;
    uint32_t i;
    USE_GLOBAL_BLOCK

    if (X == Y)
        return ERR_OK;

    for (i = Y->n - 1; i > 0; --i)
        if (Y->p[i] != 0)
            break;
    ++i;

    X->s = Y->s;

    MPI_CHK(MPI_GROW(X, i));

    MEMSET(X->p, 0, X->n * ciL);
    MEMCPY(X->p, Y->p, i * ciL);

cleanup:
    return ret;
}

// Swap the contents of X and Y.
void mpi_swap(mpi_t* X, mpi_t* Y)
{
    mpi_t T;
    USE_GLOBAL_BLOCK

    MEMCPY(&T, X, sizeof(mpi_t));
    MEMCPY(X, Y, sizeof(mpi_t));
    MEMCPY(Y, &T, sizeof( mpi_t));
}

// Set value from integer.
int mpi_lset(mpi_t* X, int32_t z)
{
    int ret;
    USE_GLOBAL_BLOCK

    MPI_CHK(MPI_GROW(X, 1));
    MEMSET(X->p, 0, X->n * ciL);

    X->p[0] = (z < 0) ? -z : z;
    X->s = (z < 0) ? -1 : 1;

cleanup:
    return ret;
}
// 
// // Get a specific bit.
// int mpi_get_bit(mpi_t* X, size_t pos)
// {
//     if (X->n * biL <= pos)
//         return 0;
// 
//     return (X->p[pos / biL] >> (pos % biL)) & 0x01;
// }
// 
// // Set a bit to a specific value of 0 or 1.
// int mpi_set_bit(mpi_t* X, size_t pos, uint8_t val)
// {
//     int ret = 0;
//     size_t off = pos / biL;
//     size_t idx = pos % biL;
//     USE_GLOBAL_BLOCK
// 
//     if( val != 0 && val != 1 )
//         return POLARSSL_ERR_MPI_BAD_INPUT_DATA;
//         
//     if( X->n * biL <= pos )
//     {
//         if( val == 0 )
//             return ( 0 );
// 
//         MPI_CHK(MPI_GROW(X, off + 1));
//     }
// 
//     X->p[off] = ( X->p[off] & ~( 0x01 << idx ) ) | ( val << idx );
// 
// cleanup:
//     
//     return( ret );
// }

// Return the number of least significant bits.
uint32_t mpi_lsb(const mpi_t* X)
{
    uint32_t i, j, count = 0;

    for (i = 0; i < X->n; ++i) {
        for (j = 0; j < biL; ++j, ++count) {
            if (((X->p[i] >> j ) & 1) != 0) {
                return count;
            }
        }
    }

    return 0;
}

// Return the number of most significant bits.
uint32_t mpi_msb(const mpi_t* X)
{
    uint32_t i, j;

    for (i = X->n - 1; i > 0; --i) {
        if (X->p[i] != 0) {
            break;
        }
    }

    for (j = biL; j > 0; --j) {
        if (((X->p[i] >> (j - 1)) & 1) != 0) {
            break;
        }
    }

    return (i * biL) + j;
}

// Return the total size in bytes.
uint32_t mpi_size(const mpi_t* X)
{
    USE_GLOBAL_BLOCK

    return((MPI_MSB(X) + 7) >> 3);
}

/*
 * Convert an ASCII character to digit value
 */
int mpi_get_digit(uint32_t* d, int radix, char c)
{
    *d = 255;

    if (c >= 0x30 && c <= 0x39)
        *d = c - 0x30;
    if (c >= 0x41 && c <= 0x46)
        *d = c - 0x37;
    if (c >= 0x61 && c <= 0x66)
        *d = c - 0x57;

    if (*d >= (uint32_t)radix)
        return POLARSSL_ERR_MPI_INVALID_CHARACTER;

    return 0;
}

// Helper for mpi_t multiplication.
void mpi_mul_hlp(uint32_t i, uint32_t* s, uint32_t* d, uint32_t b)
{
    uint32_t c = 0, t = 0;

#if defined(MULADDC_HUIT)
    for( ; i >= 8; i -= 8 )
    {
        MULADDC_INIT
        MULADDC_HUIT
        MULADDC_STOP
    }

    for( ; i > 0; i-- )
    {
        MULADDC_INIT
        MULADDC_CORE
        MULADDC_STOP
    }
#else
    for ( ; i >= 16; i -= 16) {
        MULADDC_INIT
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE

        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_STOP
    }

    for ( ; i >= 8; i -= 8) {
        MULADDC_INIT
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE

        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_STOP
    }

    for ( ; i > 0; i--) {
        MULADDC_INIT
        MULADDC_CORE
        MULADDC_STOP
    }
#endif

    t++;

    do {
        *d += c;
        c = (*d < c);
        d++;
    } while (c != 0);
}

// Baseline multiplication: X = A * B  (HAC 14.12).
int mpi_mul_mpi(mpi_t* X, const mpi_t* A, const mpi_t* B)
{
    int ret;
    uint32_t i, j;
    mpi_t TA, TB;
    USE_GLOBAL_BLOCK

    MPI_INIT(&TA);
    MPI_INIT(&TB);

    if (X == A) {
        MPI_CHK(MPI_COPY(&TA,A));
        A = &TA;
    }
    
    if (X == B) {
        MPI_CHK(MPI_COPY(&TB, B));
        B = &TB;
    }

    for( i = A->n; i > 0; i-- )
        if( A->p[i - 1] != 0 )
            break;

    for( j = B->n; j > 0; j-- )
        if( B->p[j - 1] != 0 )
            break;

    MPI_CHK(MPI_GROW(X, i + j));
    MPI_CHK(MPI_LSET(X, 0));

    for( i++; j > 0; j-- )
        MPI_MUL_HLP(i - 1, A->p, X->p + j - 1, B->p[j - 1]);

    X->s = A->s * B->s;

cleanup:

    MPI_FREE(&TB);
    MPI_FREE(&TA);

    return ret;
}

// Baseline multiplication: X = A * b.
int mpi_mul_int(mpi_t* X, const mpi_t* A, int32_t b)
{
    mpi_t _B;
    uint32_t p[1];
    USE_GLOBAL_BLOCK

    _B.s = 1;
    _B.n = 1;
    _B.p = p;
    p[0] = b;

    return MPI_MUL_MPI(X, A, &_B);
}

// Compare unsigned values.
int mpi_cmp_abs(const mpi_t* X, const mpi_t* Y)
{
    uint32_t i, j;

    for (i = X->n; i > 0; --i) {
        if (X->p[i - 1] != 0) {
            break;
        }
    }

    for (j = Y->n; j > 0; --j) {
        if (Y->p[j - 1] != 0) {
            break;
        }
    }

    if (i == 0 && j == 0) {
        return 0;
    }

    if (i > j) {
        return 1;
    }
    if (j > i) {
        return -1;
    }

    for ( ; i > 0; --i) {
        if (X->p[i - 1] > Y->p[i - 1]) {
            return 1;
        }
        if (X->p[i - 1] < Y->p[i - 1]) {
            return -1;
        }
    }

    return 0;
}

// Helper for mpi_t substraction.
void mpi_sub_hlp(uint32_t n, uint32_t* s, uint32_t* d)
{
    uint32_t i;
    uint32_t c, z;

    for(i = c = 0; i < n; ++i, ++s, ++d) {
        z = ( *d <  c );     *d -=  c;
        c = ( *d < *s ) + z; *d -= *s;
    }

    while (c != 0) {
        z = (*d < c);
        *d -= c;
        c = z;
        i++;
        d++;
    }
}

// Unsigned substraction: X = |A| - |B|  (HAC 14.9).
int mpi_sub_abs(mpi_t* X, const mpi_t* A, const mpi_t* B)
{
    mpi_t TB;
    int ret;
    uint32_t n;
    USE_GLOBAL_BLOCK

    if (MPI_CMP_ABS(A, B) < 0) {
        return POLARSSL_ERR_MPI_NEGATIVE_VALUE;
    }

    MPI_INIT(&TB);

    if (X == B) {
        MPI_CHK(MPI_COPY(&TB, B));
        B = &TB;
    }

    if (X != A)
        MPI_CHK(MPI_COPY(X, A));

    // X should always be positive as a result of unsigned substractions.
    X->s = 1;

    ret = 0;

    for( n = B->n; n > 0; n-- )
        if( B->p[n - 1] != 0 )
            break;

    MPI_SUB_HLP(n, B->p, X->p);

cleanup:
    MPI_FREE(&TB);

    return ret;
}

// Unsigned addition: X = |A| + |B|  (HAC 14.7).
int mpi_add_abs(mpi_t* X, const mpi_t* A, const mpi_t* B)
{
    int ret;
    uint32_t i, j;
    uint32_t *o, *p, c;
    USE_GLOBAL_BLOCK

    if (X == B) {
        const mpi_t* T = A;
        A = X;
        B = T;
    }

    if (X != A)
        MPI_CHK(MPI_COPY(X, A));
   
    /*
     * X should always be positive as a result of unsigned additions.
     */
    X->s = 1;

    for( j = B->n; j > 0; j-- )
        if( B->p[j - 1] != 0 )
            break;

    MPI_CHK(MPI_GROW(X, j));

    o = B->p; p = X->p; c = 0;

    for( i = 0; i < j; i++, o++, p++ )
    {
        *p +=  c; c  = ( *p <  c );
        *p += *o; c += ( *p < *o );
    }

    while( c != 0 )
    {
        if( i >= X->n )
        {
            MPI_CHK(MPI_GROW(X, i + 1));
            p = X->p + i;
        }

        *p += c; c = ( *p < c ); i++;
    }

cleanup:

    return( ret );
}

// Signed addition: X = A + B.
int mpi_add_mpi(mpi_t* X, const mpi_t* A, const mpi_t* B)
{
    int ret, s = A->s;
    USE_GLOBAL_BLOCK

    if (A->s * B->s < 0) {
        if (MPI_CMP_ABS(A, B) >= 0) {
            MPI_CHK(MPI_SUB_ABS(X, A, B));
            X->s =  s;
        }
        else {
            MPI_CHK(MPI_SUB_ABS(X, B, A));
            X->s = -s;
        }
    }
    else {
        MPI_CHK(MPI_ADD_ABS(X, A, B));
        X->s = s;
    }

cleanup:
    return ret;
}

// Signed addition: X = A + b.
int mpi_add_int(mpi_t* X, const mpi_t* A, int32_t b)
{
    mpi_t _B;
    uint32_t p[1];
    USE_GLOBAL_BLOCK

    p[0] = (b < 0) ? -b : b;
    _B.s = (b < 0) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return MPI_ADD_MPI(X, A, &_B);
}

// Signed substraction: X = A - B.
int mpi_sub_mpi(mpi_t* X, const mpi_t* A, const mpi_t* B)
{
    int ret, s = A->s;
    USE_GLOBAL_BLOCK

    if (A->s * B->s > 0) {
        if (MPI_CMP_ABS(A, B) >= 0) {
            MPI_CHK(MPI_SUB_ABS(X, A, B));
            X->s =  s;
        }
        else {
            MPI_CHK(MPI_SUB_ABS(X, B, A));
            X->s = -s;
        }
    }
    else {
        MPI_CHK(MPI_ADD_ABS(X, A, B));
        X->s = s;
    }

cleanup:
    return ret;
}

// Signed substraction: X = A - b.
int mpi_sub_int(mpi_t* X, const mpi_t* A, int32_t b)
{
    mpi_t _B;
    uint32_t p[1];
    USE_GLOBAL_BLOCK

    p[0] = (b < 0) ? -b : b;
    _B.s = (b < 0) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return MPI_SUB_MPI(X, A, &_B);
}

// Modulo: r = A mod b.
int mpi_mod_int(uint32_t* r, const mpi_t* A, int32_t b)
{
    uint32_t i;
    uint32_t x, y, z;

    if (b == 0)
        return POLARSSL_ERR_MPI_DIVISION_BY_ZERO;

    if (b < 0)
        return POLARSSL_ERR_MPI_NEGATIVE_VALUE;

    /*
     * handle trivial cases
     */
    if( b == 1 )
    {
        *r = 0;
        return( 0 );
    }

    if( b == 2 )
    {
        *r = A->p[0] & 1;
        return( 0 );
    }

    /*
     * general case
     */
    for( i = A->n, y = 0; i > 0; i-- )
    {
        x  = A->p[i - 1];
        y  = ( y << biH ) | ( x >> biH );
        z  = y / b;
        y -= z * b;

        x <<= biH;
        y  = ( y << biH ) | ( x >> biH );
        z  = y / b;
        y -= z * b;
    }

    /*
     * If A is negative, then the current y represents a negative value.
     * Flipping it to the positive side.
     */
    if( A->s < 0 && y != 0 )
        y = b - y;

    *r = y;

    return( 0 );
}

// Compare signed values.
int mpi_cmp_mpi(const mpi_t* X, const mpi_t* Y)
{
    uint32_t i, j;

    for( i = X->n; i > 0; i-- )
        if( X->p[i - 1] != 0 )
            break;

    for( j = Y->n; j > 0; j-- )
        if( Y->p[j - 1] != 0 )
            break;

    if( i == 0 && j == 0 )
        return 0;

    if (i > j)
        return X->s;
    if (j > i)
        return -Y->s;

    if (X->s > 0 && Y->s < 0)
        return 1;
    if (Y->s > 0 && X->s < 0)
        return -1;

    for ( ; i > 0; i--) {
        if (X->p[i - 1] > Y->p[i - 1])
            return X->s;
        if (X->p[i - 1] < Y->p[i - 1])
            return -X->s;
    }

    return 0;
}

// Compare signed values.
int mpi_cmp_int(const mpi_t* X, int32_t z)
{
    mpi_t Y;
    uint32_t p[1];
    USE_GLOBAL_BLOCK

    *p = (z < 0) ? -z : z;
    Y.s = (z < 0) ? -1 : 1;
    Y.n = 1;
    Y.p = p;

    return MPI_CMP_MPI(X, &Y);
}

// Left-shift: X <<= count.
int mpi_shift_l(mpi_t* X, uint32_t count)
{
    int ret;
    uint32_t i, v0, t1;
    uint32_t r0 = 0, r1;
    USE_GLOBAL_BLOCK

    v0 = count / (biL    );
    t1 = count & (biL - 1);

    i = MPI_MSB(X) + count;

    if (X->n * biL < i)
        MPI_CHK(MPI_GROW(X, BITS_TO_LIMBS(i)));

    ret = 0;

    // shift by count / limb_size.
    if (v0 > 0) {
        for (i = X->n; i > v0; i--)
            X->p[i - 1] = X->p[i - v0 - 1];

        for ( ; i > 0; i--)
            X->p[i - 1] = 0;
    }

    // shift by count % limb_size.
    if (t1 > 0) {
        for (i = v0; i < X->n; i++) {
            r1 = X->p[i] >> (biL - t1);
            X->p[i] <<= t1;
            X->p[i] |= r0;
            r0 = r1;
        }
    }

cleanup:
    return ret;
}

// Right-shift: X >>= count.
int mpi_shift_r(mpi_t* X, uint32_t count)
{
    uint32_t i, v0, v1;
    uint32_t r0 = 0, r1;

    v0 = count /  biL;
    v1 = count & (biL - 1);

    // shift by count / limb_size.
    if (v0 > 0) {
        for (i = 0; i < X->n - v0; ++i)
            X->p[i] = X->p[i + v0];

        for ( ; i < X->n; ++i)
            X->p[i] = 0;
    }

    // shift by count % limb_size.
    if (v1 > 0) {
        for (i = X->n; i > 0; i--) {
            r1 = X->p[i - 1] << (biL - v1);
            X->p[i - 1] >>= v1;
            X->p[i - 1] |= r0;
            r0 = r1;
        }
    }

    return 0 ;
}

// Division by mpi_t: A = Q * B + R  (HAC 14.20).
int mpi_div_mpi(mpi_t* Q, mpi_t* R, const mpi_t* A, const mpi_t* B)
{
    int ret;
    uint32_t i, n, t, k;
    mpi_t X, Y, Z, T1, T2;
    USE_GLOBAL_BLOCK

    if (MPI_CMP_INT(B, 0) == 0) {
        return POLARSSL_ERR_MPI_DIVISION_BY_ZERO;
    }

    MPI_INIT(&X);
    MPI_INIT(&Y);
    MPI_INIT(&Z);
    MPI_INIT(&T1);
    MPI_INIT(&T2);

    if (MPI_CMP_ABS(A, B) < 0) {
        if (Q != NULL)
            MPI_CHK(MPI_LSET(Q, 0));
        if (R != NULL)
            MPI_CHK(MPI_COPY(R, A));
        return 0;
    }

    MPI_CHK(MPI_COPY(&X, A));
    MPI_CHK(MPI_COPY(&Y, B));
    X.s = Y.s = 1;

    MPI_CHK(MPI_GROW(&Z, A->n + 2));
    MPI_CHK(MPI_LSET(&Z, 0));
    MPI_CHK(MPI_GROW(&T1, 2));
    MPI_CHK(MPI_GROW(&T2, 3));

    k = MPI_MSB(&Y) % biL;
    if (k < biL - 1) {
        k = biL - 1 - k;
        MPI_CHK(MPI_SHIFT_L(&X, k));
        MPI_CHK(MPI_SHIFT_L(&Y, k));
    }
    else k = 0;

    n = X.n - 1;
    t = Y.n - 1;
    MPI_SHIFT_L(&Y, biL * (n - t));

    while (MPI_CMP_MPI(&X, &Y) >= 0) {
        Z.p[n - t]++;
        MPI_SUB_MPI(&X, &X, &Y);
    }
    MPI_SHIFT_R(&Y, biL * (n - t));

    for( i = n; i > t ; i-- )
    {
        if( X.p[i] >= Y.p[t] )
            Z.p[i - t - 1] = (uint32_t)(~0);
        else
        {
            /*
             * __udiv_qrnnd_c, from gmp/longlong.h
             */
            uint32_t q0, q1, r0, r1;
            uint32_t d0, d1, d, m;

            d  = Y.p[t];
            d0 = ( d << biH ) >> biH;
            d1 = ( d >> biH );

            q1 = X.p[i] / d1;
            r1 = X.p[i] - d1 * q1;
            r1 <<= biH;
            r1 |= ( X.p[i - 1] >> biH );

            m = q1 * d0;
            if( r1 < m )
            {
                q1--, r1 += d;
                while( r1 >= d && r1 < m )
                    q1--, r1 += d;
            }
            r1 -= m;

            q0 = r1 / d1;
            r0 = r1 - d1 * q0;
            r0 <<= biH;
            r0 |= ( X.p[i - 1] << biH ) >> biH;

            m = q0 * d0;
            if( r0 < m )
            {
                q0--, r0 += d;
                while( r0 >= d && r0 < m )
                    q0--, r0 += d;
            }
            r0 -= m;

            Z.p[i - t - 1] = ( q1 << biH ) | q0;
        }

        Z.p[i - t - 1]++;
        do {
            Z.p[i - t - 1]--;

            MPI_CHK(MPI_LSET(&T1, 0));
            T1.p[0] = (t < 1) ? 0 : Y.p[t - 1];
            T1.p[1] = Y.p[t];
            MPI_CHK(MPI_MUL_INT(&T1, &T1, Z.p[i - t - 1]));

            MPI_CHK(MPI_LSET(&T2, 0));
            T2.p[0] = (i < 2) ? 0 : X.p[i - 2];
            T2.p[1] = (i < 1) ? 0 : X.p[i - 1];
            T2.p[2] = X.p[i];
        } while(MPI_CMP_MPI(&T1, &T2) > 0);

        MPI_CHK(MPI_MUL_INT(&T1, &Y, Z.p[i - t - 1]));
        MPI_CHK(MPI_SHIFT_L(&T1,  biL * (i - t - 1)));
        MPI_CHK(MPI_SUB_MPI(&X, &X, &T1));

        if (MPI_CMP_INT(&X, 0) < 0) {
            MPI_CHK(MPI_COPY(&T1, &Y));
            MPI_CHK(MPI_SHIFT_L(&T1, biL * (i - t - 1)));
            MPI_CHK(MPI_ADD_MPI(&X, &X, &T1));
            Z.p[i - t - 1]--;
        }
    }

    if (Q != NULL) {
        MPI_COPY(Q, &Z);
        Q->s = A->s * B->s;
    }

    if (R != NULL) {
        MPI_SHIFT_R(&X, k);
        MPI_COPY(R, &X);

        R->s = A->s;
        if (MPI_CMP_INT(R, 0) == 0)
            R->s = 1;
    }

cleanup:
    MPI_FREE(&X);
    MPI_FREE(&Y);
    MPI_FREE(&Z);
    MPI_FREE(&T1);
    MPI_FREE(&T2);

    return ret;
}

/*
 * Division by int: A = Q * b + R
 *
 * Returns 0 if successful
 *         1 if memory allocation failed
 *         POLARSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
 */
int mpi_div_int(mpi_t* Q, mpi_t* R, const mpi_t* A, int32_t b)
{
    mpi_t _B;
    uint32_t p[1];
    USE_GLOBAL_BLOCK

    p[0] = (b < 0) ? -b : b;
    _B.s = (b < 0) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return MPI_DIV_MPI(Q, R, A, &_B);
}

// Import X from unsigned binary data, big endian.
int mpi_read_binary(mpi_t* X, const uint8_t* buf, uint32_t buflen)
{
    int ret;
    uint32_t i, j, n;
    USE_GLOBAL_BLOCK

    for( n = 0; n < buflen; n++ )
        if( buf[n] != 0 )
            break;

    MPI_CHK(MPI_GROW(X, CHARS_TO_LIMBS(buflen - n)));
    MPI_CHK(MPI_LSET(X, 0));

    for( i = buflen, j = 0; i > n; i--, j++ )
        X->p[j / ciL] |= ((uint32_t) buf[i - 1]) << ((j % ciL) << 3);

cleanup:
    return ret;
}

// Export X into unsigned binary data, big endian.
int mpi_write_binary(const mpi_t* X, uint8_t* buf, uint32_t buflen)
{
    uint32_t i, j, n;
    USE_GLOBAL_BLOCK

    n = MPI_SIZE(X);

    if( buflen < n )
        return( POLARSSL_ERR_MPI_BUFFER_TOO_SMALL );

    MEMSET( buf, 0, buflen );

    for( i = buflen - 1, j = 0; n > 0; i--, j++, n-- )
        buf[i] = (uint8_t)( X->p[j / ciL] >> ((j % ciL) << 3) );

    return( 0 );
}

// Modulo: R = A mod B.
int mpi_mod_mpi(mpi_t* R, const mpi_t* A, const mpi_t* B)
{
    int ret;
    USE_GLOBAL_BLOCK

    if (MPI_CMP_INT(B, 0) < 0)
        return POLARSSL_ERR_MPI_NEGATIVE_VALUE;

    MPI_CHK(MPI_DIV_MPI(NULL, R, A, B));

    while (MPI_CMP_INT(R, 0) < 0) {
      MPI_CHK(MPI_ADD_MPI(R, R, B));
    }

    while (MPI_CMP_MPI(R, B) >= 0) {
      MPI_CHK(MPI_SUB_MPI(R, R, B));
    }

cleanup:
    return ret;
}

// Fast Montgomery initialization (thanks to Tom St Denis).
void mpi_montg_init(uint32_t* mm, const mpi_t* N)
{
    uint32_t x, m0 = N->p[0];

    x  = m0;
    x += ( ( m0 + 2 ) & 4 ) << 1;
    x *= ( 2 - ( m0 * x ) );

    /*if( biL >= 16 ) */x *= ( 2 - ( m0 * x ) );
    /*if( biL >= 32 ) */x *= ( 2 - ( m0 * x ) );
//    if( biL >= 64 ) x *= ( 2 - ( m0 * x ) );

    *mm = ~x + 1;
}

// Montgomery multiplication: A = A * B * R^-1 mod N  (HAC 14.36).
void mpi_montmul(mpi_t* A, const mpi_t* B, const mpi_t* N, uint32_t mm, const mpi_t* T)
{
    uint32_t i, n, m;
    uint32_t u0, u1, *d;
    USE_GLOBAL_BLOCK

    MEMSET(T->p, 0, T->n * ciL);

    d = T->p;
    n = N->n;
    m = (B->n < n) ? B->n : n;

    for (i = 0; i < n; ++i) {
        // T = (T + u0*B + u1*N) / 2^biL.
        u0 = A->p[i];
        u1 = ( d[0] + u0 * B->p[0] ) * mm;

        MPI_MUL_HLP(m, B->p, d, u0);
        MPI_MUL_HLP(n, N->p, d, u1);

        *d++ = u0; d[n + 1] = 0;
    }

    MEMCPY( A->p, d, (n + 1) * ciL );

    if (MPI_CMP_ABS(A, N) >= 0) {
        MPI_SUB_HLP(n, N->p, A->p);
    }
    else {
        // prevent timing attacks.
        MPI_SUB_HLP(n, A->p, T->p);
    }
}

// Montgomery reduction: A = A * R^-1 mod N.
void mpi_montred(mpi_t* A, const mpi_t* N, uint32_t mm, const mpi_t* T)
{
    uint32_t z = 1;
    mpi_t U;
    USE_GLOBAL_BLOCK

    U.n = U.s = z;
    U.p = &z;

    MPI_MONTMUL(A, &U, N, mm, T);
}

// Sliding-window exponentiation: X = A^E mod N  (HAC 14.85).
int mpi_exp_mod(mpi_t* X, const mpi_t* A, const mpi_t* E, const mpi_t* N, mpi_t* _RR)
{
    int ret;
    uint32_t wbits, wsize, one = 1;
    uint32_t i, j, nblimbs;
    uint32_t bufsize, nbits;
    uint32_t ei, mm, state;
    mpi_t RR, T, W[2 << POLARSSL_MPI_WINDOW_SIZE], Apos;
    int neg;
    USE_GLOBAL_BLOCK

    if (MPI_CMP_INT(N, 0) < 0 || (N->p[0] & 1) == 0) {
        return POLARSSL_ERR_MPI_BAD_INPUT_DATA;
    }
    
    if (MPI_CMP_INT(E, 0) < 0) {
        return POLARSSL_ERR_MPI_BAD_INPUT_DATA;
    }

    // Init temps and window size.
    MPI_MONTG_INIT(&mm, N);
    MPI_INIT(&RR);
    MPI_INIT(&T);

    MEMSET(W, 0, sizeof(W));

    i = MPI_MSB(E);

    wsize = ( i > 671 ) ? 6 : ( i > 239 ) ? 5 :
            ( i >  79 ) ? 4 : ( i >  23 ) ? 3 : 1;

    if (wsize > POLARSSL_MPI_WINDOW_SIZE)
        wsize = POLARSSL_MPI_WINDOW_SIZE;

    j = N->n + 1;
    MPI_CHK(MPI_GROW(X, j));
    MPI_CHK(MPI_GROW(&W[1],  j));
    MPI_CHK(MPI_GROW(&T, j * 2));
    
    /*
     * Compensate for negative A (and correct at the end)
     */
    neg = (A->s == -1);

    MPI_INIT(&Apos);
    
    if (neg) {
        MPI_CHK(MPI_COPY(&Apos, A));
        Apos.s = 1;
        A = &Apos;
    }

    // If 1st call, pre-compute R^2 mod N.
    if (_RR == NULL || _RR->p == NULL) {
        MPI_CHK(MPI_LSET(&RR, 1));
        MPI_CHK(MPI_SHIFT_L(&RR, N->n * 2 * biL));
        MPI_CHK(MPI_MOD_MPI(&RR, &RR, N));

        if (_RR != NULL) {
            MEMCPY(_RR, &RR, sizeof(mpi_t));
        }
    }
    else {
        MEMCPY(&RR, _RR, sizeof(mpi_t));
    }

    // W[1] = A * R^2 * R^-1 mod N = A * R mod N.
    if (MPI_CMP_MPI(A, N) >= 0) {
        MPI_MOD_MPI(&W[1], A, N);
    }
    else {
        MPI_COPY(&W[1], A);
    }

    MPI_MONTMUL(&W[1], &RR, N, mm, &T);

    // X = R^2 * R^-1 mod N = R mod N.
    MPI_CHK(MPI_COPY(X, &RR));
    MPI_MONTRED(X, N, mm, &T);

    if (wsize > 1) {
        // W[1 << (wsize - 1)] = W[1] ^ (wsize - 1).
        j =  one << (wsize - 1);

        MPI_CHK(MPI_GROW(&W[j], N->n + 1));
        MPI_CHK(MPI_COPY(&W[j], &W[1]));

        for( i = 0; i < wsize - 1; i++ )
            MPI_MONTMUL(&W[j], &W[j], N, mm, &T);
    
        // W[i] = W[i - 1] * W[1].
        for (i = j + 1; i < (one << wsize); ++i) {
            MPI_CHK(MPI_GROW(&W[i], N->n + 1));
            MPI_CHK(MPI_COPY(&W[i], &W[i - 1]));

            MPI_MONTMUL(&W[i], &W[1], N, mm, &T);
        }
    }

    nblimbs = E->n;
    bufsize = 0;
    nbits   = 0;
    wbits   = 0;
    state   = 0;

    while (1) {
        if (bufsize == 0) {
            if (nblimbs-- == 0)
                break;

            bufsize = sizeof(uint32_t) << 3;
        }

        bufsize--;

        ei = (E->p[nblimbs] >> bufsize) & 1;

        // skip leading 0s.
        if (ei == 0 && state == 0)
            continue;

        if (ei == 0 && state == 1) {
            // out of window, square X.
            MPI_MONTMUL(X, X, N, mm, &T);
            continue;
        }

        /*
         * add ei to current window
         */
        state = 2;

        nbits++;
        wbits |= (ei << (wsize - nbits));

        if( nbits == wsize )
        {
            /*
             * X = X^wsize R^-1 mod N
             */
            for( i = 0; i < wsize; i++ )
                MPI_MONTMUL(X, X, N, mm, &T);

            /*
             * X = X * W[wbits] R^-1 mod N
             */
            MPI_MONTMUL(X, &W[wbits], N, mm, &T);

            state--;
            nbits = 0;
            wbits = 0;
        }
    }

    // process the remaining bits.
    for (i = 0; i < nbits; ++i) {
        MPI_MONTMUL(X, X, N, mm, &T);

        wbits <<= 1;

        if ((wbits & (one << wsize)) != 0) {
            MPI_MONTMUL(X, &W[1], N, mm, &T);
        }
    }

    // X = A^E * R * R^-1 mod N = A^E mod N.
    MPI_MONTRED(X, N, mm, &T);

    if (neg) {
        X->s = -1;
        MPI_ADD_MPI(X, N, X);
    }
    
cleanup:
    for (i = (one << (wsize - 1)); i < (one << wsize); ++i) {
        MPI_FREE(&W[i]);
    }

    MPI_FREE(&W[1]);
    MPI_FREE(&T);
    MPI_FREE(&Apos);

    if (_RR == NULL) {
        MPI_FREE(&RR);
    }

    return ret;
}

// Greatest common divisor: G = gcd(A, B)  (HAC 14.54).
int mpi_gcd(mpi_t* G, const mpi_t* A, const mpi_t* B)
{
    int ret;
    uint32_t lz, lzt;
    mpi_t TG, TA, TB;
    USE_GLOBAL_BLOCK

    MPI_INIT(&TG);
    MPI_INIT(&TA);
    MPI_INIT(&TB);

    MPI_CHK(MPI_COPY(&TA, A));
    MPI_CHK(MPI_COPY(&TB, B));

    lz = MPI_LSB(&TA);
    lzt = MPI_LSB(&TB);

    if (lzt < lz)
        lz = lzt;

    MPI_CHK(MPI_SHIFT_R(&TA, lz));
    MPI_CHK(MPI_SHIFT_R(&TB, lz));

    TA.s = TB.s = 1;

    while (MPI_CMP_INT(&TA, 0) != 0) {
        MPI_CHK(MPI_SHIFT_R(&TA, MPI_LSB(&TA)));
        MPI_CHK(MPI_SHIFT_R(&TB, MPI_LSB(&TB)));

        if (MPI_CMP_MPI(&TA, &TB) >= 0) {
            MPI_CHK(MPI_SUB_ABS(&TA, &TA, &TB));
            MPI_CHK(MPI_SHIFT_R(&TA, 1));
        }
        else {
            MPI_CHK(MPI_SUB_ABS(&TB, &TB, &TA));
            MPI_CHK(MPI_SHIFT_R(&TB, 1));
        }
    }

    MPI_CHK(MPI_SHIFT_L(&TB, lz));
    MPI_CHK(MPI_COPY(G, &TB));

cleanup:

    MPI_FREE(&TG);
    MPI_FREE(&TA);
    MPI_FREE(&TB);

    return ret;
}

int mpi_fill_random( mpi_t *X, uint32_t size, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret;
    USE_GLOBAL_BLOCK

    MPI_CHK(MPI_GROW(X, CHARS_TO_LIMBS(size)));
    MPI_CHK(MPI_LSET(X, 0));

    f_rng(p_rng, (unsigned char*)X->p, size);

cleanup:
    return ret;
}

#ifdef USE_PRIME_NUMBERS

// Modular inverse: X = A^-1 mod N  (HAC 14.61 / 14.64).
int mpi_inv_mod(mpi_t* X, const mpi_t* A, const mpi_t* N)
{
    int ret;
    mpi_t G, TA, TU, U1, U2, TB, TV, V1, V2;
    USE_GLOBAL_BLOCK

        if (MPI_CMP_INT(N, 0) <= 0)
            return POLARSSL_ERR_MPI_BAD_INPUT_DATA;

    MPI_INIT(&TA);
    MPI_INIT(&TU);
    MPI_INIT(&U1);
    MPI_INIT(&U2);
    MPI_INIT(&G);
    MPI_INIT(&TB);
    MPI_INIT(&TV);
    MPI_INIT(&V1);
    MPI_INIT(&V2);

    MPI_CHK(MPI_GCD(&G, A, N));

    if (MPI_CMP_INT(&G, 1) != 0) {
        ret = POLARSSL_ERR_MPI_NOT_ACCEPTABLE;
        goto cleanup;
    }

    MPI_CHK(MPI_MOD_MPI(&TA, A, N));
    MPI_CHK(MPI_COPY(&TU, &TA));
    MPI_CHK(MPI_COPY(&TB, N));
    MPI_CHK(MPI_COPY(&TV, N));

    MPI_CHK(MPI_LSET(&U1, 1));
    MPI_CHK(MPI_LSET(&U2, 0));
    MPI_CHK(MPI_LSET(&V1, 0));
    MPI_CHK(MPI_LSET(&V2, 1));

    do {
        while ((TU.p[0] & 1) == 0) {
            MPI_CHK(MPI_SHIFT_R(&TU, 1));

            if ((U1.p[0] & 1) != 0 || (U2.p[0] & 1) != 0) {
                MPI_CHK(MPI_ADD_MPI(&U1, &U1, &TB));
                MPI_CHK(MPI_SUB_MPI(&U2, &U2, &TA));
            }

            MPI_CHK(MPI_SHIFT_R(&U1, 1));
            MPI_CHK(MPI_SHIFT_R(&U2, 1));
        }

        while( ( TV.p[0] & 1 ) == 0 )
        {
            MPI_CHK(MPI_SHIFT_R(&TV, 1));

            if ((V1.p[0] & 1) != 0 || (V2.p[0] & 1) != 0) {
                MPI_CHK(MPI_ADD_MPI(&V1, &V1, &TB));
                MPI_CHK(MPI_SUB_MPI(&V2, &V2, &TA));
            }

            MPI_CHK(MPI_SHIFT_R(&V1, 1));
            MPI_CHK(MPI_SHIFT_R(&V2, 1));
        }

        if (MPI_CMP_MPI(&TU, &TV) >= 0) {
            MPI_CHK(MPI_SUB_MPI(&TU, &TU, &TV));
            MPI_CHK(MPI_SUB_MPI(&U1, &U1, &V1));
            MPI_CHK(MPI_SUB_MPI(&U2, &U2, &V2));
        }
        else {
            MPI_CHK(MPI_SUB_MPI(&TV, &TV, &TU));
            MPI_CHK(MPI_SUB_MPI(&V1, &V1, &U1));
            MPI_CHK(MPI_SUB_MPI(&V2, &V2, &U2));
        }
    } while (MPI_CMP_INT(&TU, 0) != 0);

    while (MPI_CMP_INT(&V1, 0) < 0) { 
        MPI_CHK(MPI_ADD_MPI(&V1, &V1, N));
    }

    while (MPI_CMP_MPI(&V1, N) >= 0) {
        MPI_CHK(MPI_SUB_MPI(&V1, &V1, N));
    }

    MPI_CHK(MPI_COPY(X, &V1));

cleanup:

    MPI_FREE(&TA);
    MPI_FREE(&TU);
    MPI_FREE(&U1);
    MPI_FREE(&U2);
    MPI_FREE(&G);
    MPI_FREE(&TB);
    MPI_FREE(&TV);
    MPI_FREE(&V1);
    MPI_FREE(&V2);

    return ret;
}

static const int small_prime[] =
{
        3,    5,    7,   11,   13,   17,   19,   23,
       29,   31,   37,   41,   43,   47,   53,   59,
       61,   67,   71,   73,   79,   83,   89,   97,
      101,  103,  107,  109,  113,  127,  131,  137,
      139,  149,  151,  157,  163,  167,  173,  179,
      181,  191,  193,  197,  199,  211,  223,  227,
      229,  233,  239,  241,  251,  257,  263,  269,
      271,  277,  281,  283,  293,  307,  311,  313,
      317,  331,  337,  347,  349,  353,  359,  367,
      373,  379,  383,  389,  397,  401,  409,  419,
      421,  431,  433,  439,  443,  449,  457,  461,
      463,  467,  479,  487,  491,  499,  503,  509,
      521,  523,  541,  547,  557,  563,  569,  571,
      577,  587,  593,  599,  601,  607,  613,  617,
      619,  631,  641,  643,  647,  653,  659,  661,
      673,  677,  683,  691,  701,  709,  719,  727,
      733,  739,  743,  751,  757,  761,  769,  773,
      787,  797,  809,  811,  821,  823,  827,  829,
      839,  853,  857,  859,  863,  877,  881,  883,
      887,  907,  911,  919,  929,  937,  941,  947,
      953,  967,  971,  977,  983,  991,  997, -103
};

/*
 * Miller-Rabin primality test  (HAC 4.24)
 */
int mpi_is_prime( mpi_t *X, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret, xs;
    uint32_t i, j, n, s;
    mpi_t W, R, T, A, RR;

    if (mpi_cmp_int( X, 0 ) == 0 || mpi_cmp_int( X, 1 ) == 0 ) {
        return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );
    }

    if (mpi_cmp_int( X, 2 ) == 0) {
        return 0;
    }

    mpi_init(&W);
    mpi_init(&R);
    mpi_init(&T);
    mpi_init(&A);
    mpi_init(&RR);

    xs = X->s; X->s = 1;

    /*
     * test trivial factors first
     */
    if( ( X->p[0] & 1 ) == 0 )
        return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );

    for( i = 0; small_prime[i] > 0; i++ )
    {
        uint32_t r;

        if( mpi_cmp_int( X, small_prime[i] ) <= 0 )
            return( 0 );

        MPI_CHK( mpi_mod_int( (uint32_t*)&r, X, small_prime[i] ) );

        if( r == 0 )
            return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );
    }

    /*
     * W = |X| - 1
     * R = W >> lsb( W )
     */
    MPI_CHK( mpi_sub_int( &W, X, 1 ) );
    s = mpi_lsb( &W );
    MPI_CHK( mpi_copy( &R, &W ) );
    MPI_CHK( mpi_shift_r( &R, s ) );

    i = mpi_msb( X );
    /*
     * HAC, table 4.4
     */
    n = ( ( i >= 1300 ) ?  2 : ( i >=  850 ) ?  3 :
          ( i >=  650 ) ?  4 : ( i >=  350 ) ?  8 :
          ( i >=  250 ) ? 12 : ( i >=  150 ) ? 18 : 27 );

    for( i = 0; i < n; i++ )
    {
        /*
         * pick a random A, 1 < A < |X| - 1
         */
        MPI_CHK(mpi_fill_random(&A, X->n * ciL, f_rng, p_rng));

        if( mpi_cmp_mpi( &A, &W ) >= 0 )
        {
            j = mpi_msb( &A ) - mpi_msb( &W );
            MPI_CHK( mpi_shift_r( &A, j + 1 ) );
        }
        A.p[0] |= 3;

        /*
         * A = A^R mod |X|
         */
        MPI_CHK( mpi_exp_mod( &A, &A, &R, X, &RR ) );

        if( mpi_cmp_mpi( &A, &W ) == 0 ||
            mpi_cmp_int( &A,  1 ) == 0 )
            continue;

        j = 1;
        while( j < s && mpi_cmp_mpi( &A, &W ) != 0 )
        {
            /*
             * A = A * A mod |X|
             */
            MPI_CHK( mpi_mul_mpi( &T, &A, &A ) );
            MPI_CHK( mpi_mod_mpi( &A, &T, X  ) );

            if( mpi_cmp_int( &A, 1 ) == 0 )
                break;

            j++;
        }

        /*
         * not prime if A != |X| - 1 or A == 1
         */
        if( mpi_cmp_mpi( &A, &W ) != 0 ||
            mpi_cmp_int( &A,  1 ) == 0 )
        {
            ret = POLARSSL_ERR_MPI_NOT_ACCEPTABLE;
            break;
        }
    }

cleanup:

    X->s = xs;

    mpi_free( &W ); mpi_free( &R ); mpi_free( &T ); mpi_free( &A );
    mpi_free( &RR );

    return( ret );
}

/*
 * Prime number generation
 */
int mpi_gen_prime( mpi_t *X, uint32_t nbits, int dh_flag, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret;
    uint32_t k, n;
    mpi_t Y;

    if( nbits < 3 || nbits > POLARSSL_MPI_MAX_BITS )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    mpi_init( &Y );

    n = BITS_TO_LIMBS( nbits );

    MPI_CHK( mpi_fill_random( X, n * ciL, f_rng, p_rng ) );

    k = mpi_msb( X );
    if( k < nbits ) MPI_CHK( mpi_shift_l( X, nbits - k ) );
    if( k > nbits ) MPI_CHK( mpi_shift_r( X, k - nbits ) );

    X->p[0] |= 3;

    if( dh_flag == 0 )
    {
        while( ( ret = mpi_is_prime( X, f_rng, p_rng ) ) != 0 )
        {
            if( ret != POLARSSL_ERR_MPI_NOT_ACCEPTABLE )
                goto cleanup;

            MPI_CHK( mpi_add_int( X, X, 2 ) );
        }
    }
    else
    {
        MPI_CHK( mpi_sub_int( &Y, X, 1 ) );
        MPI_CHK( mpi_shift_r( &Y, 1 ) );

        while( 1 )
        {
            if( ( ret = mpi_is_prime( X, f_rng, p_rng ) ) == 0 )
            {
                if( ( ret = mpi_is_prime( &Y, f_rng, p_rng ) ) == 0 )
                    break;

                if( ret != POLARSSL_ERR_MPI_NOT_ACCEPTABLE )
                    goto cleanup;
            }

            if( ret != POLARSSL_ERR_MPI_NOT_ACCEPTABLE )
                goto cleanup;

            MPI_CHK( mpi_add_int( &Y, X, 1 ) );
            MPI_CHK( mpi_add_int(  X, X, 2 ) );
            MPI_CHK( mpi_shift_r( &Y, 1 ) );
        }
    }

cleanup:

    mpi_free( &Y );

    return( ret );
}

#endif // USE_PRIME_NUMBERS

#ifdef USE_STRING_IO

// Import from an ASCII string.
int mpi_read_string(mpi_t* X, int radix, const char* s)
{
    int ret;
    uint32_t i, j, slen, n;
    uint32_t d;
    mpi_t T;
    USE_GLOBAL_BLOCK

    if( radix < 2 || radix > 16 ) {
        return POLARSSL_ERR_MPI_BAD_INPUT_DATA;
    }

    MPI_INIT(&T);

    slen = strlen(s);

    if (radix == 16) {
        n = BITS_TO_LIMBS(slen << 2);

        MPI_CHK(MPI_GROW(X, n));
        MPI_CHK(MPI_LSET(X, 0));

        for (i = slen, j = 0; i > 0; i--, j++) {
            if (i == 1 && s[i - 1] == '-') {
                X->s = -1;
                break;
            }

            MPI_CHK(MPI_GET_DIGIT(&d, radix, s[i - 1]));
            X->p[j / (2 * ciL)] |= d << ( (j % (2 * ciL)) << 2 );
        }
    }
    else {
        MPI_CHK(MPI_LSET(X, 0));

        for (i = 0; i < slen; i++) {
            if (i == 0 && s[i] == '-') {
                X->s = -1;
                continue;
            }

            MPI_CHK(MPI_GET_DIGIT(&d, radix, s[i]));
            MPI_CHK(MPI_MUL_INT(&T, X, radix));

            if (X->s == 1) {
                MPI_CHK(MPI_ADD_INT(X, &T, d));
            }
            else {
                MPI_CHK(MPI_SUB_INT(X, &T, d));
            }
        }
    }

cleanup:

    MPI_FREE(&T);

    return ret;
}

#ifdef USE_STRING_WRITE_IO

// Helper to write the digits high-order first.
int mpi_write_hlp(mpi_t* X, int radix, char** p)
{
    int ret;
    uint32_t r;

    if( radix < 2 || radix > 16 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    MPI_CHK( mpi_mod_int( &r, X, radix ) );
    MPI_CHK( mpi_div_int( X, NULL, X, radix ) );

    if( mpi_cmp_int( X, 0 ) != 0 )
        MPI_CHK( mpi_write_hlp( X, radix, p ) );

    if( r < 10 )
        *(*p)++ = (char)( r + 0x30 );
    else
        *(*p)++ = (char)( r + 0x37 );

cleanup:

    return( ret );
}

// Export into an ASCII string.
int mpi_write_string(const mpi_t* X, int radix, char* s, uint32_t* slen)
{
    int ret = 0;
    uint32_t n;
    char *p;
    mpi_t T;

    if( radix < 2 || radix > 16 )
        return( POLARSSL_ERR_MPI_BAD_INPUT_DATA );

    n = mpi_msb( X );
    if( radix >=  4 ) n >>= 1;
    if( radix >= 16 ) n >>= 1;
    n += 3;

    if( *slen < n )
    {
        *slen = n;
        return( POLARSSL_ERR_MPI_BUFFER_TOO_SMALL );
    }

    p = s;
    mpi_init( &T );

    if( X->s == -1 )
        *p++ = '-';

    if( radix == 16 )
    {
        int c;
        uint32_t i, j, k;

        for( i = X->n, k = 0; i > 0; i-- )
        {
            for( j = ciL; j > 0; j-- )
            {
                c = ( X->p[i - 1] >> ( ( j - 1 ) << 3) ) & 0xFF;

                if( c == 0 && k == 0 && ( i + j + 3 ) != 0 )
                    continue;

                p += sprintf( p, "%02X", c );
                k = 1;
            }
        }
    }
    else
    {
        MPI_CHK( mpi_copy( &T, X ) );

        if( T.s == -1 )
            T.s = 1;

        MPI_CHK( mpi_write_hlp( &T, radix, &p ) );
    }

    *p++ = '\0';
    *slen = p - s;

cleanup:

    mpi_free( &T );

    return( ret );
}

#endif // USE_STRING_WRITE_IO

// Read X from an opened file
int mpi_read_file(mpi_t *X, int radix, FILE* fin)
{
    uint32_t d;
    uint32_t slen;
    char *p;
    /*
     * Buffer should have space for (short) label and decimal formatted MPI,
     * newline characters and '\0'
     */
    char s[ POLARSSL_MPI_READ_BUFFER_SIZE ];
    USE_GLOBAL_BLOCK

    MEMSET( s, 0, sizeof( s ) );
    if( fgets( s, sizeof( s ) - 1, fin ) == NULL )
        return( POLARSSL_ERR_MPI_FILE_IO_ERROR );

    slen = strlen( s );
    if( slen == sizeof( s ) - 2 )
        return( POLARSSL_ERR_MPI_BUFFER_TOO_SMALL );

    if( s[slen - 1] == '\n' ) { slen--; s[slen] = '\0'; }
    if( s[slen - 1] == '\r' ) { slen--; s[slen] = '\0'; }

    p = s + slen;
    while( --p >= s )
        if (MPI_GET_DIGIT(&d, radix, *p) != 0)
            break;

    return( mpi_read_string( X, radix, p + 1 ) );
}

#endif // USE_STRING_IO

#ifdef MPI_USE_FILE_IO

// Write X into an opened file (or stdout if fout == NULL).
int mpi_write_file( const char *p, const mpi_t *X, int radix, FILE *fout )
{
    int ret;
    uint32_t n, slen, plen;
    /*
     * Buffer should have space for minus sign, hexified MPI and '\0'
     */
    char s[ 2 * POLARSSL_MPI_MAX_SIZE + 5 ];
    USE_GLOBAL_BLOCK

    n = sizeof( s );
    MEMSET( s, 0, n );
    n -= 2;

    MPI_CHK(mpi_write_string( X, radix, s, &n));

    if( p == NULL ) p = "";

    plen = strlen( p );
    slen = strlen( s );
    s[slen++] = '\r';
    s[slen++] = '\n';

    if( fout != NULL )
    {
        if( fwrite( p, 1, plen, fout ) != plen ||
            fwrite( s, 1, slen, fout ) != slen )
            return( POLARSSL_ERR_MPI_FILE_IO_ERROR );
    }
    else
        printf( "%s%s", p, s );

cleanup:

    return( ret );
}

#endif // MPI_USE_FILE_IO
