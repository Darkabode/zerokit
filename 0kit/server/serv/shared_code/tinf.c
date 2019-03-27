#ifndef VAR_SLTREE
tinf_tree_t sltree; /* fixed length/symbol tree */
#define VAR_SLTREE sltree
#endif // VAR_SLTREE

#ifndef VAR_SDTREE
tinf_tree_t sdtree; /* fixed distance tree */
#define VAR_SDTREE sdtree
#endif // VAR_SDTREE

/* extra bits and base tables for length codes */
#ifndef VAR_LENGTH_BITS
uint8_t length_bits[30];
#define VAR_LENGTH_BITS length_bits
#endif // VAR_LENGTH_BITS

#ifndef VAR_LENGTH_BASE
uint16_t length_base[30];
#define VAR_LENGTH_BASE length_base
#endif // VAR_LENGTH_BASE

/* extra bits and base tables for distance codes */
#ifndef VAR_DIST_BITS
uint8_t dist_bits[30];
#define VAR_DIST_BITS dist_bits
#endif // VAR_DIST_BITS

#ifndef VAR_DIST_BASE
uint16_t dist_base[30];
#define VAR_DIST_BASE dist_base
#endif // VAR_DIST_BASE

/* special ordering of code length codes */
#ifndef VAR_CLCIDX
const uint8_t clcidx[] = {
   16, 17, 18, 0, 8, 7, 9, 6,
   10, 5, 11, 4, 12, 3, 13, 2,
   14, 1, 15
};
#define VAR_CLCIDX clcidx
#endif // VAR_CLCIDX

#ifndef FN_TINF_BUILD_BITS_BASE
#define FN_TINF_BUILD_BITS_BASE tinf_build_bits_base
#endif // FN_TINF_BUILD_BITS_BASE

#ifndef FN_TINF_BUILD_TREE
#define FN_TINF_BUILD_TREE tinf_build_tree
#endif // FN_TINF_BUILD_TREE

#ifndef FN_TINF_GETBIT
#define FN_TINF_GETBIT tinf_getbit
#endif // FN_TINF_GETBIT

#ifndef FN_TINF_READ_BITS
#define FN_TINF_READ_BITS tinf_read_bits
#endif // FN_TINF_READ_BITS

#ifndef FN_TINF_DECODE_SYMBOL
#define FN_TINF_DECODE_SYMBOL tinf_decode_symbol
#endif // FN_TINF_DECODE_SYMBOL

#ifndef FN_TINF_INFLATE_BLOCK_DATA
#define FN_TINF_INFLATE_BLOCK_DATA tinf_inflate_block_data
#endif // FN_TINF_INFLATE_BLOCK_DATA

#ifndef FN_TINF_ADLER32
#define FN_TINF_ADLER32 tinf_adler32
#endif // FN_TINF_ADLER32

#ifndef FN_TINF_INIT
#define FN_TINF_INIT tinf_init
#endif // FN_TINF_INIT

#ifndef FN_TINF_DECOMPRESS
#define FN_TINF_DECOMPRESS tinf_decompress
#endif // FN_TINF_DECOMPRESS

/* build extra bits and base tables */
void tinf_build_bits_base(uint8_t* bits, uint16_t* base, int delta, int first)
{
    int i, sum;
    USE_GLOBAL_BLOCK
    
    /* build bits table */
    MEMSET(bits, 0, delta);
    for (i = 0; i < 30 - delta; ++i) {
        bits[i + delta] = i / delta;
    }

    /* build base table */
    for (sum = first, i = 0; i < 30; ++i) {
        base[i] = (uint16_t)sum;
        sum += 1 << bits[i];
    }
}

/* given an array of code lengths, build a tree */
void tinf_build_tree(ptinf_tree_t t, const uint8_t *lengths, uint32_t num)
{
    uint16_t offs[16];
    uint32_t i, sum;

    /* clear code length count table */
    for (i = 0; i < 16; ++i) {
        t->table[i] = 0;
    }

    /* scan symbol lengths, and sum code length counts */
    for (i = 0; i < num; ++i) {
        t->table[lengths[i]]++;
    }

    t->table[0] = 0;

    /* compute offset table for distribution sort */
    for (sum = 0, i = 0; i < 16; ++i) {
        offs[i] = (uint16_t)sum;
        sum += t->table[i];
    }

    /* create code->symbol translation table (symbols sorted by code) */
    for (i = 0; i < num; ++i) {
        if (lengths[i]) {
            t->trans[offs[lengths[i]]++] = (uint16_t)i;
        }
    }
}

/* ---------------------- *
 * -- decode functions -- *
 * ---------------------- */

/* get one bit from source stream */
int tinf_getbit(ptinf_data_t d)
{
   uint32_t bit;

   /* check if tag is empty */
   if (!d->bitcount--)
   {
      /* load next tag */
      d->tag = *d->source++;
      d->bitcount = 7;
   }

   /* shift bit out of tag */
   bit = d->tag & 0x01;
   d->tag >>= 1;

   return bit;
}

/* read a num bit value from a stream and add base */
uint32_t tinf_read_bits(ptinf_data_t d, int num, int base)
{
    uint32_t val = 0;
    USE_GLOBAL_BLOCK

    /* read num bits */
    if (num) {
        uint32_t limit = 1 << (num);
        uint32_t mask;

        for (mask = 1; mask < limit; mask *= 2) {
            if (FN_TINF_GETBIT(d)) {
                val += mask;
            }
        }
    }

return val + base;
}

/* given a data stream and a tree, decode a symbol */
int tinf_decode_symbol(ptinf_data_t d, ptinf_tree_t t)
{
    int sum = 0, cur = 0, len = 0;
    USE_GLOBAL_BLOCK

    /* get more bits while code value is above sum */
    do {
        cur = 2 * cur + FN_TINF_GETBIT(d);

        ++len;

        sum += t->table[len];
        cur -= t->table[len];
    } while (cur >= 0);

    return t->trans[sum + cur];
}

/* ----------------------------- *
 * -- block inflate functions -- *
 * ----------------------------- */

/* given a stream and two trees, inflate a block of data */
int tinf_inflate_block_data(ptinf_data_t d, ptinf_tree_t lt, ptinf_tree_t dt)
{
   /* remember current output position */
   uint8_t* start = d->dest;
   USE_GLOBAL_BLOCK

   while (1) {
      int sym = FN_TINF_DECODE_SYMBOL(d, lt);

      /* check for end of block */
      if (sym == 256) {
         *d->destLen += (uint32_t)(d->dest - start);
         return ERR_OK;
      }

      if (sym < 256) {
         *d->dest++ = (uint8_t)sym;

      }
      else {
         int length, dist, offs;
         int i;

         sym -= 257;

         /* possibly get more bits from length code */
         length = FN_TINF_READ_BITS(d, VAR_LENGTH_BITS[sym], VAR_LENGTH_BASE[sym]);

         dist = FN_TINF_DECODE_SYMBOL(d, dt);

         /* possibly get more bits from distance code */
         offs = FN_TINF_READ_BITS(d, VAR_DIST_BITS[dist], VAR_DIST_BASE[dist]);

         /* copy match */
         for (i = 0; i < length; ++i) {
            d->dest[i] = d->dest[i - offs];
         }

         d->dest += length;
      }
   }
}

/* ---------------------- *
 * -- public functions -- *
 * ---------------------- */

/* initialize global data */
void tinf_init()
{
    int i;
    USE_GLOBAL_BLOCK
  
    /* build fixed huffman trees */
    /* build fixed length tree */
    for (i = 0; i < 7; ++i) VAR_SLTREE.table[i] = 0;

    VAR_SLTREE.table[7] = 24;
    VAR_SLTREE.table[8] = 152;
    VAR_SLTREE.table[9] = 112;

    for (i = 0; i < 24; ++i)
        VAR_SLTREE.trans[i] = 256 + i;
    for (i = 0; i < 144; ++i)
        VAR_SLTREE.trans[24 + i] = (uint16_t)i;
    for (i = 0; i < 8; ++i)
        VAR_SLTREE.trans[24 + 144 + i] = 280 + i;
    for (i = 0; i < 112; ++i)
        VAR_SLTREE.trans[24 + 144 + 8 + i] = 144 + i;

    /* build fixed distance tree */
    for (i = 0; i < 5; ++i) VAR_SDTREE.table[i] = 0;

    VAR_SDTREE.table[5] = 32;

    for (i = 0; i < 32; ++i)
        VAR_SDTREE.trans[i] = (uint16_t)i;

   /* build extra bits and base tables */
   FN_TINF_BUILD_BITS_BASE(VAR_LENGTH_BITS, VAR_LENGTH_BASE, 4, 3);
   FN_TINF_BUILD_BITS_BASE(VAR_DIST_BITS, VAR_DIST_BASE, 2, 1);

   /* fix a special case */
   VAR_LENGTH_BITS[28] = 0;
   VAR_LENGTH_BASE[28] = 258;
}

#define A32_BASE 65521
#define A32_NMAX 5552

uint32_t tinf_adler32(const void *data, uint32_t length)
{
    const uint8_t* buf = (const uint8_t*)data;

    uint32_t s1 = 1;
    uint32_t s2 = 0;

    while (length > 0) {
        int k = length < A32_NMAX ? length : A32_NMAX;
        int i;

        for (i = k / 16; i; --i, buf += 16) {
            s1 += buf[0];  s2 += s1; s1 += buf[1];  s2 += s1;
            s1 += buf[2];  s2 += s1; s1 += buf[3];  s2 += s1;
            s1 += buf[4];  s2 += s1; s1 += buf[5];  s2 += s1;
            s1 += buf[6];  s2 += s1; s1 += buf[7];  s2 += s1;

            s1 += buf[8];  s2 += s1; s1 += buf[9];  s2 += s1;
            s1 += buf[10]; s2 += s1; s1 += buf[11]; s2 += s1;
            s1 += buf[12]; s2 += s1; s1 += buf[13]; s2 += s1;
            s1 += buf[14]; s2 += s1; s1 += buf[15]; s2 += s1;
        }

        for (i = k % 16; i; --i) {
            s1 += *buf++;
            s2 += s1;
        }

        s1 %= A32_BASE;
        s2 %= A32_BASE;

        length -= k;
    }

    return (s2 << 16) | s1;
}

int tinf_decompress(puchar_t dest, uint32_t* pDestLen, const puchar_t source, uint32_t sourceLen, bool_t withZlibHdr)
{
    tinf_data_t d;
    int bfinal;
    uint32_t a32;
    int res = ERR_OK;
    uint8_t cmf, flg;
    USE_GLOBAL_BLOCK

    if (withZlibHdr) {
        /* -- get header bytes -- */
        cmf = source[0];
        flg = source[1];

        /* -- check format -- */
        /* check checksum */
        if ((256*cmf + flg) % 31) {
            return ERR_BAD;
        }

        /* check method is deflate */
        if ((cmf & 0x0f) != 8) {
            return ERR_BAD;
        }

        /* check window size is valid */
        if ((cmf >> 4) > 7) {
            return ERR_BAD;
        }

        /* check there is no preset dictionary */
        if (flg & 0x20) {
            return ERR_BAD;
        }

        /* -- get adler32 checksum -- */

        a32 = source[sourceLen - 4];
        a32 = 256*a32 + source[sourceLen - 3];
        a32 = 256*a32 + source[sourceLen - 2];
        a32 = 256*a32 + source[sourceLen - 1];

        d.source = source + 2;
        sourceLen -= 6;
    }
    else {
        d.source = source;
    }
    
    d.bitcount = 0;
    d.dest = dest;
    d.destLen = pDestLen;
    
    *pDestLen = 0;

    do {
        uint32_t btype;
        int res;

        /* read final block flag */
        bfinal = FN_TINF_GETBIT(&d);

        /* read block type (2 bits) */
        btype = FN_TINF_READ_BITS(&d, 2, 0);

        /* decompress block */
        if (btype == 0) {
            /* decompress uncompressed block */
            uint32_t length, invlength;
            uint32_t i;

            do {
                /* get length */
                length = d.source[1];
                length = 256*length + d.source[0];

                /* get one's complement of length */
                invlength = d.source[3];
                invlength = 256 * invlength + d.source[2];

                /* check length */
                if (length != (~invlength & 0x0000ffff)) {
                    res = ERR_BAD;
                    break;
                }

                d.source += 4;

                /* copy block */
                for (i = length; i; --i)
                    *d.dest++ = *d.source++;

                /* make sure we start next block on a byte boundary */
                d.bitcount = 0;

                *d.destLen += length;

                res = ERR_OK;
            } while (0);                    
        }
        else if (btype == 1) {
            /* decompress block with fixed huffman trees */
            res = FN_TINF_INFLATE_BLOCK_DATA(&d, &VAR_SLTREE, &VAR_SDTREE);
        }
        else if (btype == 2) {
            tinf_tree_t code_tree;
            uint8_t lengths[288+32];
            uint32_t hlit, hdist, hclen;
            uint32_t i, num, length;

            /* decode trees from stream */
            /* get 5 bits HLIT (257-286) */
            hlit = FN_TINF_READ_BITS(&d, 5, 257);

            /* get 5 bits HDIST (1-32) */
            hdist = FN_TINF_READ_BITS(&d, 5, 1);

            /* get 4 bits HCLEN (4-19) */
            hclen = FN_TINF_READ_BITS(&d, 4, 4);

            MEMSET(lengths, 0, 19);

            /* read code lengths for code length alphabet */
            for (i = 0; i < hclen; ++i) {
                /* get 3 bits code length (0-7) */
                uint32_t clen = FN_TINF_READ_BITS(&d, 3, 0);
                lengths[VAR_CLCIDX[i]] = (uint8_t)clen;
            }

            /* build code length tree */
            FN_TINF_BUILD_TREE(&code_tree, lengths, 19);

            /* decode code lengths for the dynamic trees */
            for (num = 0; num < hlit + hdist; ) {
                int sym = FN_TINF_DECODE_SYMBOL(&d, &code_tree);

                if (sym == 16) { /* copy previous code length 3-6 times (read 2 bits) */
                    uint8_t prev = lengths[num - 1];
                    length = FN_TINF_READ_BITS(&d, 2, 3);
                    MEMSET(lengths + num, prev, length);
                    num += length;
                }
                else if (sym == 17) {
                    /* repeat code length 0 for 3-10 times (read 3 bits) */
                    length = FN_TINF_READ_BITS(&d, 3, 3);
                    MEMSET(lengths + num, 0, length);
                    num += length;
                }
                else if (sym == 18) {
                    /* repeat code length 0 for 11-138 times (read 7 bits) */
                    length = FN_TINF_READ_BITS(&d, 7, 11);
                    MEMSET(lengths + num, 0, length);
                    num += length;
                }
                else {
                    /* values 0-15 represent the actual code lengths */
                    lengths[num++] = (uint8_t)sym;
                }
            }

            /* build dynamic trees */
            FN_TINF_BUILD_TREE(&d.ltree, lengths, hlit);
            FN_TINF_BUILD_TREE(&d.dtree, lengths + hlit, hdist);


            /* decode block using decoded trees */
            res = FN_TINF_INFLATE_BLOCK_DATA(&d, &d.ltree, &d.dtree);
        }
        else {
            res = ERR_BAD;
            goto exit;
        }

        if (res != ERR_OK) {
            goto exit;
        }
    } while (!bfinal);

exit:
    if (res != ERR_OK) {
        return res;
    }
    
    /* -- check adler32 checksum -- */
    if (a32 != FN_TINF_ADLER32(dest, *pDestLen)) {
        return ERR_BAD;
    }

    return ERR_OK;
}
