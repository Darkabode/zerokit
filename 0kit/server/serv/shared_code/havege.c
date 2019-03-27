/* ------------------------------------------------------------------------
 * On average, one iteration accesses two 8-word blocks in the havege WALK
 * table, and generates 16 words in the RES array.
 *
 * The data read in the WALK table is updated and permuted after each use.
 * The result of the hardware clock counter read is used  for this update.
 *
 * 25 conditional tests are present.  The conditional tests are grouped in
 * two nested  groups of 12 conditional tests and 1 test that controls the
 * permutation; on average, there should be 6 tests executed and 3 of them
 * should be mispredicted.
 * ------------------------------------------------------------------------
 */

#ifndef FN_HARDCLOCK
#ifndef _WIN64
uint32_t hardclock()
{
    uint32_t tsc;
    __asm rdtsc
    __asm mov [tsc], eax
    return( tsc );
}
#endif // _WIN64

#define FN_HARDCLOCK hardclock
#endif // FN_HARDCLOCK

#define SWAP(X,Y) { int *T = X; X = Y; Y = T; }

#define TST1_ENTER if( PTEST & 1 ) { PTEST ^= 3; PTEST >>= 1;
#define TST2_ENTER if( PTEST & 1 ) { PTEST ^= 3; PTEST >>= 1;

#define TST1_LEAVE U1++; }
#define TST2_LEAVE U2++; }

#define ONE_ITERATION                                   \
                                                        \
    PTEST = PT1 >> 20;                                  \
                                                        \
    TST1_ENTER  TST1_ENTER  TST1_ENTER  TST1_ENTER      \
    TST1_ENTER  TST1_ENTER  TST1_ENTER  TST1_ENTER      \
    TST1_ENTER  TST1_ENTER  TST1_ENTER  TST1_ENTER      \
                                                        \
    TST1_LEAVE  TST1_LEAVE  TST1_LEAVE  TST1_LEAVE      \
    TST1_LEAVE  TST1_LEAVE  TST1_LEAVE  TST1_LEAVE      \
    TST1_LEAVE  TST1_LEAVE  TST1_LEAVE  TST1_LEAVE      \
                                                        \
    PTX = (PT1 >> 18) & 7;                              \
    PT1 &= 0x1FFF;                                      \
    PT2 &= 0x1FFF;                                      \
    CLK = (int)FN_HARDCLOCK();                            \
                                                        \
    i = 0;                                              \
    A = &WALK[PT1    ]; RES[i++] ^= *A;                 \
    B = &WALK[PT2    ]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 1]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 4]; RES[i++] ^= *D;                 \
                                                        \
    C_IN = (*A >> (1)) ^ (*A << (31)) ^ CLK;            \
    *A = (*B >> (2)) ^ (*B << (30)) ^ CLK;              \
    *B = C_IN ^ U1;                                     \
    *C = (*C >> (3)) ^ (*C << (29)) ^ CLK;              \
    *D = (*D >> (4)) ^ (*D << (28)) ^ CLK;              \
                                                        \
    A = &WALK[PT1 ^ 2]; RES[i++] ^= *A;                 \
    B = &WALK[PT2 ^ 2]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 3]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 6]; RES[i++] ^= *D;                 \
                                                        \
    if( PTEST & 1 ) SWAP( A, C );                       \
                                                        \
    C_IN = (*A >> (5)) ^ (*A << (27)) ^ CLK;            \
    *A = (*B >> (6)) ^ (*B << (26)) ^ CLK;              \
    *B = C_IN; CLK = (int)FN_HARDCLOCK();               \
    *C = (*C >> (7)) ^ (*C << (25)) ^ CLK;              \
    *D = (*D >> (8)) ^ (*D << (24)) ^ CLK;              \
                                                        \
    A = &WALK[PT1 ^ 4];                                 \
    B = &WALK[PT2 ^ 1];                                 \
                                                        \
    PTEST = PT2 >> 1;                                   \
                                                        \
    PT2 = (RES[(i - 8) ^ PTY] ^ WALK[PT2 ^ PTY ^ 7]);   \
    PT2 = ((PT2 & 0x1FFF) & (~8)) ^ ((PT1 ^ 8) & 0x8);  \
    PTY = (PT2 >> 10) & 7;                              \
                                                        \
    TST2_ENTER  TST2_ENTER  TST2_ENTER  TST2_ENTER      \
    TST2_ENTER  TST2_ENTER  TST2_ENTER  TST2_ENTER      \
    TST2_ENTER  TST2_ENTER  TST2_ENTER  TST2_ENTER      \
                                                        \
    TST2_LEAVE  TST2_LEAVE  TST2_LEAVE  TST2_LEAVE      \
    TST2_LEAVE  TST2_LEAVE  TST2_LEAVE  TST2_LEAVE      \
    TST2_LEAVE  TST2_LEAVE  TST2_LEAVE  TST2_LEAVE      \
                                                        \
    C = &WALK[PT1 ^ 5];                                 \
    D = &WALK[PT2 ^ 5];                                 \
                                                        \
    RES[i++] ^= *A;                                     \
    RES[i++] ^= *B;                                     \
    RES[i++] ^= *C;                                     \
    RES[i++] ^= *D;                                     \
                                                        \
    C_IN = (*A >> ( 9)) ^ (*A << (23)) ^ CLK;           \
    *A = (*B >> (10)) ^ (*B << (22)) ^ CLK;             \
    *B = C_IN ^ U2;                                     \
    *C = (*C >> (11)) ^ (*C << (21)) ^ CLK;             \
    *D = (*D >> (12)) ^ (*D << (20)) ^ CLK;             \
                                                        \
    A = &WALK[PT1 ^ 6]; RES[i++] ^= *A;                 \
    B = &WALK[PT2 ^ 3]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 7]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 7]; RES[i++] ^= *D;                 \
                                                        \
    C_IN = (*A >> (13)) ^ (*A << (19)) ^ CLK;           \
    *A = (*B >> (14)) ^ (*B << (18)) ^ CLK;             \
    *B = C_IN;                                          \
    *C = (*C >> (15)) ^ (*C << (17)) ^ CLK;             \
    *D = (*D >> (16)) ^ (*D << (16)) ^ CLK;             \
                                                        \
    PT1 = ( RES[(i - 8) ^ PTX] ^                        \
            WALK[PT1 ^ PTX ^ 7] ) & (~1);               \
    PT1 ^= (PT2 ^ 0x10) & 0x10;                         \
                                                        \
    for( n++, i = 0; i < 16; i++ )                      \
        pHS->pool[n % COLLECT_SIZE] ^= RES[i];

/*
 * Entropy gathering function
 */
void havege_fill(phavege_state_t pHS)
{
    int i, n = 0;
    int  U1,  U2, *A, *B, *C, *D;
    int PT1, PT2, *WALK, RES[16];
    int PTX, PTY, CLK, PTEST, C_IN;
    USE_GLOBAL_BLOCK

    WALK = pHS->WALK;
    PT1  = pHS->PT1;
    PT2  = pHS->PT2;

    PTX  = U1 = 0;
    PTY  = U2 = 0;

    MEMSET((uint8_t*)RES, 0, sizeof(RES));

    while (n < COLLECT_SIZE * 4) {
        ONE_ITERATION
        ONE_ITERATION
        ONE_ITERATION
        ONE_ITERATION
    }

    pHS->PT1 = PT1;
    pHS->PT2 = PT2;

    pHS->offset[0] = 0;
    pHS->offset[1] = COLLECT_SIZE / 2;
}

/*
 * HAVEGE initialization
 */
void havege_init(phavege_state_t pHS)
{
    USE_GLOBAL_BLOCK
    MEMSET((uint8_t*)pHS, 0, sizeof(havege_state_t));

    FN_HAVEGE_FILL(pHS);
}

/*
 * HAVEGE rand function
 */
void havege_rand(phavege_state_t pHS, uint8_t* buf, size_t len)
{
    int val;
	size_t use_len;
	uint8_t* p = buf;
    USE_GLOBAL_BLOCK

    while(len > 0) {
        use_len = len;
        if (use_len > sizeof(int))
            use_len = sizeof(int);

        if (pHS->offset[1] >= COLLECT_SIZE)
            FN_HAVEGE_FILL(pHS);

        val = pHS->pool[pHS->offset[0]++];
        val ^= pHS->pool[pHS->offset[1]++];

        MEMCPY(p, &val, use_len);
        
        len -= use_len;
        p += use_len;
    }
}
