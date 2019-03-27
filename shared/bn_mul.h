#ifndef POLARSSL_BN_MUL_H
#define POLARSSL_BN_MUL_H

#if defined(__GNUC__)
#if defined(__i386__)

#define MULADDC_INIT                \
    asm( "                          \
        movl   %%ebx, %0;           \
        movl   %5, %%esi;           \
        movl   %6, %%edi;           \
        movl   %7, %%ecx;           \
        movl   %8, %%ebx;           \
        "

#define MULADDC_CORE                \
        "                           \
        lodsl;                      \
        mull   %%ebx;               \
        addl   %%ecx,   %%eax;      \
        adcl   $0,      %%edx;      \
        addl   (%%edi), %%eax;      \
        adcl   $0,      %%edx;      \
        movl   %%edx,   %%ecx;      \
        stosl;                      \
        "

#if defined(POLARSSL_HAVE_SSE2)

#define MULADDC_HUIT                    \
        "                               \
        movd     %%ecx,     %%mm1;      \
        movd     %%ebx,     %%mm0;      \
        movd     (%%edi),   %%mm3;      \
        paddq    %%mm3,     %%mm1;      \
        movd     (%%esi),   %%mm2;      \
        pmuludq  %%mm0,     %%mm2;      \
        movd     4(%%esi),  %%mm4;      \
        pmuludq  %%mm0,     %%mm4;      \
        movd     8(%%esi),  %%mm6;      \
        pmuludq  %%mm0,     %%mm6;      \
        movd     12(%%esi), %%mm7;      \
        pmuludq  %%mm0,     %%mm7;      \
        paddq    %%mm2,     %%mm1;      \
        movd     4(%%edi),  %%mm3;      \
        paddq    %%mm4,     %%mm3;      \
        movd     8(%%edi),  %%mm5;      \
        paddq    %%mm6,     %%mm5;      \
        movd     12(%%edi), %%mm4;      \
        paddq    %%mm4,     %%mm7;      \
        movd     %%mm1,     (%%edi);    \
        movd     16(%%esi), %%mm2;      \
        pmuludq  %%mm0,     %%mm2;      \
        psrlq    $32,       %%mm1;      \
        movd     20(%%esi), %%mm4;      \
        pmuludq  %%mm0,     %%mm4;      \
        paddq    %%mm3,     %%mm1;      \
        movd     24(%%esi), %%mm6;      \
        pmuludq  %%mm0,     %%mm6;      \
        movd     %%mm1,     4(%%edi);   \
        psrlq    $32,       %%mm1;      \
        movd     28(%%esi), %%mm3;      \
        pmuludq  %%mm0,     %%mm3;      \
        paddq    %%mm5,     %%mm1;      \
        movd     16(%%edi), %%mm5;      \
        paddq    %%mm5,     %%mm2;      \
        movd     %%mm1,     8(%%edi);   \
        psrlq    $32,       %%mm1;      \
        paddq    %%mm7,     %%mm1;      \
        movd     20(%%edi), %%mm5;      \
        paddq    %%mm5,     %%mm4;      \
        movd     %%mm1,     12(%%edi);  \
        psrlq    $32,       %%mm1;      \
        paddq    %%mm2,     %%mm1;      \
        movd     24(%%edi), %%mm5;      \
        paddq    %%mm5,     %%mm6;      \
        movd     %%mm1,     16(%%edi);  \
        psrlq    $32,       %%mm1;      \
        paddq    %%mm4,     %%mm1;      \
        movd     28(%%edi), %%mm5;      \
        paddq    %%mm5,     %%mm3;      \
        movd     %%mm1,     20(%%edi);  \
        psrlq    $32,       %%mm1;      \
        paddq    %%mm6,     %%mm1;      \
        movd     %%mm1,     24(%%edi);  \
        psrlq    $32,       %%mm1;      \
        paddq    %%mm3,     %%mm1;      \
        movd     %%mm1,     28(%%edi);  \
        addl     $32,       %%edi;      \
        addl     $32,       %%esi;      \
        psrlq    $32,       %%mm1;      \
        movd     %%mm1,     %%ecx;      \
        "

#define MULADDC_STOP            \
        "                       \
        emms;                   \
        movl   %4, %%ebx;       \
        movl   %%ecx, %1;       \
        movl   %%edi, %2;       \
        movl   %%esi, %3;       \
        "                       \
        : "=m" (t), "=m" (c), "=m" (d), "=m" (s)        \
        : "m" (t), "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "eax", "ecx", "edx", "esi", "edi"             \
    );

#else

#define MULADDC_STOP            \
        "                       \
        movl   %4, %%ebx;       \
        movl   %%ecx, %1;       \
        movl   %%edi, %2;       \
        movl   %%esi, %3;       \
        "                       \
        : "=m" (t), "=m" (c), "=m" (d), "=m" (s)        \
        : "m" (t), "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "eax", "ecx", "edx", "esi", "edi"             \
    );
#endif /* SSE2 */
#endif /* i386 */
/*
#if defined(__amd64__) || defined (__x86_64__)

#define MULADDC_INIT                            \
    asm( "movq   %0, %%rsi      " :: "m" (s));  \
    asm( "movq   %0, %%rdi      " :: "m" (d));  \
    asm( "movq   %0, %%rcx      " :: "m" (c));  \
    asm( "movq   %0, %%rbx      " :: "m" (b));  \
    asm( "xorq   %r8, %r8       " );

#define MULADDC_CORE                            \
    asm( "movq  (%rsi),%rax     " );            \
    asm( "mulq   %rbx           " );            \
    asm( "addq   $8,   %rsi     " );            \
    asm( "addq   %rcx, %rax     " );            \
    asm( "movq   %r8,  %rcx     " );            \
    asm( "adcq   $0,   %rdx     " );            \
    asm( "nop                   " );            \
    asm( "addq   %rax, (%rdi)   " );            \
    asm( "adcq   %rdx, %rcx     " );            \
    asm( "addq   $8,   %rdi     " );

#define MULADDC_STOP                            \
    asm( "movq   %%rcx, %0      " : "=m" (c));  \
    asm( "movq   %%rdi, %0      " : "=m" (d));  \
    asm( "movq   %%rsi, %0      " : "=m" (s) :: \
    "rax", "rcx", "rdx", "rbx", "rsi", "rdi", "r8" );

#endif //
*/
#endif /* GNUC */

#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)

#define MULADDC_INIT                            \
    __asm   mov     esi, s                      \
    __asm   mov     edi, d                      \
    __asm   mov     ecx, c                      \
    __asm   mov     ebx, b

#define MULADDC_CORE                            \
    __asm   lodsd                               \
    __asm   mul     ebx                         \
    __asm   add     eax, ecx                    \
    __asm   adc     edx, 0                      \
    __asm   add     eax, [edi]                  \
    __asm   adc     edx, 0                      \
    __asm   mov     ecx, edx                    \
    __asm   stosd

#if defined(POLARSSL_HAVE_SSE2)

#define EMIT __asm _emit

#define MULADDC_HUIT                            \
    EMIT 0x0F  EMIT 0x6E  EMIT 0xC9             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0xC3             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x1F             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x16             \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x66  EMIT 0x04  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xE0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x76  EMIT 0x08  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x7E  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF8             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCA             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x5F  EMIT 0x04  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xDC             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x08  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xEE             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x67  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xFC             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x0F             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x56  EMIT 0x10  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD0             \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x66  EMIT 0x14  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xE0             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x76  EMIT 0x18  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF0             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x04  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x5E  EMIT 0x1C  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD8             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCD             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x10  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xD5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x08  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCF             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x14  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xE5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCA             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x18  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xF5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x10  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCC             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x1C  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xDD             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x14  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCE             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x18  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x1C  \
    EMIT 0x83  EMIT 0xC7  EMIT 0x20             \
    EMIT 0x83  EMIT 0xC6  EMIT 0x20             \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x7E  EMIT 0xC9

#define MULADDC_STOP                            \
    EMIT 0x0F  EMIT 0x77                        \
    __asm   mov     c, ecx                      \
    __asm   mov     d, edi                      \
    __asm   mov     s, esi                      \

#else

#define MULADDC_STOP                            \
    __asm   mov     c, ecx                      \
    __asm   mov     d, edi                      \
    __asm   mov     s, esi                      \

#endif /* SSE2 */
#endif /* MSVC */

#if !defined(MULADDC_CORE)
#if defined(POLARSSL_HAVE_LONGLONG)

#define MULADDC_INIT                    \
{                                       \
    t_udbl r;                           \
    uint32_t r0, r1;

#define MULADDC_CORE                    \
    r   = *(s++) * (t_udbl) b;           \
    r0  = r;                            \
    r1  = r >> biL;                     \
    r0 += c;  r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_STOP                    \
}

#else
#define MULADDC_INIT                    \
{                                       \
    uint32_t s0, s1, b0, b1;              \
    uint32_t r0, r1, rx, ry;              \
    b0 = ( b << biH ) >> biH;           \
    b1 = ( b >> biH );

#define MULADDC_CORE                    \
    s0 = ( *s << biH ) >> biH;          \
    s1 = ( *s >> biH ); s++;            \
    rx = s0 * b1; r0 = s0 * b0;         \
    ry = s1 * b0; r1 = s1 * b1;         \
    r1 += ( rx >> biH );                \
    r1 += ( ry >> biH );                \
    rx <<= biH; ry <<= biH;             \
    r0 += rx; r1 += (r0 < rx);          \
    r0 += ry; r1 += (r0 < ry);          \
    r0 +=  c; r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_STOP                    \
}

#endif /* C (generic)  */
#endif /* C (longlong) */

#endif /* bn_mul.h */
