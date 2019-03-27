#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

extern HANDLE gHeap;

#undef malloc
#define malloc(sz) HeapAlloc(gHeap, 0, sz)

#undef free
#define free(ptr) HeapFree(gHeap, 0, ptr)

/* ..scanf */
struct arg_scanf {
    void *data;
    int (*getch)(void*);
    int (*putch)(int,void*);
};

#define A_GETC(fn)	(++consumed,(fn)->getch((fn)->data))
#define A_PUTC(c,fn)	(--consumed,(fn)->putch((c),(fn)->data))

int __cdecl __v_scanf(struct arg_scanf* fn, const char *format, va_list arg_ptr)
{
    unsigned int ch;	/* format act. char */
    int n=0;

    /* arg_ptr tmps */
#ifdef WANT_FLOATING_POINT_IN_SCANF
    double *pd;
    float  *pf;
#endif
#ifdef WANT_LONGLONG_SCANF
    long long *pll;
#endif
    long   *pl;
    short  *ph;
    int    *pi;
    char    *s;

    unsigned int consumed=0;

    /* get one char */
    int tpch= A_GETC(fn);

    //while ((tpch!=-1)&&(*format))
    while (*format)
    {
        ch=*format++;
        switch (ch) {
            /* end of format string ?!? */
            case 0: return 0;

                /* skip spaces ... */
            case ' ':
            case '\f':
            case '\t':
            case '\v':
            case '\n':
            case '\r':
                while((*format)&&(isspace(*format))) ++format;
                while(isspace(tpch)) tpch=A_GETC(fn);
                break;

                /* format string ... */
            case '%':
                {
                    unsigned int _div=0;
                    int width=-1;
                    char flag_width=0;
                    char flag_discard=0;
                    char flag_half=0;
                    char flag_long=0;
                    char flag_longlong=0;

in_scan:
                    ch=*format++;
                    if(ch!='n' && tpch==-1) goto err_out;
                    switch (ch) {
                        /* end of format string ?!? */
            case 0: return 0;

                /* check for % */
            case '%':
                if ((unsigned char)tpch != ch) goto err_out;
                tpch=A_GETC(fn);
                break;

                /* FLAGS */
            case '*':
                flag_discard=1;
                goto in_scan;
            case 'h':
                flag_half=1;
                goto in_scan;
            case 'l':
                if (flag_long) flag_longlong=1;
                flag_long=1;
                goto in_scan;
            case 'q':
            case 'L':
                flag_longlong=1;
                goto in_scan;

                /* WIDTH */
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                width=strtol(format-1,&s,10);
                format=s;
                flag_width=1;
                goto in_scan;

                /* scan for integer / strtol reimplementation ... */
            case 'p':
            case 'X':
            case 'x':
                _div+=6;
            case 'd':
                _div+=2;
            case 'o':
                _div+=8;
            case 'u':
            case 'i':
                {
#ifdef WANT_LONGLONG_SCANF
                    unsigned long long v=0;
#else
                    unsigned long v=0;
#endif
                    unsigned int consumedsofar;
                    int neg=0;
                    while(isspace(tpch)) tpch=A_GETC(fn);
                    if (tpch=='-') {
                        tpch=A_GETC(fn);
                        neg=1;
                    }

                    if (tpch=='+') tpch=A_GETC(fn);

                    if (tpch==-1) return n;
                    consumedsofar=consumed;

                    if (!flag_width) {
                        if ((_div==16) && (tpch=='0')) goto scan_hex;
                        if (!_div) {
                            _div=10;
                            if (tpch=='0') {
                                _div=8;
scan_hex:
                                tpch=A_GETC(fn);
                                if ((tpch|32)=='x') {
                                    tpch=A_GETC(fn);
                                    _div=16;
                                }
                            }
                        }
                    }
                    while ((width)&&(tpch!=-1)) {
                        register unsigned long c=tpch&0xff;
#ifdef WANT_LONGLONG_SCANF
                        register unsigned long long d=c|0x20;
#else
                        register unsigned long d=c|0x20;
#endif
                        c=(d>='a'?d-'a'+10:c<='9'?c-'0':0xff);
                        if (c>=_div) break;
                        d=v*_div;
#ifdef WANT_LONGLONG_SCANF
                        v=(d<v)?ULLONG_MAX:d+c;
#else
                        v=(d<v)?ULONG_MAX:d+c;
#endif
                        --width;
                        tpch=A_GETC(fn);
                    }

                    if (consumedsofar==consumed) return n;

                    if ((ch|0x20)<'p') {
#ifdef WANT_LONGLONG_SCANF
                        register long long l=v;
                        if (v>=-((unsigned long long)LLONG_MIN)) {
                            l=(neg)?LLONG_MIN:LLONG_MAX;
                        }
                        else {
                            if (neg) v*=-1;
                        }
#else
                        register long l=v;
                        if (v>=-((unsigned long)LONG_MIN)) {
                            l=(neg)?LONG_MIN:LONG_MAX;
                        }
                        else {
                            if (neg) v*=-1;
                        }
#endif
                    }
                    if (!flag_discard) {
#ifdef WANT_LONGLONG_SCANF
                        if (flag_longlong) {
                            pll=(long long *)va_arg(arg_ptr,long long*);
                            *pll=v;
                        } else
#endif
                            if (flag_long) {
                                pl=(long *)va_arg(arg_ptr,long*);
                                *pl=v;
                            } else if (flag_half) {
                                ph=(short*)va_arg(arg_ptr,short*);
                                *ph=v;
                            } else {
                                pi=(int *)va_arg(arg_ptr,int*);
                                *pi=v;
                            }
                            if(consumedsofar<consumed) ++n;
                    }
                }
                break;

                /* FIXME: return value of *scanf with ONE float maybe -1 instead of 0 */
#ifdef WANT_FLOATING_POINT_IN_SCANF
                /* floating point numbers */
            case 'e':
            case 'E':
            case 'f':
            case 'g':
                {
                    double d=0.0;
                    int neg=0;
                    unsigned int consumedsofar;

                    while(isspace(tpch)) tpch=A_GETC(fn);

                    if (tpch=='-') {
                        tpch=A_GETC(fn);
                        neg=1;
                    }
                    if (tpch=='+') tpch=A_GETC(fn);

                    consumedsofar=consumed;

                    while (isdigit(tpch)) {
                        d=d*10+(tpch-'0');
                        tpch=A_GETC(fn);
                    }
                    if (tpch=='.') {
                        double factor=.1;
                        consumedsofar++;
                        tpch=A_GETC(fn);
                        while (isdigit(tpch)) {
                            d=d+(factor*(tpch-'0'));
                            factor/=10;
                            tpch=A_GETC(fn);
                        }
                    }
                    if (consumedsofar==consumed) return n;	/* error */
                    if ((tpch|0x20)=='e') {
                        int exp=0, prec=tpch;
                        double factor=10;
                        tpch=A_GETC(fn);
                        if (tpch=='-') {
                            factor=0.1;
                            tpch=A_GETC(fn);
                        } else if (tpch=='+') {
                            tpch=A_GETC(fn);
                        } else {
                            d=0;
                            if (tpch!=-1) A_PUTC(tpch,fn);
                            tpch=prec;
                            goto exp_out;
                        }
                        consumedsofar=consumed;
                        while (isdigit(tpch)) {
                            exp=exp*10+(tpch-'0');
                            tpch=A_GETC(fn);
                        }
                        if (consumedsofar==consumed) return n;	/* error */
                        while (exp) {	/* as in strtod: XXX: this introduces rounding errors */
                            d*=factor; --exp;
                        }
                    }
exp_out:
                    if (neg) d = -d;
                    if (!flag_discard) {
                        if (flag_long) {
                            pd=(double *)va_arg(arg_ptr,double*);
                            *pd=d;
                        } else {
                            pf=(float *)va_arg(arg_ptr,float*);
                            *pf=d;
                        }
                        ++n;
                    }
                }
                break;
#endif

                /* char-sequences */
            case 'c':
                if (!flag_discard) {
                    s=(char *)va_arg(arg_ptr,char*);
                    ++n;
                }
                if (!flag_width) width=1;
                while (width && (tpch!=-1)) {
                    if (!flag_discard) *(s++)=tpch;
                    --width;
                    tpch=A_GETC(fn);
                }
                break;

                /* string */
            case 's':
                if (!flag_discard) s=(char *)va_arg(arg_ptr,char*);
                while(isspace(tpch)) tpch=A_GETC(fn);
                if (tpch==-1) break;		/* end of scan -> error */
                while (width && (tpch!=-1) && (!isspace(tpch))) {
                    if (!flag_discard) *s=tpch;
                    if (tpch) ++s; else break;
                    --width;
                    tpch=A_GETC(fn);
                }
                if (!flag_discard) { *s=0; ++n; }
                break;

                /* consumed-count */
            case 'n':
                if (!flag_discard) {
                    pi=(int *)va_arg(arg_ptr,int *);
                    //	    ++n;	/* in accordance to ANSI C we don't count this conversion */
                    *pi=consumed-1;
                }
                break;

#ifdef WANT_CHARACTER_CLASSES_IN_SCANF
            case '[':
                {
                    char cset[256];
                    int flag_not=0;
                    int flag_dash=0;
                    int matched=0;
                    __stosb(cset,0,sizeof(cset));
                    ch=*format++;
                    /* first char specials */
                    if (ch=='^') {
                        flag_not=1;
                        ch=*format++;
                    }
                    if ((ch=='-')||(ch==']')) {
                        cset[ch]=1;
                        ch=*format++;
                    }
                    /* almost all non special chars */
                    for (;(*format) && (*format!=']');++format) {
                        if (flag_dash) {
                            register unsigned char tmp=*format;
//                             for ( ;ch <= tmp; ++ch)
//                                 cset[ch]=1;
                            flag_dash=0;
                            ch=*format;
                        }
                        else if (*format=='-') flag_dash=1;
                        else {
                            cset[ch]=1;
                            ch=*format;
                        }
                    }
                    /* last char specials */
                    if (flag_dash) cset['-']=1;
                    else cset[ch]=1;

                    /* like %c or %s */
                    if (!flag_discard)
                        s=(char *)va_arg(arg_ptr,char*);
                    while (width && (tpch>=0) && (cset[tpch]^flag_not)) {
                        if (!flag_discard) *s=tpch;
                        if (tpch) ++s; else break;
                        --width;
                        tpch=A_GETC(fn);
                        matched=1;
                    }
                    if (!flag_discard) *s=0;
                    ++format;
                    if (matched && !flag_discard)
                        ++n;
                }
                break;
#endif
            default:
                goto err_out;
                    }
                }
                break;

                /* check if equal format string... */
            default:
                if ((unsigned char)tpch != ch) goto err_out;
                tpch=A_GETC(fn);
                break;
                }
    }

    /* maybe a "%n" follows */
    if(*format) {
        while(isspace(*format)) format++;
        if(format[0] == '%' && format[1] == 'n') {
            pi = (int *) va_arg(arg_ptr, int *);
            *pi = consumed - 1;
        }
    }

err_out:
    if (tpch<0 && n==0) return EOF;
    A_PUTC(tpch,fn);
    return n;
}


struct str_data {
    unsigned char* str;
};

static int __cdecl sgetc(struct str_data* sd) {
    register unsigned int ret = *(sd->str++);
    return (ret)?(int)ret:-1;
}

static int __cdecl sputc(int c, struct str_data* sd) {
    return (*(--sd->str)==c)?c:-1;
}

int __cdecl vsscanf(const char* str, const char* format, va_list arg_ptr)
{
    struct str_data  fdat = { (unsigned char*)str };
    struct arg_scanf farg = { (void*)&fdat, (int(*)(void*))sgetc, (int(*)(int,void*))sputc };
    return __v_scanf(&farg,format,arg_ptr);
}

int __cdecl vfscanf(FILE *stream, const char *format, va_list arg_ptr)
{
    struct arg_scanf farg = { (void*)stream, (int(*)(void*))fgetc, (int(*)(int,void*))fputc/*ungetc*/ };
    return __v_scanf(&farg,format,arg_ptr);
}



int __cdecl sscanf(const char *str, const char *format, ...)
{
    int n;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    n=vsscanf(str,format,arg_ptr);
    va_end (arg_ptr);
    return n;
}

int __cdecl fscanf(FILE *stream, const char *format, ...)
{
    int n;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    n=vfscanf(stream,format,arg_ptr);
    va_end (arg_ptr);
    return n;
}

int __cdecl scanf(const char *format, ...) {
    int n;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    n=vfscanf(stdin,format,arg_ptr);
    va_end (arg_ptr);
    return n;
}

typedef struct {
    unsigned char LeadChar;
    unsigned char SecondChar;
} CharPair;

typedef struct {
    unsigned short ScanCode;
    CharPair RegChars;
    CharPair ShiftChars;
    CharPair CtrlChars;
    CharPair AltChars;
} EnhKeyVals;

typedef struct {
    CharPair RegChars;
    CharPair ShiftChars;
    CharPair CtrlChars;
    CharPair AltChars;
} NormKeyVals;

/*
 * Declaration for console handle
 */
/*extern */intptr_t _coninpfh = -2;

/*
 * This is the one character push-back buffer used by _getch(), _getche()
 * and _ungetch().
 */
static int chbuf = EOF;


void __cdecl __initconin()
{
    _coninpfh = (intptr_t)CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
}


/*
 * macro for the number of elements of in EnhancedKeys[]
 */
#define NUM_EKA_ELTS    ( sizeof( EnhancedKeys ) / sizeof( EnhKeyVals ) )

/*
 * Table of key values for normal keys. Note that the table is padded so
 * that the key scan code serves as an index into the table.
 */
const static NormKeyVals NormalKeys[] = {

        /* padding */
        { /*  0 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /*  1 */ {  27,   0 }, {  27,   0 }, {  27,   0 }, {   0,   1 } },
        { /*  2 */ {  49,   0 }, {  33,   0 }, {   0,   0 }, {   0, 120 } },
        { /*  3 */ {  50,   0 }, {  64,   0 }, {   0,   3 }, {   0, 121 } },
        { /*  4 */ {  51,   0 }, {  35,   0 }, {   0,   0 }, {   0, 122 } },
        { /*  5 */ {  52,   0 }, {  36,   0 }, {   0,   0 }, {   0, 123 } },
        { /*  6 */ {  53,   0 }, {  37,   0 }, {   0,   0 }, {   0, 124 } },
        { /*  7 */ {  54,   0 }, {  94,   0 }, {  30,   0 }, {   0, 125 } },
        { /*  8 */ {  55,   0 }, {  38,   0 }, {   0,   0 }, {   0, 126 } },
        { /*  9 */ {  56,   0 }, {  42,   0 }, {   0,   0 }, {   0, 127 } },
        { /* 10 */ {  57,   0 }, {  40,   0 }, {   0,   0 }, {   0, 128 } },
        { /* 11 */ {  48,   0 }, {  41,   0 }, {   0,   0 }, {   0, 129 } },
        { /* 12 */ {  45,   0 }, {  95,   0 }, {  31,   0 }, {   0, 130 } },
        { /* 13 */ {  61,   0 }, {  43,   0 }, {   0,   0 }, {   0, 131 } },
        { /* 14 */ {   8,   0 }, {   8,   0 }, { 127,   0 }, {   0,  14 } },
        { /* 15 */ {   9,   0 }, {   0,  15 }, {   0, 148 }, {   0,  15 } },
        { /* 16 */ { 113,   0 }, {  81,   0 }, {  17,   0 }, {   0,  16 } },
        { /* 17 */ { 119,   0 }, {  87,   0 }, {  23,   0 }, {   0,  17 } },
        { /* 18 */ { 101,   0 }, {  69,   0 }, {   5,   0 }, {   0,  18 } },
        { /* 19 */ { 114,   0 }, {  82,   0 }, {  18,   0 }, {   0,  19 } },
        { /* 20 */ { 116,   0 }, {  84,   0 }, {  20,   0 }, {   0,  20 } },
        { /* 21 */ { 121,   0 }, {  89,   0 }, {  25,   0 }, {   0,  21 } },
        { /* 22 */ { 117,   0 }, {  85,   0 }, {  21,   0 }, {   0,  22 } },
        { /* 23 */ { 105,   0 }, {  73,   0 }, {   9,   0 }, {   0,  23 } },
        { /* 24 */ { 111,   0 }, {  79,   0 }, {  15,   0 }, {   0,  24 } },
        { /* 25 */ { 112,   0 }, {  80,   0 }, {  16,   0 }, {   0,  25 } },
        { /* 26 */ {  91,   0 }, { 123,   0 }, {  27,   0 }, {   0,  26 } },
        { /* 27 */ {  93,   0 }, { 125,   0 }, {  29,   0 }, {   0,  27 } },
        { /* 28 */ {  13,   0 }, {  13,   0 }, {  10,   0 }, {   0,  28 } },

        /* padding */
        { /* 29 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 30 */ {  97,   0 }, {  65,   0 }, {   1,   0 }, {   0,  30 } },
        { /* 31 */ { 115,   0 }, {  83,   0 }, {  19,   0 }, {   0,  31 } },
        { /* 32 */ { 100,   0 }, {  68,   0 }, {   4,   0 }, {   0,  32 } },
        { /* 33 */ { 102,   0 }, {  70,   0 }, {   6,   0 }, {   0,  33 } },
        { /* 34 */ { 103,   0 }, {  71,   0 }, {   7,   0 }, {   0,  34 } },
        { /* 35 */ { 104,   0 }, {  72,   0 }, {   8,   0 }, {   0,  35 } },
        { /* 36 */ { 106,   0 }, {  74,   0 }, {  10,   0 }, {   0,  36 } },
        { /* 37 */ { 107,   0 }, {  75,   0 }, {  11,   0 }, {   0,  37 } },
        { /* 38 */ { 108,   0 }, {  76,   0 }, {  12,   0 }, {   0,  38 } },
        { /* 39 */ {  59,   0 }, {  58,   0 }, {   0,   0 }, {   0,  39 } },
        { /* 40 */ {  39,   0 }, {  34,   0 }, {   0,   0 }, {   0,  40 } },
        { /* 41 */ {  96,   0 }, { 126,   0 }, {   0,   0 }, {   0,  41 } },

        /* padding */
        { /* 42 */ {    0,  0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 43 */ {  92,   0 }, { 124,   0 }, {  28,   0 }, {   0,   0 } },
        { /* 44 */ { 122,   0 }, {  90,   0 }, {  26,   0 }, {   0,  44 } },
        { /* 45 */ { 120,   0 }, {  88,   0 }, {  24,   0 }, {   0,  45 } },
        { /* 46 */ {  99,   0 }, {  67,   0 }, {   3,   0 }, {   0,  46 } },
        { /* 47 */ { 118,   0 }, {  86,   0 }, {  22,   0 }, {   0,  47 } },
        { /* 48 */ {  98,   0 }, {  66,   0 }, {   2,   0 }, {   0,  48 } },
        { /* 49 */ { 110,   0 }, {  78,   0 }, {  14,   0 }, {   0,  49 } },
        { /* 50 */ { 109,   0 }, {  77,   0 }, {  13,   0 }, {   0,  50 } },
        { /* 51 */ {  44,   0 }, {  60,   0 }, {   0,   0 }, {   0,  51 } },
        { /* 52 */ {  46,   0 }, {  62,   0 }, {   0,   0 }, {   0,  52 } },
        { /* 53 */ {  47,   0 }, {  63,   0 }, {   0,   0 }, {   0,  53 } },

        /* padding */
        { /* 54 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 55 */ {  42,   0 }, {   0,   0 }, { 114,   0 }, {   0,   0 } },

        /* padding */
        { /* 56 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 57 */ {  32,   0 }, {  32,   0 }, {  32,   0 }, {  32,   0 } },

        /* padding */
        { /* 58 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 59 */ {   0,  59 }, {   0,  84 }, {   0,  94 }, {   0, 104 } },
        { /* 60 */ {   0,  60 }, {   0,  85 }, {   0,  95 }, {   0, 105 } },
        { /* 61 */ {   0,  61 }, {   0,  86 }, {   0,  96 }, {   0, 106 } },
        { /* 62 */ {   0,  62 }, {   0,  87 }, {   0,  97 }, {   0, 107 } },
        { /* 63 */ {   0,  63 }, {   0,  88 }, {   0,  98 }, {   0, 108 } },
        { /* 64 */ {   0,  64 }, {   0,  89 }, {   0,  99 }, {   0, 109 } },
        { /* 65 */ {   0,  65 }, {   0,  90 }, {   0, 100 }, {   0, 110 } },
        { /* 66 */ {   0,  66 }, {   0,  91 }, {   0, 101 }, {   0, 111 } },
        { /* 67 */ {   0,  67 }, {   0,  92 }, {   0, 102 }, {   0, 112 } },
        { /* 68 */ {   0,  68 }, {   0,  93 }, {   0, 103 }, {   0, 113 } },

        /* padding */
        { /* 69 */ {    0,  0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },
        { /* 70 */ {    0,  0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 71 */ {   0,  71 }, {  55,   0 }, {   0, 119 }, {   0,   0 } },
        { /* 72 */ {   0,  72 }, {  56,   0 }, {   0, 141 }, {   0,   0 } },
        { /* 73 */ {   0,  73 }, {  57,   0 }, {   0, 132 }, {   0,   0 } },
        { /* 74 */ {   0,   0 }, {  45,   0 }, {   0,   0 }, {   0,   0 } },
        { /* 75 */ {   0,  75 }, {  52,   0 }, {   0, 115 }, {   0,   0 } },
        { /* 76 */ {   0,   0 }, {  53,   0 }, {   0,   0 }, {   0,   0 } },
        { /* 77 */ {   0,  77 }, {  54,   0 }, {   0, 116 }, {   0,   0 } },
        { /* 78 */ {   0,   0 }, {  43,   0 }, {   0,   0 }, {   0,   0 } },
        { /* 79 */ {   0,  79 }, {  49,   0 }, {   0, 117 }, {   0,   0 } },
        { /* 80 */ {   0,  80 }, {  50,   0 }, {   0, 145 }, {   0,   0 } },
        { /* 81 */ {   0,  81 }, {  51,   0 }, {   0, 118 }, {   0,   0 } },
        { /* 82 */ {   0,  82 }, {  48,   0 }, {   0, 146 }, {   0,   0 } },
        { /* 83 */ {   0,  83 }, {  46,   0 }, {   0, 147 }, {   0,   0 } },

        /* padding */
        { /* 84 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },
        { /* 85 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },
        { /* 86 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

        { /* 87 */ { 224, 133 }, { 224, 135 }, { 224, 137 }, { 224, 139 } },
        { /* 88 */ { 224, 134 }, { 224, 136 }, { 224, 138 }, { 224, 140 } }

};

/*
 * Table of key values for enhanced keys
 */
const static EnhKeyVals EnhancedKeys[] = {
        { 28, {  13,   0 }, {  13,   0 }, {  10,   0 }, {   0, 166 } },
        { 53, {  47,   0 }, {  63,   0 }, {   0, 149 }, {   0, 164 } },
        { 71, { 224,  71 }, { 224,  71 }, { 224, 119 }, {   0, 151 } },
        { 72, { 224,  72 }, { 224,  72 }, { 224, 141 }, {   0, 152 } },
        { 73, { 224,  73 }, { 224,  73 }, { 224, 134 }, {   0, 153 } },
        { 75, { 224,  75 }, { 224,  75 }, { 224, 115 }, {   0, 155 } },
        { 77, { 224,  77 }, { 224,  77 }, { 224, 116 }, {   0, 157 } },
        { 79, { 224,  79 }, { 224,  79 }, { 224, 117 }, {   0, 159 } },
        { 80, { 224,  80 }, { 224,  80 }, { 224, 145 }, {   0, 160 } },
        { 81, { 224,  81 }, { 224,  81 }, { 224, 118 }, {   0, 161 } },
        { 82, { 224,  82 }, { 224,  82 }, { 224, 146 }, {   0, 162 } },
        { 83, { 224,  83 }, { 224,  83 }, { 224, 147 }, {   0, 163 } }
        };

const CharPair* __cdecl _getextendedkeycode (KEY_EVENT_RECORD *pKE)
{
    DWORD CKS;              /* hold dwControlKeyState value */
    const CharPair *pCP;    /* pointer to CharPair containing extended
                               code */
    int i;

    if ((CKS = pKE->dwControlKeyState) & ENHANCED_KEY) {
        /*
         * Find the appropriate entry in EnhancedKeys[]
         */
        for (pCP = NULL, i = 0 ; i < NUM_EKA_ELTS ; i++) {

            if ( EnhancedKeys[i].ScanCode == pKE->wVirtualScanCode ) {

                if ( CKS & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) )

                    pCP = &(EnhancedKeys[i].AltChars);

                else if ( CKS & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) )

                    pCP = &(EnhancedKeys[i].CtrlChars);

                else if ( CKS & SHIFT_PRESSED)

                    pCP = &(EnhancedKeys[i].ShiftChars);

                else

                    pCP = &(EnhancedKeys[i].RegChars);

                break;

            }
        }

    }

    else {

        /*
         * Regular key or a keyboard event which shouldn't be recognized.
         * Determine which by getting the proper field of the proper
         * entry in NormalKeys[], and examining the extended code.
         */
        if ( CKS & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) )

            pCP = &(NormalKeys[pKE->wVirtualScanCode].AltChars);

        else if ( CKS & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) )

            pCP = &(NormalKeys[pKE->wVirtualScanCode].CtrlChars);

        else if ( CKS & SHIFT_PRESSED)

            pCP = &(NormalKeys[pKE->wVirtualScanCode].ShiftChars);

        else

            pCP = &(NormalKeys[pKE->wVirtualScanCode].RegChars);

        if ( ((pCP->LeadChar != 0) && (pCP->LeadChar != 224)) ||
             (pCP->SecondChar == 0) )
            /*
             * Must be a keyboard event which should not be recognized
             * (e.g., shift key was pressed)
             */
            pCP = NULL;

    }

    return(pCP);
}

#define _ALLOCA_S_MARKER_SIZE 16
#define _ALLOCA_S_HEAP_MARKER 0xDDDD

#pragma warning(push)
#pragma warning(disable:6540)
__inline void * __cdecl _MarkAllocaS(_Out_opt_ __crt_typefix(unsigned int*) void *_Ptr, unsigned int _Marker)
{
    if (_Ptr)
    {
        *((unsigned int*)_Ptr) = _Marker;
        _Ptr = (char*)_Ptr + _ALLOCA_S_MARKER_SIZE;
    }
    return _Ptr;
}
#pragma warning(pop)


#define _malloca(size) \
    __pragma(warning(suppress: 6255)) \
    _MarkAllocaS(malloc((size) + _ALLOCA_S_MARKER_SIZE), _ALLOCA_S_HEAP_MARKER)


#define _HEAP_MAXREQ 0xFFFFFFE0

/* _calloca helper */
#define _calloca(count, size)  ((count<=0 || size<=0 || ((((size_t)_HEAP_MAXREQ) / ((size_t)count)) < ((size_t)size)))? NULL : _malloca(count * size))

_CRTNOALIAS __inline void __CRTDECL _freea(_Inout_opt_ void * _Memory)
{
    unsigned int _Marker;
    if (_Memory)
    {
        _Memory = (char*)_Memory - _ALLOCA_S_MARKER_SIZE;
        _Marker = *(unsigned int *)_Memory;
        if (_Marker == _ALLOCA_S_HEAP_MARKER)
        {
            free(_Memory);
        }
#if defined(_ASSERTE)
        else if (_Marker != _ALLOCA_S_STACK_MARKER)
        {
            _ASSERTE(("Corrupted pointer passed to _freea", 0));
        }
#endif
    }
}

int __cdecl _kbhit()
{
    DWORD NumPending;
    DWORD NumPeeked;
    int ret = FALSE;
    PINPUT_RECORD pIRBuf=NULL;

    /*
     * if a character has been pushed back, return TRUE
     */
    if (chbuf != -1) {
        return TRUE;
    }

    /*
     * _coninpfh, the handle to the console input, is created the first
     * time that either _getch() or _cgets() or _kbhit() is called.
     */

    if (_coninpfh == -2) {
        __initconin();
    }

    /*
     * Peek all pending console events
     */
    if ( (_coninpfh == -1) || !GetNumberOfConsoleInputEvents((HANDLE)_coninpfh, &NumPending) || (NumPending == 0)) {
        return FALSE;
    }

    pIRBuf=(PINPUT_RECORD)_calloca(NumPending, sizeof(INPUT_RECORD));
    if (pIRBuf == NULL) {
        return FALSE;
    }

    if (PeekConsoleInput((HANDLE)_coninpfh, pIRBuf, NumPending, &NumPeeked) && (NumPeeked != 0L) && (NumPeeked <= NumPending)) {
        /*
         * Scan all of the peeked events to determine if any is a key event
         * which should be recognized.
         */
        PINPUT_RECORD pIR;
        for (pIR = pIRBuf; NumPeeked > 0; NumPeeked--, pIR++) {
            if ((pIR->EventType == KEY_EVENT) && (pIR->Event.KeyEvent.bKeyDown) && (pIR->Event.KeyEvent.uChar.AsciiChar || _getextendedkeycode(&(pIR->Event.KeyEvent)))) {
                /*
                 * Key event corresponding to an ASCII character or an
                 * extended code. In either case, success!
                 */
                ret = TRUE;
            }
        }
    }

    _freea(pIRBuf);

    return ret;
}


int __cdecl getch()
{
    INPUT_RECORD ConInpRec;
    DWORD NumRead;
    const CharPair *pCP;
    int ch = 0;                     /* single character buffer */
    DWORD oldstate;

    /*
     * check pushback buffer (chbuf) a for character
     */
    if ( chbuf != EOF ) {
        /*
         * something there, clear buffer and return the character.
         */
        ch = (unsigned char)(chbuf & 0xFF);
        chbuf = EOF;
        return ch;
    }

    /*
     * _coninpfh, the handle to the console input, is created the first
     * time that either _getch() or _cgets() or _kbhit() is called.
     */

    if ( _coninpfh == -2 )
        __initconin();

    if (_coninpfh == -1)
        return EOF;

    /*
     * Switch to raw mode (no line input, no echo input)
     */
    GetConsoleMode( (HANDLE)_coninpfh, &oldstate );
    SetConsoleMode( (HANDLE)_coninpfh, 0L );

    for ( ; ; ) {

        /*
         * Get a console input event.
         */
        if ( !ReadConsoleInput( (HANDLE)_coninpfh,
                                &ConInpRec,
                                1L,
                                &NumRead )
             || (NumRead == 0L) )
        {
            ch = EOF;
            break;
        }

        /*
         * Look for, and decipher, key events.
         */
        if ( (ConInpRec.EventType == KEY_EVENT) &&
             ConInpRec.Event.KeyEvent.bKeyDown ) {
            /*
             * Easy case: if uChar.AsciiChar is non-zero, just stuff it
             * into ch and quit.
             */

            if ( ch = (unsigned char)ConInpRec.Event.KeyEvent.uChar.AsciiChar )
                break;

            /*
             * Hard case: either an extended code or an event which should
             * not be recognized. let _getextendedkeycode() do the work...
             */
            if ( pCP = _getextendedkeycode( &(ConInpRec.Event.KeyEvent) ) ) {
                ch = pCP->LeadChar;
                chbuf = pCP->SecondChar;
                break;
            }
        }
    }


    /*
     * Restore previous console mode.
     */
    SetConsoleMode( (HANDLE)_coninpfh, oldstate );

    return ch;
}
