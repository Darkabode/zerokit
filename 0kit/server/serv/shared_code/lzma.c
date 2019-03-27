// Compressor
#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)


#if USE_LZMA_COMPRESSOR

#define kHash2Size (1 << 10)
#define kHash3Size (1 << 16)
#define kHash4Size (1 << 20)

#define kFix3HashSize (kHash2Size)
#define kFix4HashSize (kHash2Size + kHash3Size)
#define kFix5HashSize (kHash2Size + kHash3Size + kHash4Size)

#define HASH2_CALC hashValue = cur[0] | ((uint32_t)cur[1] << 8);

#define HASH3_CALC { \
    uint32_t temp = p->crc[cur[0]] ^ cur[1]; \
    hash2Value = temp & (kHash2Size - 1); \
    hashValue = (temp ^ ((uint32_t)cur[2] << 8)) & p->hashMask; }

#define HASH4_CALC { \
    uint32_t temp = p->crc[cur[0]] ^ cur[1]; \
    hash2Value = temp & (kHash2Size - 1); \
    hash3Value = (temp ^ ((uint32_t)cur[2] << 8)) & (kHash3Size - 1); \
    hashValue = (temp ^ ((uint32_t)cur[2] << 8) ^ (p->crc[cur[3]] << 5)) & p->hashMask; }

#define HASH5_CALC { \
    uint32_t temp = p->crc[cur[0]] ^ cur[1]; \
    hash2Value = temp & (kHash2Size - 1); \
    hash3Value = (temp ^ ((uint32_t)cur[2] << 8)) & (kHash3Size - 1); \
    hash4Value = (temp ^ ((uint32_t)cur[2] << 8) ^ (p->crc[cur[3]] << 5)); \
    hashValue = (hash4Value ^ (p->crc[cur[4]] << 3)) & p->hashMask; \
    hash4Value &= (kHash4Size - 1); }

/* #define HASH_ZIP_CALC hashValue = ((cur[0] | ((uint32_t)cur[1] << 8)) ^ p->crc[cur[2]]) & 0xFFFF; */
#define HASH_ZIP_CALC hashValue = ((cur[2] | ((uint32_t)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;


#define MT_HASH2_CALC \
    hash2Value = (p->crc[cur[0]] ^ cur[1]) & (kHash2Size - 1);

#define MT_HASH3_CALC { \
    uint32_t temp = p->crc[cur[0]] ^ cur[1]; \
    hash2Value = temp & (kHash2Size - 1); \
    hash3Value = (temp ^ ((uint32_t)cur[2] << 8)) & (kHash3Size - 1); }

#define MT_HASH4_CALC { \
    uint32_t temp = p->crc[cur[0]] ^ cur[1]; \
    hash2Value = temp & (kHash2Size - 1); \
    hash3Value = (temp ^ ((uint32_t)cur[2] << 8)) & (kHash3Size - 1); \
    hash4Value = (temp ^ ((uint32_t)cur[2] << 8) ^ (p->crc[cur[3]] << 5)) & (kHash4Size - 1); }

typedef uint32_t CLzRef;

typedef struct _CMatchFinder
{
    uint8_t *buffer;
    uint32_t pos;
    uint32_t posLimit;
    uint32_t streamPos;
    uint32_t lenLimit;

    uint32_t cyclicBufferPos;
    uint32_t cyclicBufferSize; /* it must be = (historySize + 1) */

    uint32_t matchMaxLen;
    CLzRef *hash;
    CLzRef *son;
    uint32_t hashMask;
    //  uint32_t cutValue;

    uint8_t *bufferBase;
    ISeqInStream *stream;
    int streamEndWasReached;

    uint32_t blockSize;
    uint32_t keepSizeBefore;
    uint32_t keepSizeAfter;

    int directInput;
    size_t directInputRem;
    int bigHash;
    uint32_t historySize;
    uint32_t fixedHashSize;
    uint32_t hashSizeSum;
    uint32_t numSons;
    int result;
    uint32_t crc[256];
} CMatchFinder;

typedef void * CLzmaEncHandle;

/*
Conditions:
  Mf_GetNumAvailableBytes_Func must be called before each Mf_GetMatchLen_Func.
  Mf_GetPointerToCurrentPos_Func's result must be used only before any other function
*/

typedef void (*Mf_Init_Func)(void *object);
typedef uint8_t (*Mf_GetIndexByte_Func)(void *object, int32_t index);
typedef uint32_t (*Mf_GetNumAvailableBytes_Func)(void *object);
typedef const uint8_t * (*Mf_GetPointerToCurrentPos_Func)(void *object);
typedef uint32_t (*Mf_GetMatches_Func)(void *object, uint32_t *distances);
typedef void (*Mf_Skip_Func)(void *object, uint32_t);

typedef struct _IMatchFinder
{
    Mf_Init_Func Init;
    Mf_GetIndexByte_Func GetIndexByte;
    Mf_GetNumAvailableBytes_Func GetNumAvailableBytes;
    Mf_GetPointerToCurrentPos_Func GetPointerToCurrentPos;
    Mf_GetMatches_Func GetMatches;
    Mf_Skip_Func Skip;
} IMatchFinder;


#define kEmptyHashValue 0
#define kMaxValForNormalize ((uint32_t)0xFFFFFFFF)
#define kNormalizeStepMin (1 << 10) /* it must be power of 2 */
#define kNormalizeMask (~(kNormalizeStepMin - 1))
#define kMaxHistorySize ((uint32_t)3 << 30)

#define kStartMaxLen 3

static void LzInWindow_Free(CMatchFinder *p)
{
    if (!p->directInput)
    {
        SYS_DEALLOCATOR(p->bufferBase);
        p->bufferBase = 0;
    }
}

/* keepSizeBefore + keepSizeAfter + keepSizeReserv must be < 4G) */

static int LzInWindow_Create(CMatchFinder *p, uint32_t keepSizeReserv)
{
    uint32_t blockSize = p->keepSizeBefore + p->keepSizeAfter + keepSizeReserv;
    if (p->directInput)
    {
        p->blockSize = blockSize;
        return 1;
    }
    if (p->bufferBase == 0 || p->blockSize != blockSize)
    {
        LzInWindow_Free(p);
        p->blockSize = blockSize;
        p->bufferBase = (uint8_t*)SYS_ALLOCATOR((size_t)blockSize);
    }
    return (p->bufferBase != 0);
}

uint8_t *MatchFinder_GetPointerToCurrentPos(CMatchFinder *p) { return p->buffer; }
uint8_t MatchFinder_GetIndexByte(CMatchFinder *p, int32_t index) { return p->buffer[index]; }

uint32_t MatchFinder_GetNumAvailableBytes(CMatchFinder *p) { return p->streamPos - p->pos; }

void MatchFinder_ReduceOffsets(CMatchFinder *p, uint32_t subValue)
{
    p->posLimit -= subValue;
    p->pos -= subValue;
    p->streamPos -= subValue;
}

static void MatchFinder_ReadBlock(CMatchFinder *p)
{
    if (p->streamEndWasReached || p->result != SZ_OK)
        return;
    if (p->directInput)
    {
        uint32_t curSize = 0xFFFFFFFF - p->streamPos;
        if (curSize > p->directInputRem)
            curSize = (uint32_t)p->directInputRem;
        p->directInputRem -= curSize;
        p->streamPos += curSize;
        if (p->directInputRem == 0)
            p->streamEndWasReached = 1;
        return;
    }
    for (;;)
    {
        uint8_t *dest = p->buffer + (p->streamPos - p->pos);
        size_t size = (p->bufferBase + p->blockSize - dest);
        if (size == 0)
            return;
        p->result = p->stream->Read(p->stream, dest, &size);
        if (p->result != SZ_OK)
            return;
        if (size == 0)
        {
            p->streamEndWasReached = 1;
            return;
        }
        p->streamPos += (uint32_t)size;
        if (p->streamPos - p->pos > p->keepSizeAfter)
            return;
    }
}

void MatchFinder_MoveBlock(CMatchFinder *p)
{
    memmove(p->bufferBase,
        p->buffer - p->keepSizeBefore,
        (size_t)(p->streamPos - p->pos + p->keepSizeBefore));
    p->buffer = p->bufferBase + p->keepSizeBefore;
}

int MatchFinder_NeedMove(CMatchFinder *p)
{
    if (p->directInput)
        return 0;
    /* if (p->streamEndWasReached) return 0; */
    return ((size_t)(p->bufferBase + p->blockSize - p->buffer) <= p->keepSizeAfter);
}

// void MatchFinder_ReadIfRequired(CMatchFinder *p)
// {
//   if (p->streamEndWasReached)
//     return;
//   if (p->keepSizeAfter >= p->streamPos - p->pos)
//     MatchFinder_ReadBlock(p);
// }

static void MatchFinder_CheckAndMoveAndRead(CMatchFinder *p)
{
    if (MatchFinder_NeedMove(p))
        MatchFinder_MoveBlock(p);
    MatchFinder_ReadBlock(p);
}

static void MatchFinder_SetDefaultSettings(CMatchFinder *p)
{
    p->bigHash = 0;
}

#define kCrcPoly 0xEDB88320

void MatchFinder_Construct(CMatchFinder *p)
{
    uint32_t i;
    p->bufferBase = 0;
    p->directInput = 0;
    p->hash = 0;
    MatchFinder_SetDefaultSettings(p);

    for (i = 0; i < 256; i++)
    {
        uint32_t r = i;
        int j;
        for (j = 0; j < 8; j++)
            r = (r >> 1) ^ (kCrcPoly & ~((r & 1) - 1));
        p->crc[i] = r;
    }
}

static void MatchFinder_FreeThisClassMemory(CMatchFinder *p)
{
    SYS_DEALLOCATOR(p->hash);
    p->hash = 0;
}

void MatchFinder_Free(CMatchFinder *p)
{
    MatchFinder_FreeThisClassMemory(p);
    LzInWindow_Free(p);
}

static CLzRef* AllocRefs(uint32_t num)
{
    size_t sizeInBytes = (size_t)num * sizeof(CLzRef);
    if (sizeInBytes / sizeof(CLzRef) != num)
        return 0;
    return (CLzRef*)SYS_ALLOCATOR(sizeInBytes);
}

int MatchFinder_Create(CMatchFinder *p, uint32_t historySize,
                       uint32_t keepAddBufferBefore, uint32_t matchMaxLen, uint32_t keepAddBufferAfter)
{
    uint32_t sizeReserv;
    if (historySize > kMaxHistorySize)
    {
        MatchFinder_Free(p);
        return 0;
    }
    sizeReserv = historySize >> 1;
    if (historySize > ((uint32_t)2 << 30))
        sizeReserv = historySize >> 2;
    sizeReserv += (keepAddBufferBefore + matchMaxLen + keepAddBufferAfter) / 2 + (1 << 19);

    p->keepSizeBefore = historySize + keepAddBufferBefore + 1;
    p->keepSizeAfter = matchMaxLen + keepAddBufferAfter;
    /* we need one additional byte, since we use MoveBlock after pos++ and before dictionary using */
    if (LzInWindow_Create(p, sizeReserv))
    {
        uint32_t newCyclicBufferSize = historySize + 1;
        uint32_t hs;
        p->matchMaxLen = matchMaxLen;
        {
            p->fixedHashSize = 0;

            hs = historySize - 1;
            hs |= (hs >> 1);
            hs |= (hs >> 2);
            hs |= (hs >> 4);
            hs |= (hs >> 8);
            hs >>= 1;
            hs |= 0xFFFF; /* don't change it! It's required for Deflate */
            if (hs > (1 << 24)) {
                hs >>= 1;
            }
            p->hashMask = hs;
            hs++;
            p->fixedHashSize += kHash2Size;
            p->fixedHashSize += kHash3Size;
            hs += p->fixedHashSize;
        }

        {
            uint32_t prevSize = p->hashSizeSum + p->numSons;
            uint32_t newSize;
            p->historySize = historySize;
            p->hashSizeSum = hs;
            p->cyclicBufferSize = newCyclicBufferSize;
            p->numSons = newCyclicBufferSize * 2;
            newSize = p->hashSizeSum + p->numSons;
            if (p->hash != 0 && prevSize == newSize)
                return 1;
            MatchFinder_FreeThisClassMemory(p);
            p->hash = AllocRefs(newSize);
            if (p->hash != 0)
            {
                p->son = p->hash + p->hashSizeSum;
                return 1;
            }
        }
    }
    MatchFinder_Free(p);
    return 0;
}

static void MatchFinder_SetLimits(CMatchFinder *p)
{
    uint32_t limit = kMaxValForNormalize - p->pos;
    uint32_t limit2 = p->cyclicBufferSize - p->cyclicBufferPos;
    if (limit2 < limit)
        limit = limit2;
    limit2 = p->streamPos - p->pos;
    if (limit2 <= p->keepSizeAfter)
    {
        if (limit2 > 0)
            limit2 = 1;
    }
    else
        limit2 -= p->keepSizeAfter;
    if (limit2 < limit)
        limit = limit2;
    {
        uint32_t lenLimit = p->streamPos - p->pos;
        if (lenLimit > p->matchMaxLen)
            lenLimit = p->matchMaxLen;
        p->lenLimit = lenLimit;
    }
    p->posLimit = p->pos + limit;
}

void MatchFinder_Init(CMatchFinder *p)
{
    uint32_t i;
    for (i = 0; i < p->hashSizeSum; i++)
        p->hash[i] = kEmptyHashValue;
    p->cyclicBufferPos = 0;
    p->buffer = p->bufferBase;
    p->pos = p->streamPos = p->cyclicBufferSize;
    p->result = SZ_OK;
    p->streamEndWasReached = 0;
    MatchFinder_ReadBlock(p);
    MatchFinder_SetLimits(p);
}

static uint32_t MatchFinder_GetSubValue(CMatchFinder *p)
{
    return (p->pos - p->historySize - 1) & kNormalizeMask;
}

void MatchFinder_Normalize3(uint32_t subValue, CLzRef *items, uint32_t numItems)
{
    uint32_t i;
    for (i = 0; i < numItems; i++)
    {
        uint32_t value = items[i];
        if (value <= subValue)
            value = kEmptyHashValue;
        else
            value -= subValue;
        items[i] = value;
    }
}

static void MatchFinder_Normalize(CMatchFinder *p)
{
    uint32_t subValue = MatchFinder_GetSubValue(p);
    MatchFinder_Normalize3(subValue, p->hash, p->hashSizeSum + p->numSons);
    MatchFinder_ReduceOffsets(p, subValue);
}

static void MatchFinder_CheckLimits(CMatchFinder *p)
{
    if (p->pos == kMaxValForNormalize)
        MatchFinder_Normalize(p);
    if (!p->streamEndWasReached && p->keepSizeAfter == p->streamPos - p->pos)
        MatchFinder_CheckAndMoveAndRead(p);
    if (p->cyclicBufferPos == p->cyclicBufferSize)
        p->cyclicBufferPos = 0;
    MatchFinder_SetLimits(p);
}

static uint32_t * Hc_GetMatchesSpec(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *cur, CLzRef *son,
                                    uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t cutValue,
                                    uint32_t *distances, uint32_t maxLen)
{
    son[_cyclicBufferPos] = curMatch;
    for (;;)
    {
        uint32_t delta = pos - curMatch;
        if (cutValue-- == 0 || delta >= _cyclicBufferSize)
            return distances;
        {
            const uint8_t *pb = cur - delta;
            curMatch = son[_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)];
            if (pb[maxLen] == cur[maxLen] && *pb == *cur)
            {
                uint32_t len = 0;
                while (++len != lenLimit)
                    if (pb[len] != cur[len])
                        break;
                if (maxLen < len)
                {
                    *distances++ = maxLen = len;
                    *distances++ = delta - 1;
                    if (len == lenLimit)
                        return distances;
                }
            }
        }
    }
}

uint32_t * GetMatchesSpec1(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *cur, CLzRef *son,
                           uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t cutValue,
                           uint32_t *distances, uint32_t maxLen)
{
    CLzRef *ptr0 = son + (_cyclicBufferPos << 1) + 1;
    CLzRef *ptr1 = son + (_cyclicBufferPos << 1);
    uint32_t len0 = 0, len1 = 0;
    for (;;)
    {
        uint32_t delta = pos - curMatch;
        if (cutValue-- == 0 || delta >= _cyclicBufferSize)
        {
            *ptr0 = *ptr1 = kEmptyHashValue;
            return distances;
        }
        {
            CLzRef *pair = son + ((_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)) << 1);
            const uint8_t *pb = cur - delta;
            uint32_t len = (len0 < len1 ? len0 : len1);
            if (pb[len] == cur[len])
            {
                if (++len != lenLimit && pb[len] == cur[len])
                    while (++len != lenLimit)
                        if (pb[len] != cur[len])
                            break;
                if (maxLen < len)
                {
                    *distances++ = maxLen = len;
                    *distances++ = delta - 1;
                    if (len == lenLimit)
                    {
                        *ptr1 = pair[0];
                        *ptr0 = pair[1];
                        return distances;
                    }
                }
            }
            if (pb[len] < cur[len])
            {
                *ptr1 = curMatch;
                ptr1 = pair + 1;
                curMatch = *ptr1;
                len1 = len;
            }
            else
            {
                *ptr0 = curMatch;
                ptr0 = pair;
                curMatch = *ptr0;
                len0 = len;
            }
        }
    }
}

static void SkipMatchesSpec(uint32_t lenLimit, uint32_t curMatch, uint32_t pos, const uint8_t *cur, CLzRef *son,
                            uint32_t _cyclicBufferPos, uint32_t _cyclicBufferSize, uint32_t cutValue)
{
    CLzRef *ptr0 = son + (_cyclicBufferPos << 1) + 1;
    CLzRef *ptr1 = son + (_cyclicBufferPos << 1);
    uint32_t len0 = 0, len1 = 0;
    for (;;)
    {
        uint32_t delta = pos - curMatch;
        if (cutValue-- == 0 || delta >= _cyclicBufferSize)
        {
            *ptr0 = *ptr1 = kEmptyHashValue;
            return;
        }
        {
            CLzRef *pair = son + ((_cyclicBufferPos - delta + ((delta > _cyclicBufferPos) ? _cyclicBufferSize : 0)) << 1);
            const uint8_t *pb = cur - delta;
            uint32_t len = (len0 < len1 ? len0 : len1);
            if (pb[len] == cur[len])
            {
                while (++len != lenLimit)
                    if (pb[len] != cur[len])
                        break;
                {
                    if (len == lenLimit)
                    {
                        *ptr1 = pair[0];
                        *ptr0 = pair[1];
                        return;
                    }
                }
            }
            if (pb[len] < cur[len])
            {
                *ptr1 = curMatch;
                ptr1 = pair + 1;
                curMatch = *ptr1;
                len1 = len;
            }
            else
            {
                *ptr0 = curMatch;
                ptr0 = pair;
                curMatch = *ptr0;
                len0 = len;
            }
        }
    }
}

#define MOVE_POS \
    ++p->cyclicBufferPos; \
    p->buffer++; \
    if (++p->pos == p->posLimit) MatchFinder_CheckLimits(p);

#define MOVE_POS_RET MOVE_POS return offset;

static void MatchFinder_MovePos(CMatchFinder *p) { MOVE_POS; }

#define GET_MATCHES_HEADER2(minLen, ret_op) \
    uint32_t lenLimit; uint32_t hashValue; const uint8_t *cur; uint32_t curMatch; \
    lenLimit = p->lenLimit; { if (lenLimit < minLen) { MatchFinder_MovePos(p); ret_op; }} \
    cur = p->buffer;

#define GET_MATCHES_HEADER(minLen) GET_MATCHES_HEADER2(minLen, return 0)
#define SKIP_HEADER(minLen)        GET_MATCHES_HEADER2(minLen, continue)

#define MF_PARAMS(p) p->pos, p->buffer, p->son, p->cyclicBufferPos, p->cyclicBufferSize, 1 << 30

#define GET_MATCHES_FOOTER(offset, maxLen) \
    offset = (uint32_t)(GetMatchesSpec1(lenLimit, curMatch, MF_PARAMS(p), \
    distances + offset, maxLen) - distances); MOVE_POS_RET;

#define SKIP_FOOTER \
    SkipMatchesSpec(lenLimit, curMatch, MF_PARAMS(p)); MOVE_POS;

uint32_t Bt3Zip_MatchFinder_GetMatches(CMatchFinder *p, uint32_t *distances)
{
    uint32_t offset;
    GET_MATCHES_HEADER(3)
        HASH_ZIP_CALC;
    curMatch = p->hash[hashValue];
    p->hash[hashValue] = p->pos;
    offset = 0;
    GET_MATCHES_FOOTER(offset, 2)
}

static uint32_t Bt4_MatchFinder_GetMatches(CMatchFinder *p, uint32_t *distances)
{
    uint32_t hash2Value, hash3Value, delta2, delta3, maxLen, offset;
    GET_MATCHES_HEADER(4)

        HASH4_CALC;

    delta2 = p->pos - p->hash[                hash2Value];
    delta3 = p->pos - p->hash[kFix3HashSize + hash3Value];
    curMatch = p->hash[kFix4HashSize + hashValue];

    p->hash[                hash2Value] =
        p->hash[kFix3HashSize + hash3Value] =
        p->hash[kFix4HashSize + hashValue] = p->pos;

    maxLen = 1;
    offset = 0;
    if (delta2 < p->cyclicBufferSize && *(cur - delta2) == *cur)
    {
        distances[0] = maxLen = 2;
        distances[1] = delta2 - 1;
        offset = 2;
    }
    if (delta2 != delta3 && delta3 < p->cyclicBufferSize && *(cur - delta3) == *cur)
    {
        maxLen = 3;
        distances[offset + 1] = delta3 - 1;
        offset += 2;
        delta2 = delta3;
    }
    if (offset != 0)
    {
        for (; maxLen != lenLimit; maxLen++)
            if (cur[(ptrdiff_t)maxLen - delta2] != cur[maxLen])
                break;
        distances[offset - 2] = maxLen;
        if (maxLen == lenLimit)
        {
            SkipMatchesSpec(lenLimit, curMatch, MF_PARAMS(p));
            MOVE_POS_RET;
        }
    }
    if (maxLen < 3)
        maxLen = 3;
    GET_MATCHES_FOOTER(offset, maxLen)
}

static uint32_t Hc4_MatchFinder_GetMatches(CMatchFinder *p, uint32_t *distances)
{
    uint32_t hash2Value, hash3Value, delta2, delta3, maxLen, offset;
    GET_MATCHES_HEADER(4)

        HASH4_CALC;

    delta2 = p->pos - p->hash[                hash2Value];
    delta3 = p->pos - p->hash[kFix3HashSize + hash3Value];
    curMatch = p->hash[kFix4HashSize + hashValue];

    p->hash[                hash2Value] =
        p->hash[kFix3HashSize + hash3Value] =
        p->hash[kFix4HashSize + hashValue] = p->pos;

    maxLen = 1;
    offset = 0;
    if (delta2 < p->cyclicBufferSize && *(cur - delta2) == *cur)
    {
        distances[0] = maxLen = 2;
        distances[1] = delta2 - 1;
        offset = 2;
    }
    if (delta2 != delta3 && delta3 < p->cyclicBufferSize && *(cur - delta3) == *cur)
    {
        maxLen = 3;
        distances[offset + 1] = delta3 - 1;
        offset += 2;
        delta2 = delta3;
    }
    if (offset != 0)
    {
        for (; maxLen != lenLimit; maxLen++)
            if (cur[(ptrdiff_t)maxLen - delta2] != cur[maxLen])
                break;
        distances[offset - 2] = maxLen;
        if (maxLen == lenLimit)
        {
            p->son[p->cyclicBufferPos] = curMatch;
            MOVE_POS_RET;
        }
    }
    if (maxLen < 3)
        maxLen = 3;
    offset = (uint32_t)(Hc_GetMatchesSpec(lenLimit, curMatch, MF_PARAMS(p),
        distances + offset, maxLen) - (distances));
    MOVE_POS_RET
}

uint32_t Hc3Zip_MatchFinder_GetMatches(CMatchFinder *p, uint32_t *distances)
{
    uint32_t offset;
    GET_MATCHES_HEADER(3)
        HASH_ZIP_CALC;
    curMatch = p->hash[hashValue];
    p->hash[hashValue] = p->pos;
    offset = (uint32_t)(Hc_GetMatchesSpec(lenLimit, curMatch, MF_PARAMS(p),
        distances, 2) - (distances));
    MOVE_POS_RET
}

void Bt3Zip_MatchFinder_Skip(CMatchFinder *p, uint32_t num)
{
    do
    {
        SKIP_HEADER(3)
            HASH_ZIP_CALC;
        curMatch = p->hash[hashValue];
        p->hash[hashValue] = p->pos;
        SKIP_FOOTER
    }
    while (--num != 0);
}

static void Bt3_MatchFinder_Skip(CMatchFinder *p, uint32_t num)
{
    do
    {
        uint32_t hash2Value;
        SKIP_HEADER(3)
            HASH3_CALC;
        curMatch = p->hash[kFix3HashSize + hashValue];
        p->hash[hash2Value] =
            p->hash[kFix3HashSize + hashValue] = p->pos;
        SKIP_FOOTER
    }
    while (--num != 0);
}

static void Bt4_MatchFinder_Skip(CMatchFinder *p, uint32_t num)
{
    do
    {
        uint32_t hash2Value, hash3Value;
        SKIP_HEADER(4)
            HASH4_CALC;
        curMatch = p->hash[kFix4HashSize + hashValue];
        p->hash[                hash2Value] =
            p->hash[kFix3HashSize + hash3Value] = p->pos;
        p->hash[kFix4HashSize + hashValue] = p->pos;
        SKIP_FOOTER
    }
    while (--num != 0);
}

static void Hc4_MatchFinder_Skip(CMatchFinder *p, uint32_t num)
{
    do
    {
        uint32_t hash2Value, hash3Value;
        SKIP_HEADER(4)
            HASH4_CALC;
        curMatch = p->hash[kFix4HashSize + hashValue];
        p->hash[                hash2Value] =
            p->hash[kFix3HashSize + hash3Value] =
            p->hash[kFix4HashSize + hashValue] = p->pos;
        p->son[p->cyclicBufferPos] = curMatch;
        MOVE_POS
    }
    while (--num != 0);
}

void MatchFinder_CreateVTable(IMatchFinder *vTable)
{
    vTable->Init = (Mf_Init_Func)MatchFinder_Init;
    vTable->GetIndexByte = (Mf_GetIndexByte_Func)MatchFinder_GetIndexByte;
    vTable->GetNumAvailableBytes = (Mf_GetNumAvailableBytes_Func)MatchFinder_GetNumAvailableBytes;
    vTable->GetPointerToCurrentPos = (Mf_GetPointerToCurrentPos_Func)MatchFinder_GetPointerToCurrentPos;
    vTable->GetMatches = (Mf_GetMatches_Func)Bt4_MatchFinder_GetMatches;
    vTable->Skip = (Mf_Skip_Func)Bt4_MatchFinder_Skip;
}




#define kBlockSizeMax ((1 << LZMA_NUM_BLOCK_SIZE_BITS) - 1)

#define kBlockSize (9 << 10)
#define kUnpackBlockSize (1 << 18)
#define kMatchArraySize (1 << 21)
#define kMatchRecordMaxSize ((LZMA_MATCH_LEN_MAX * 2 + 3) * LZMA_MATCH_LEN_MAX)

#define kNumMaxDirectBits (31)

#define kNumTopBits 24
#define kTopValue ((uint32_t)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5
#define kProbInitValue (kBitModelTotal >> 1)

#define kNumMoveReducingBits 4
#define kNumBitPriceShiftBits 4
#define kBitPrice (1 << kNumBitPriceShiftBits)

void LzmaEncProps_Init(CLzmaEncProps *p)
{
//  p->mc = 0;
  p->lc = p->lp = p->pb = p->fb = -1;
}

void LzmaEncProps_Normalize(CLzmaEncProps *p)
{
  if (p->lc < 0) p->lc = 3;
  if (p->lp < 0) p->lp = 0;
  if (p->pb < 0) p->pb = 2;
  if (p->fb < 0) p->fb = 64;
//   if (p->mc == 0) {
//       p->mc = 16 + (p->fb >> 1);
//   }
}

uint32_t LzmaEncProps_GetDictSize(const CLzmaEncProps *props2)
{
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);
  return props.dictSize;
}

#define kNumLogBits (9 + (int)sizeof(size_t) / 2)
#define kDicLogSizeMaxCompress ((kNumLogBits - 1) * 2 + 7)

void LzmaEnc_FastPosInit(uint8_t *g_FastPos)
{
  int c = 2, slotFast;
  g_FastPos[0] = 0;
  g_FastPos[1] = 1;
  
  for (slotFast = 2; slotFast < kNumLogBits * 2; slotFast++)
  {
    uint32_t k = (1 << ((slotFast >> 1) - 1));
    uint32_t j;
    for (j = 0; j < k; j++, c++)
      g_FastPos[c] = (uint8_t)slotFast;
  }
}

#define BSR2_RET(pos, res) { uint32_t i = 6 + ((kNumLogBits - 1) & \
  (0 - (((((uint32_t)1 << (kNumLogBits + 6)) - 1) - pos) >> 31))); \
  res = p->g_FastPos[pos >> i] + (i * 2); }
/*
#define BSR2_RET(pos, res) { res = (pos < (1 << (kNumLogBits + 6))) ? \
  p->g_FastPos[pos >> 6] + 12 : \
  p->g_FastPos[pos >> (6 + kNumLogBits - 1)] + (6 + (kNumLogBits - 1)) * 2; }
*/

#define GetPosSlot1(pos) p->g_FastPos[pos]
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if (pos < kNumFullDistances) res = p->g_FastPos[pos]; else BSR2_RET(pos, res); }


#define LZMA_NUM_REPS 4

typedef unsigned CState;

typedef struct
{
  uint32_t price;

  CState state;
  int prev1IsChar;
  int prev2;

  uint32_t posPrev2;
  uint32_t backPrev2;

  uint32_t posPrev;
  uint32_t backPrev;
  uint32_t backs[LZMA_NUM_REPS];
} COptimal;

#define kNumOpts (1 << 12)

#define kNumLenToPosStates 4
#define kNumPosSlotBits 6
#define kDicLogSizeMin 0
#define kDicLogSizeMax 32
#define kDistTableSizeMax (kDicLogSizeMax * 2)


#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)
#define kAlignMask (kAlignTableSize - 1)

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumPosModels (kEndPosModelIndex - kStartPosModelIndex)

#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define LZMA_PB_MAX 4
#define LZMA_LC_MAX 8
#define LZMA_LP_MAX 4

#define LZMA_NUM_PB_STATES_MAX (1 << LZMA_PB_MAX)

#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define kLenNumSymbolsTotal (kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define LZMA_MATCH_LEN_MIN 2
#define LZMA_MATCH_LEN_MAX (LZMA_MATCH_LEN_MIN + kLenNumSymbolsTotal - 1)

#define kNumStates 12

typedef struct
{
  uint32_t choice;
  uint32_t choice2;
  uint32_t low[LZMA_NUM_PB_STATES_MAX << kLenNumLowBits];
  uint32_t mid[LZMA_NUM_PB_STATES_MAX << kLenNumMidBits];
  uint32_t high[kLenNumHighSymbols];
} CLenEnc;

typedef struct
{
  CLenEnc p;
  uint32_t prices[LZMA_NUM_PB_STATES_MAX][kLenNumSymbolsTotal];
  uint32_t tableSize;
  uint32_t counters[LZMA_NUM_PB_STATES_MAX];
} CLenPriceEnc;

typedef struct
{
  uint32_t range;
  uint8_t cache;
  uint64_t low;
  uint64_t cacheSize;
  uint8_t *buf;
  uint8_t *bufLim;
  uint8_t *bufBase;
  ISeqOutStream *outStream;
  uint64_t processed;
  int res;
} CRangeEnc;

typedef struct
{
  uint32_t *litProbs;

  uint32_t isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  uint32_t isRep[kNumStates];
  uint32_t isRepG0[kNumStates];
  uint32_t isRepG1[kNumStates];
  uint32_t isRepG2[kNumStates];
  uint32_t isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

  uint32_t posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  uint32_t posEncoders[kNumFullDistances - kEndPosModelIndex];
  uint32_t posAlignEncoder[1 << kNumAlignBits];
  
  CLenPriceEnc lenEnc;
  CLenPriceEnc repLenEnc;

  uint32_t reps[LZMA_NUM_REPS];
  uint32_t state;
} CSaveState;

typedef struct
{
  IMatchFinder matchFinder;
  void *matchFinderObj;

//  bool_t mtMode;
//  CMatchFinderMt matchFinderMt;

  CMatchFinder matchFinderBase;

  #ifndef _7ZIP_ST
  uint8_t pad[128];
  #endif
  
  uint32_t optimumEndIndex;
  uint32_t optimumCurrentIndex;

  uint32_t longestMatchLength;
  uint32_t numPairs;
  uint32_t numAvail;
  COptimal opt[kNumOpts];
  
  uint8_t g_FastPos[1 << kNumLogBits];

  uint32_t ProbPrices[kBitModelTotal >> kNumMoveReducingBits];
  uint32_t matches[LZMA_MATCH_LEN_MAX * 2 + 2 + 1];
  uint32_t numFastBytes;
  uint32_t additionalOffset;
  uint32_t reps[LZMA_NUM_REPS];
  uint32_t state;

  uint32_t posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  uint32_t distancesPrices[kNumLenToPosStates][kNumFullDistances];
  uint32_t alignPrices[kAlignTableSize];
  uint32_t alignPriceCount;

  uint32_t distTableSize;

  unsigned lc, lp, pb;
  unsigned lpMask, pbMask;

  uint32_t *litProbs;

  uint32_t isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  uint32_t isRep[kNumStates];
  uint32_t isRepG0[kNumStates];
  uint32_t isRepG1[kNumStates];
  uint32_t isRepG2[kNumStates];
  uint32_t isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

  uint32_t posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  uint32_t posEncoders[kNumFullDistances - kEndPosModelIndex];
  uint32_t posAlignEncoder[1 << kNumAlignBits];
  
  CLenPriceEnc lenEnc;
  CLenPriceEnc repLenEnc;

  unsigned lclp;

  CRangeEnc rc;

  uint64_t nowPos64;
  uint32_t matchPriceCount;
  bool_t finished;

  int result;
  uint32_t dictSize;
//  uint32_t matchFinderCycles;

  int needInit;

  CSaveState saveState;
} CLzmaEnc;

void LzmaEnc_SaveState(CLzmaEncHandle pp)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  CSaveState *dest = &p->saveState;
  int i;
  dest->lenEnc = p->lenEnc;
  dest->repLenEnc = p->repLenEnc;
  dest->state = p->state;

  for (i = 0; i < kNumStates; i++)
  {
    memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
    memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
  }
  for (i = 0; i < kNumLenToPosStates; i++)
    memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));
  memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
  memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
  memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
  memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
  memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
  memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
  memcpy(dest->reps, p->reps, sizeof(p->reps));
  memcpy(dest->litProbs, p->litProbs, (0x300 << p->lclp) * sizeof(uint32_t));
}

void LzmaEnc_RestoreState(CLzmaEncHandle pp)
{
  CLzmaEnc *dest = (CLzmaEnc *)pp;
  const CSaveState *p = &dest->saveState;
  int i;
  dest->lenEnc = p->lenEnc;
  dest->repLenEnc = p->repLenEnc;
  dest->state = p->state;

  for (i = 0; i < kNumStates; i++)
  {
    memcpy(dest->isMatch[i], p->isMatch[i], sizeof(p->isMatch[i]));
    memcpy(dest->isRep0Long[i], p->isRep0Long[i], sizeof(p->isRep0Long[i]));
  }
  for (i = 0; i < kNumLenToPosStates; i++)
    memcpy(dest->posSlotEncoder[i], p->posSlotEncoder[i], sizeof(p->posSlotEncoder[i]));
  memcpy(dest->isRep, p->isRep, sizeof(p->isRep));
  memcpy(dest->isRepG0, p->isRepG0, sizeof(p->isRepG0));
  memcpy(dest->isRepG1, p->isRepG1, sizeof(p->isRepG1));
  memcpy(dest->isRepG2, p->isRepG2, sizeof(p->isRepG2));
  memcpy(dest->posEncoders, p->posEncoders, sizeof(p->posEncoders));
  memcpy(dest->posAlignEncoder, p->posAlignEncoder, sizeof(p->posAlignEncoder));
  memcpy(dest->reps, p->reps, sizeof(p->reps));
  memcpy(dest->litProbs, p->litProbs, (0x300 << dest->lclp) * sizeof(uint32_t));
}

int LzmaEnc_SetProps(CLzmaEncHandle pp, const CLzmaEncProps *props2)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);

  if (props.lc > LZMA_LC_MAX || props.lp > LZMA_LP_MAX || props.pb > LZMA_PB_MAX ||
      props.dictSize > ((uint32_t)1 << kDicLogSizeMaxCompress) || props.dictSize > ((uint32_t)1 << 30))
    return SZ_ERROR_PARAM;
  p->dictSize = props.dictSize;
//  p->matchFinderCycles = props.mc;
  {
    unsigned fb = props.fb;
    if (fb < 5)
      fb = 5;
    if (fb > LZMA_MATCH_LEN_MAX)
      fb = LZMA_MATCH_LEN_MAX;
    p->numFastBytes = fb;
  }
  p->lc = props.lc;
  p->lp = props.lp;
  p->pb = props.pb;
  
//  p->matchFinderBase.cutValue = props.mc;

  return SZ_OK;
}

static const int kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
static const int kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
static const int kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
static const int kShortRepNextStates[kNumStates]= {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

#define IsCharState(s) ((s) < 7)

#define GetLenToPosState(len) (((len) < kNumLenToPosStates + 1) ? (len) - 2 : kNumLenToPosStates - 1)

#define kInfinityPrice (1 << 30)

static void RangeEnc_Construct(CRangeEnc *p)
{
  p->outStream = 0;
  p->bufBase = 0;
}

#define RangeEnc_GetProcessed(p) ((p)->processed + ((p)->buf - (p)->bufBase) + (p)->cacheSize)

#define RC_BUF_SIZE (1 << 16)
static int RangeEnc_Alloc(CRangeEnc *p)
{
  if (p->bufBase == 0)
  {
    p->bufBase = (uint8_t*)SYS_ALLOCATOR(RC_BUF_SIZE);
    if (p->bufBase == 0)
      return 0;
    p->bufLim = p->bufBase + RC_BUF_SIZE;
  }
  return 1;
}

static void RangeEnc_Free(CRangeEnc *p)
{
  SYS_DEALLOCATOR(p->bufBase);
  p->bufBase = 0;
}

static void RangeEnc_Init(CRangeEnc *p)
{
  /* Stream.Init(); */
  p->low = 0;
  p->range = 0xFFFFFFFF;
  p->cacheSize = 1;
  p->cache = 0;

  p->buf = p->bufBase;

  p->processed = 0;
  p->res = SZ_OK;
}

static void RangeEnc_FlushStream(CRangeEnc *p)
{
  size_t num;
  if (p->res != SZ_OK)
    return;
  num = p->buf - p->bufBase;
  if (num != p->outStream->Write(p->outStream, p->bufBase, num))
    p->res = SZ_ERROR_WRITE;
  p->processed += num;
  p->buf = p->bufBase;
}

static void __fastcall RangeEnc_ShiftLow(CRangeEnc *p)
{
  if ((uint32_t)p->low < (uint32_t)0xFF000000 || (int)(p->low >> 32) != 0)
  {
    uint8_t temp = p->cache;
    do
    {
      uint8_t *buf = p->buf;
      *buf++ = (uint8_t)(temp + (uint8_t)(p->low >> 32));
      p->buf = buf;
      if (buf == p->bufLim)
        RangeEnc_FlushStream(p);
      temp = 0xFF;
    }
    while (--p->cacheSize != 0);
    p->cache = (uint8_t)((uint32_t)p->low >> 24);
  }
  p->cacheSize++;
  p->low = (uint32_t)p->low << 8;
}

static void RangeEnc_FlushData(CRangeEnc *p)
{
  int i;
  for (i = 0; i < 5; i++)
    RangeEnc_ShiftLow(p);
}

static void RangeEnc_EncodeDirectBits(CRangeEnc *p, uint32_t value, int numBits)
{
  do
  {
    p->range >>= 1;
    p->low += p->range & (0 - ((value >> --numBits) & 1));
    if (p->range < kTopValue)
    {
      p->range <<= 8;
      RangeEnc_ShiftLow(p);
    }
  }
  while (numBits != 0);
}

static void RangeEnc_EncodeBit(CRangeEnc *p, uint32_t *prob, uint32_t symbol)
{
  uint32_t ttt = *prob;
  uint32_t newBound = (p->range >> kNumBitModelTotalBits) * ttt;
  if (symbol == 0)
  {
    p->range = newBound;
    ttt += (kBitModelTotal - ttt) >> kNumMoveBits;
  }
  else
  {
    p->low += newBound;
    p->range -= newBound;
    ttt -= ttt >> kNumMoveBits;
  }
  *prob = (uint32_t)ttt;
  if (p->range < kTopValue)
  {
    p->range <<= 8;
    RangeEnc_ShiftLow(p);
  }
}

static void LitEnc_Encode(CRangeEnc *p, uint32_t *probs, uint32_t symbol)
{
  symbol |= 0x100;
  do
  {
    RangeEnc_EncodeBit(p, probs + (symbol >> 8), (symbol >> 7) & 1);
    symbol <<= 1;
  }
  while (symbol < 0x10000);
}

static void LitEnc_EncodeMatched(CRangeEnc *p, uint32_t *probs, uint32_t symbol, uint32_t matchByte)
{
  uint32_t offs = 0x100;
  symbol |= 0x100;
  do
  {
    matchByte <<= 1;
    RangeEnc_EncodeBit(p, probs + (offs + (matchByte & offs) + (symbol >> 8)), (symbol >> 7) & 1);
    symbol <<= 1;
    offs &= ~(matchByte ^ symbol);
  }
  while (symbol < 0x10000);
}

void LzmaEnc_InitPriceTables(uint32_t *ProbPrices)
{
  uint32_t i;
  for (i = (1 << kNumMoveReducingBits) / 2; i < kBitModelTotal; i += (1 << kNumMoveReducingBits))
  {
    const int kCyclesBits = kNumBitPriceShiftBits;
    uint32_t w = i;
    uint32_t bitCount = 0;
    int j;
    for (j = 0; j < kCyclesBits; j++)
    {
      w = w * w;
      bitCount <<= 1;
      while (w >= ((uint32_t)1 << 16))
      {
        w >>= 1;
        bitCount++;
      }
    }
    ProbPrices[i >> kNumMoveReducingBits] = ((kNumBitModelTotalBits << kCyclesBits) - 15 - bitCount);
  }
}


#define GET_PRICE(prob, symbol) \
  p->ProbPrices[((prob) ^ (((-(int)(symbol))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];

#define GET_PRICEa(prob, symbol) \
  ProbPrices[((prob) ^ ((-((int)(symbol))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];

#define GET_PRICE_0(prob) p->ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1(prob) p->ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

#define GET_PRICE_0a(prob) ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1a(prob) ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

static uint32_t LitEnc_GetPrice(const uint32_t *probs, uint32_t symbol, uint32_t *ProbPrices)
{
  uint32_t price = 0;
  symbol |= 0x100;
  do
  {
    price += GET_PRICEa(probs[symbol >> 8], (symbol >> 7) & 1);
    symbol <<= 1;
  }
  while (symbol < 0x10000);
  return price;
}

static uint32_t LitEnc_GetPriceMatched(const uint32_t *probs, uint32_t symbol, uint32_t matchByte, uint32_t *ProbPrices)
{
  uint32_t price = 0;
  uint32_t offs = 0x100;
  symbol |= 0x100;
  do
  {
    matchByte <<= 1;
    price += GET_PRICEa(probs[offs + (matchByte & offs) + (symbol >> 8)], (symbol >> 7) & 1);
    symbol <<= 1;
    offs &= ~(matchByte ^ symbol);
  }
  while (symbol < 0x10000);
  return price;
}


static void RcTree_Encode(CRangeEnc *rc, uint32_t *probs, int numBitLevels, uint32_t symbol)
{
  uint32_t m = 1;
  int i;
  for (i = numBitLevels; i != 0;)
  {
    uint32_t bit;
    i--;
    bit = (symbol >> i) & 1;
    RangeEnc_EncodeBit(rc, probs + m, bit);
    m = (m << 1) | bit;
  }
}

static void RcTree_ReverseEncode(CRangeEnc *rc, uint32_t *probs, int numBitLevels, uint32_t symbol)
{
  uint32_t m = 1;
  int i;
  for (i = 0; i < numBitLevels; i++)
  {
    uint32_t bit = symbol & 1;
    RangeEnc_EncodeBit(rc, probs + m, bit);
    m = (m << 1) | bit;
    symbol >>= 1;
  }
}

static uint32_t RcTree_GetPrice(const uint32_t *probs, int numBitLevels, uint32_t symbol, uint32_t *ProbPrices)
{
  uint32_t price = 0;
  symbol |= (1 << numBitLevels);
  while (symbol != 1)
  {
    price += GET_PRICEa(probs[symbol >> 1], symbol & 1);
    symbol >>= 1;
  }
  return price;
}

static uint32_t RcTree_ReverseGetPrice(const uint32_t *probs, int numBitLevels, uint32_t symbol, uint32_t *ProbPrices)
{
  uint32_t price = 0;
  uint32_t m = 1;
  int i;
  for (i = numBitLevels; i != 0; i--)
  {
    uint32_t bit = symbol & 1;
    symbol >>= 1;
    price += GET_PRICEa(probs[m], bit);
    m = (m << 1) | bit;
  }
  return price;
}


static void LenEnc_Init(CLenEnc *p)
{
  unsigned i;
  p->choice = p->choice2 = kProbInitValue;
  for (i = 0; i < (LZMA_NUM_PB_STATES_MAX << kLenNumLowBits); i++)
    p->low[i] = kProbInitValue;
  for (i = 0; i < (LZMA_NUM_PB_STATES_MAX << kLenNumMidBits); i++)
    p->mid[i] = kProbInitValue;
  for (i = 0; i < kLenNumHighSymbols; i++)
    p->high[i] = kProbInitValue;
}

static void LenEnc_Encode(CLenEnc *p, CRangeEnc *rc, uint32_t symbol, uint32_t posState)
{
  if (symbol < kLenNumLowSymbols)
  {
    RangeEnc_EncodeBit(rc, &p->choice, 0);
    RcTree_Encode(rc, p->low + (posState << kLenNumLowBits), kLenNumLowBits, symbol);
  }
  else
  {
    RangeEnc_EncodeBit(rc, &p->choice, 1);
    if (symbol < kLenNumLowSymbols + kLenNumMidSymbols)
    {
      RangeEnc_EncodeBit(rc, &p->choice2, 0);
      RcTree_Encode(rc, p->mid + (posState << kLenNumMidBits), kLenNumMidBits, symbol - kLenNumLowSymbols);
    }
    else
    {
      RangeEnc_EncodeBit(rc, &p->choice2, 1);
      RcTree_Encode(rc, p->high, kLenNumHighBits, symbol - kLenNumLowSymbols - kLenNumMidSymbols);
    }
  }
}

static void LenEnc_SetPrices(CLenEnc *p, uint32_t posState, uint32_t numSymbols, uint32_t *prices, uint32_t *ProbPrices)
{
  uint32_t a0 = GET_PRICE_0a(p->choice);
  uint32_t a1 = GET_PRICE_1a(p->choice);
  uint32_t b0 = a1 + GET_PRICE_0a(p->choice2);
  uint32_t b1 = a1 + GET_PRICE_1a(p->choice2);
  uint32_t i = 0;
  for (i = 0; i < kLenNumLowSymbols; i++)
  {
    if (i >= numSymbols)
      return;
    prices[i] = a0 + RcTree_GetPrice(p->low + (posState << kLenNumLowBits), kLenNumLowBits, i, ProbPrices);
  }
  for (; i < kLenNumLowSymbols + kLenNumMidSymbols; i++)
  {
    if (i >= numSymbols)
      return;
    prices[i] = b0 + RcTree_GetPrice(p->mid + (posState << kLenNumMidBits), kLenNumMidBits, i - kLenNumLowSymbols, ProbPrices);
  }
  for (; i < numSymbols; i++)
    prices[i] = b1 + RcTree_GetPrice(p->high, kLenNumHighBits, i - kLenNumLowSymbols - kLenNumMidSymbols, ProbPrices);
}

static void __fastcall LenPriceEnc_UpdateTable(CLenPriceEnc *p, uint32_t posState, uint32_t *ProbPrices)
{
  LenEnc_SetPrices(&p->p, posState, p->tableSize, p->prices[posState], ProbPrices);
  p->counters[posState] = p->tableSize;
}

static void LenPriceEnc_UpdateTables(CLenPriceEnc *p, uint32_t numPosStates, uint32_t *ProbPrices)
{
  uint32_t posState;
  for (posState = 0; posState < numPosStates; posState++)
    LenPriceEnc_UpdateTable(p, posState, ProbPrices);
}

static void LenEnc_Encode2(CLenPriceEnc *p, CRangeEnc *rc, uint32_t symbol, uint32_t posState, bool_t updatePrice, uint32_t *ProbPrices)
{
  LenEnc_Encode(&p->p, rc, symbol, posState);
  if (updatePrice)
    if (--p->counters[posState] == 0)
      LenPriceEnc_UpdateTable(p, posState, ProbPrices);
}




static void MovePos(CLzmaEnc *p, uint32_t num)
{
  if (num != 0)
  {
    p->additionalOffset += num;
    p->matchFinder.Skip(p->matchFinderObj, num);
  }
}

static uint32_t ReadMatchDistances(CLzmaEnc *p, uint32_t *numDistancePairsRes)
{
  uint32_t lenRes = 0, numPairs;
  p->numAvail = p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
  numPairs = p->matchFinder.GetMatches(p->matchFinderObj, p->matches);
  if (numPairs > 0)
  {
    lenRes = p->matches[numPairs - 2];
    if (lenRes == p->numFastBytes)
    {
      const uint8_t *pby = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
      uint32_t distance = p->matches[numPairs - 1] + 1;
      uint32_t numAvail = p->numAvail;
      if (numAvail > LZMA_MATCH_LEN_MAX)
        numAvail = LZMA_MATCH_LEN_MAX;
      {
        const uint8_t *pby2 = pby - distance;
        for (; lenRes < numAvail && pby[lenRes] == pby2[lenRes]; lenRes++);
      }
    }
  }
  p->additionalOffset++;
  *numDistancePairsRes = numPairs;
  return lenRes;
}


#define MakeAsChar(p) (p)->backPrev = (uint32_t)(-1); (p)->prev1IsChar = FALSE;
#define MakeAsShortRep(p) (p)->backPrev = 0; (p)->prev1IsChar = FALSE;
#define IsShortRep(p) ((p)->backPrev == 0)

static uint32_t GetRepLen1Price(CLzmaEnc *p, uint32_t state, uint32_t posState)
{
  return
    GET_PRICE_0(p->isRepG0[state]) +
    GET_PRICE_0(p->isRep0Long[state][posState]);
}

static uint32_t GetPureRepPrice(CLzmaEnc *p, uint32_t repIndex, uint32_t state, uint32_t posState)
{
  uint32_t price;
  if (repIndex == 0)
  {
    price = GET_PRICE_0(p->isRepG0[state]);
    price += GET_PRICE_1(p->isRep0Long[state][posState]);
  }
  else
  {
    price = GET_PRICE_1(p->isRepG0[state]);
    if (repIndex == 1)
      price += GET_PRICE_0(p->isRepG1[state]);
    else
    {
      price += GET_PRICE_1(p->isRepG1[state]);
      price += GET_PRICE(p->isRepG2[state], repIndex - 2);
    }
  }
  return price;
}

static uint32_t GetRepPrice(CLzmaEnc *p, uint32_t repIndex, uint32_t len, uint32_t state, uint32_t posState)
{
  return p->repLenEnc.prices[posState][len - LZMA_MATCH_LEN_MIN] +
    GetPureRepPrice(p, repIndex, state, posState);
}

static uint32_t Backward(CLzmaEnc *p, uint32_t *backRes, uint32_t cur)
{
  uint32_t posMem = p->opt[cur].posPrev;
  uint32_t backMem = p->opt[cur].backPrev;
  p->optimumEndIndex = cur;
  do
  {
    if (p->opt[cur].prev1IsChar)
    {
      MakeAsChar(&p->opt[posMem])
      p->opt[posMem].posPrev = posMem - 1;
      if (p->opt[cur].prev2)
      {
        p->opt[posMem - 1].prev1IsChar = FALSE;
        p->opt[posMem - 1].posPrev = p->opt[cur].posPrev2;
        p->opt[posMem - 1].backPrev = p->opt[cur].backPrev2;
      }
    }
    {
      uint32_t posPrev = posMem;
      uint32_t backCur = backMem;
      
      backMem = p->opt[posPrev].backPrev;
      posMem = p->opt[posPrev].posPrev;
      
      p->opt[posPrev].backPrev = backCur;
      p->opt[posPrev].posPrev = cur;
      cur = posPrev;
    }
  }
  while (cur != 0);
  *backRes = p->opt[0].backPrev;
  p->optimumCurrentIndex  = p->opt[0].posPrev;
  return p->optimumCurrentIndex;
}

#define LIT_PROBS(pos, prevByte) (p->litProbs + ((((pos) & p->lpMask) << p->lc) + ((prevByte) >> (8 - p->lc))) * 0x300)

static uint32_t GetOptimum(CLzmaEnc *p, uint32_t position, uint32_t *backRes)
{
  uint32_t numAvail, mainLen, numPairs, repMaxIndex, i, posState, lenEnd, len, cur;
  uint32_t matchPrice, repMatchPrice, normalMatchPrice;
  uint32_t reps[LZMA_NUM_REPS], repLens[LZMA_NUM_REPS];
  uint32_t *matches;
  const uint8_t *data;
  uint8_t curByte, matchByte;
  if (p->optimumEndIndex != p->optimumCurrentIndex)
  {
    const COptimal *opt = &p->opt[p->optimumCurrentIndex];
    uint32_t lenRes = opt->posPrev - p->optimumCurrentIndex;
    *backRes = opt->backPrev;
    p->optimumCurrentIndex = opt->posPrev;
    return lenRes;
  }
  p->optimumCurrentIndex = p->optimumEndIndex = 0;
  
  if (p->additionalOffset == 0)
    mainLen = ReadMatchDistances(p, &numPairs);
  else
  {
    mainLen = p->longestMatchLength;
    numPairs = p->numPairs;
  }

  numAvail = p->numAvail;
  if (numAvail < 2)
  {
    *backRes = (uint32_t)(-1);
    return 1;
  }
  if (numAvail > LZMA_MATCH_LEN_MAX)
    numAvail = LZMA_MATCH_LEN_MAX;

  data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
  repMaxIndex = 0;
  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
    uint32_t lenTest;
    const uint8_t *data2;
    reps[i] = p->reps[i];
    data2 = data - (reps[i] + 1);
    if (data[0] != data2[0] || data[1] != data2[1])
    {
      repLens[i] = 0;
      continue;
    }
    for (lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++);
    repLens[i] = lenTest;
    if (lenTest > repLens[repMaxIndex])
      repMaxIndex = i;
  }
  if (repLens[repMaxIndex] >= p->numFastBytes)
  {
    uint32_t lenRes;
    *backRes = repMaxIndex;
    lenRes = repLens[repMaxIndex];
    MovePos(p, lenRes - 1);
    return lenRes;
  }

  matches = p->matches;
  if (mainLen >= p->numFastBytes)
  {
    *backRes = matches[numPairs - 1] + LZMA_NUM_REPS;
    MovePos(p, mainLen - 1);
    return mainLen;
  }
  curByte = *data;
  matchByte = *(data - (reps[0] + 1));

  if (mainLen < 2 && curByte != matchByte && repLens[repMaxIndex] < 2)
  {
    *backRes = (uint32_t)-1;
    return 1;
  }

  p->opt[0].state = (CState)p->state;

  posState = (position & p->pbMask);

  {
    const uint32_t *probs = LIT_PROBS(position, *(data - 1));
    p->opt[1].price = GET_PRICE_0(p->isMatch[p->state][posState]) +
        (!IsCharState(p->state) ?
          LitEnc_GetPriceMatched(probs, curByte, matchByte, p->ProbPrices) :
          LitEnc_GetPrice(probs, curByte, p->ProbPrices));
  }

  MakeAsChar(&p->opt[1]);

  matchPrice = GET_PRICE_1(p->isMatch[p->state][posState]);
  repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[p->state]);

  if (matchByte == curByte)
  {
    uint32_t shortRepPrice = repMatchPrice + GetRepLen1Price(p, p->state, posState);
    if (shortRepPrice < p->opt[1].price)
    {
      p->opt[1].price = shortRepPrice;
      MakeAsShortRep(&p->opt[1]);
    }
  }
  lenEnd = ((mainLen >= repLens[repMaxIndex]) ? mainLen : repLens[repMaxIndex]);

  if (lenEnd < 2)
  {
    *backRes = p->opt[1].backPrev;
    return 1;
  }

  p->opt[1].posPrev = 0;
  for (i = 0; i < LZMA_NUM_REPS; i++)
    p->opt[0].backs[i] = reps[i];

  len = lenEnd;
  do
    p->opt[len--].price = kInfinityPrice;
  while (len >= 2);

  for (i = 0; i < LZMA_NUM_REPS; i++)
  {
    uint32_t repLen = repLens[i];
    uint32_t price;
    if (repLen < 2)
      continue;
    price = repMatchPrice + GetPureRepPrice(p, i, p->state, posState);
    do
    {
      uint32_t curAndLenPrice = price + p->repLenEnc.prices[posState][repLen - 2];
      COptimal *opt = &p->opt[repLen];
      if (curAndLenPrice < opt->price)
      {
        opt->price = curAndLenPrice;
        opt->posPrev = 0;
        opt->backPrev = i;
        opt->prev1IsChar = FALSE;
      }
    }
    while (--repLen >= 2);
  }

  normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[p->state]);

  len = ((repLens[0] >= 2) ? repLens[0] + 1 : 2);
  if (len <= mainLen)
  {
    uint32_t offs = 0;
    while (len > matches[offs])
      offs += 2;
    for (; ; len++)
    {
      COptimal *opt;
      uint32_t distance = matches[offs + 1];

      uint32_t curAndLenPrice = normalMatchPrice + p->lenEnc.prices[posState][len - LZMA_MATCH_LEN_MIN];
      uint32_t lenToPosState = GetLenToPosState(len);
      if (distance < kNumFullDistances)
        curAndLenPrice += p->distancesPrices[lenToPosState][distance];
      else
      {
        uint32_t slot;
        GetPosSlot2(distance, slot);
        curAndLenPrice += p->alignPrices[distance & kAlignMask] + p->posSlotPrices[lenToPosState][slot];
      }
      opt = &p->opt[len];
      if (curAndLenPrice < opt->price)
      {
        opt->price = curAndLenPrice;
        opt->posPrev = 0;
        opt->backPrev = distance + LZMA_NUM_REPS;
        opt->prev1IsChar = FALSE;
      }
      if (len == matches[offs])
      {
        offs += 2;
        if (offs == numPairs)
          break;
      }
    }
  }

  cur = 0;

  for (;;)
  {
    uint32_t numAvailFull, newLen, numPairs, posPrev, state, posState, startLen;
    uint32_t curPrice, curAnd1Price, matchPrice, repMatchPrice;
    bool_t nextIsChar;
    uint8_t curByte, matchByte;
    const uint8_t *data;
    COptimal *curOpt;
    COptimal *nextOpt;

    cur++;
    if (cur == lenEnd)
      return Backward(p, backRes, cur);

    newLen = ReadMatchDistances(p, &numPairs);
    if (newLen >= p->numFastBytes)
    {
      p->numPairs = numPairs;
      p->longestMatchLength = newLen;
      return Backward(p, backRes, cur);
    }
    position++;
    curOpt = &p->opt[cur];
    posPrev = curOpt->posPrev;
    if (curOpt->prev1IsChar)
    {
      posPrev--;
      if (curOpt->prev2)
      {
        state = p->opt[curOpt->posPrev2].state;
        if (curOpt->backPrev2 < LZMA_NUM_REPS)
          state = kRepNextStates[state];
        else
          state = kMatchNextStates[state];
      }
      else
        state = p->opt[posPrev].state;
      state = kLiteralNextStates[state];
    }
    else
      state = p->opt[posPrev].state;
    if (posPrev == cur - 1)
    {
      if (IsShortRep(curOpt))
        state = kShortRepNextStates[state];
      else
        state = kLiteralNextStates[state];
    }
    else
    {
      uint32_t pos;
      const COptimal *prevOpt;
      if (curOpt->prev1IsChar && curOpt->prev2)
      {
        posPrev = curOpt->posPrev2;
        pos = curOpt->backPrev2;
        state = kRepNextStates[state];
      }
      else
      {
        pos = curOpt->backPrev;
        if (pos < LZMA_NUM_REPS)
          state = kRepNextStates[state];
        else
          state = kMatchNextStates[state];
      }
      prevOpt = &p->opt[posPrev];
      if (pos < LZMA_NUM_REPS)
      {
        uint32_t i;
        reps[0] = prevOpt->backs[pos];
        for (i = 1; i <= pos; i++)
          reps[i] = prevOpt->backs[i - 1];
        for (; i < LZMA_NUM_REPS; i++)
          reps[i] = prevOpt->backs[i];
      }
      else
      {
        uint32_t i;
        reps[0] = (pos - LZMA_NUM_REPS);
        for (i = 1; i < LZMA_NUM_REPS; i++)
          reps[i] = prevOpt->backs[i - 1];
      }
    }
    curOpt->state = (CState)state;

    curOpt->backs[0] = reps[0];
    curOpt->backs[1] = reps[1];
    curOpt->backs[2] = reps[2];
    curOpt->backs[3] = reps[3];

    curPrice = curOpt->price;
    nextIsChar = FALSE;
    data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - 1;
    curByte = *data;
    matchByte = *(data - (reps[0] + 1));

    posState = (position & p->pbMask);

    curAnd1Price = curPrice + GET_PRICE_0(p->isMatch[state][posState]);
    {
      const uint32_t *probs = LIT_PROBS(position, *(data - 1));
      curAnd1Price +=
        (!IsCharState(state) ?
          LitEnc_GetPriceMatched(probs, curByte, matchByte, p->ProbPrices) :
          LitEnc_GetPrice(probs, curByte, p->ProbPrices));
    }

    nextOpt = &p->opt[cur + 1];

    if (curAnd1Price < nextOpt->price)
    {
      nextOpt->price = curAnd1Price;
      nextOpt->posPrev = cur;
      MakeAsChar(nextOpt);
      nextIsChar = TRUE;
    }

    matchPrice = curPrice + GET_PRICE_1(p->isMatch[state][posState]);
    repMatchPrice = matchPrice + GET_PRICE_1(p->isRep[state]);
    
    if (matchByte == curByte && !(nextOpt->posPrev < cur && nextOpt->backPrev == 0))
    {
      uint32_t shortRepPrice = repMatchPrice + GetRepLen1Price(p, state, posState);
      if (shortRepPrice <= nextOpt->price)
      {
        nextOpt->price = shortRepPrice;
        nextOpt->posPrev = cur;
        MakeAsShortRep(nextOpt);
        nextIsChar = TRUE;
      }
    }
    numAvailFull = p->numAvail;
    {
      uint32_t temp = kNumOpts - 1 - cur;
      if (temp < numAvailFull)
        numAvailFull = temp;
    }

    if (numAvailFull < 2)
      continue;
    numAvail = (numAvailFull <= p->numFastBytes ? numAvailFull : p->numFastBytes);

    if (!nextIsChar && matchByte != curByte) /* speed optimization */
    {
      /* try Literal + rep0 */
      uint32_t temp;
      uint32_t lenTest2;
      const uint8_t *data2 = data - (reps[0] + 1);
      uint32_t limit = p->numFastBytes + 1;
      if (limit > numAvailFull)
        limit = numAvailFull;

      for (temp = 1; temp < limit && data[temp] == data2[temp]; temp++);
      lenTest2 = temp - 1;
      if (lenTest2 >= 2)
      {
        uint32_t state2 = kLiteralNextStates[state];
        uint32_t posStateNext = (position + 1) & p->pbMask;
        uint32_t nextRepMatchPrice = curAnd1Price +
            GET_PRICE_1(p->isMatch[state2][posStateNext]) +
            GET_PRICE_1(p->isRep[state2]);
        /* for (; lenTest2 >= 2; lenTest2--) */
        {
          uint32_t curAndLenPrice;
          COptimal *opt;
          uint32_t offset = cur + 1 + lenTest2;
          while (lenEnd < offset)
            p->opt[++lenEnd].price = kInfinityPrice;
          curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
          opt = &p->opt[offset];
          if (curAndLenPrice < opt->price)
          {
            opt->price = curAndLenPrice;
            opt->posPrev = cur + 1;
            opt->backPrev = 0;
            opt->prev1IsChar = TRUE;
            opt->prev2 = FALSE;
          }
        }
      }
    }
    
    startLen = 2; /* speed optimization */
    {
    uint32_t repIndex;
    for (repIndex = 0; repIndex < LZMA_NUM_REPS; repIndex++)
    {
      uint32_t lenTest;
      uint32_t lenTestTemp;
      uint32_t price;
      const uint8_t *data2 = data - (reps[repIndex] + 1);
      if (data[0] != data2[0] || data[1] != data2[1])
        continue;
      for (lenTest = 2; lenTest < numAvail && data[lenTest] == data2[lenTest]; lenTest++);
      while (lenEnd < cur + lenTest)
        p->opt[++lenEnd].price = kInfinityPrice;
      lenTestTemp = lenTest;
      price = repMatchPrice + GetPureRepPrice(p, repIndex, state, posState);
      do
      {
        uint32_t curAndLenPrice = price + p->repLenEnc.prices[posState][lenTest - 2];
        COptimal *opt = &p->opt[cur + lenTest];
        if (curAndLenPrice < opt->price)
        {
          opt->price = curAndLenPrice;
          opt->posPrev = cur;
          opt->backPrev = repIndex;
          opt->prev1IsChar = FALSE;
        }
      }
      while (--lenTest >= 2);
      lenTest = lenTestTemp;
      
      if (repIndex == 0)
        startLen = lenTest + 1;
        
      /* if (_maxMode) */
        {
          uint32_t lenTest2 = lenTest + 1;
          uint32_t limit = lenTest2 + p->numFastBytes;
          uint32_t nextRepMatchPrice;
          if (limit > numAvailFull)
            limit = numAvailFull;
          for (; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++);
          lenTest2 -= lenTest + 1;
          if (lenTest2 >= 2)
          {
            uint32_t state2 = kRepNextStates[state];
            uint32_t posStateNext = (position + lenTest) & p->pbMask;
            uint32_t curAndLenCharPrice =
                price + p->repLenEnc.prices[posState][lenTest - 2] +
                GET_PRICE_0(p->isMatch[state2][posStateNext]) +
                LitEnc_GetPriceMatched(LIT_PROBS(position + lenTest, data[lenTest - 1]),
                    data[lenTest], data2[lenTest], p->ProbPrices);
            state2 = kLiteralNextStates[state2];
            posStateNext = (position + lenTest + 1) & p->pbMask;
            nextRepMatchPrice = curAndLenCharPrice +
                GET_PRICE_1(p->isMatch[state2][posStateNext]) +
                GET_PRICE_1(p->isRep[state2]);
            
            /* for (; lenTest2 >= 2; lenTest2--) */
            {
              uint32_t curAndLenPrice;
              COptimal *opt;
              uint32_t offset = cur + lenTest + 1 + lenTest2;
              while (lenEnd < offset)
                p->opt[++lenEnd].price = kInfinityPrice;
              curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
              opt = &p->opt[offset];
              if (curAndLenPrice < opt->price)
              {
                opt->price = curAndLenPrice;
                opt->posPrev = cur + lenTest + 1;
                opt->backPrev = 0;
                opt->prev1IsChar = TRUE;
                opt->prev2 = TRUE;
                opt->posPrev2 = cur;
                opt->backPrev2 = repIndex;
              }
            }
          }
        }
    }
    }
    /* for (uint32_t lenTest = 2; lenTest <= newLen; lenTest++) */
    if (newLen > numAvail)
    {
      newLen = numAvail;
      for (numPairs = 0; newLen > matches[numPairs]; numPairs += 2);
      matches[numPairs] = newLen;
      numPairs += 2;
    }
    if (newLen >= startLen)
    {
      uint32_t normalMatchPrice = matchPrice + GET_PRICE_0(p->isRep[state]);
      uint32_t offs, curBack, posSlot;
      uint32_t lenTest;
      while (lenEnd < cur + newLen)
        p->opt[++lenEnd].price = kInfinityPrice;

      offs = 0;
      while (startLen > matches[offs])
        offs += 2;
      curBack = matches[offs + 1];
      GetPosSlot2(curBack, posSlot);
      for (lenTest = /*2*/ startLen; ; lenTest++)
      {
        uint32_t curAndLenPrice = normalMatchPrice + p->lenEnc.prices[posState][lenTest - LZMA_MATCH_LEN_MIN];
        uint32_t lenToPosState = GetLenToPosState(lenTest);
        COptimal *opt;
        if (curBack < kNumFullDistances)
          curAndLenPrice += p->distancesPrices[lenToPosState][curBack];
        else
          curAndLenPrice += p->posSlotPrices[lenToPosState][posSlot] + p->alignPrices[curBack & kAlignMask];
        
        opt = &p->opt[cur + lenTest];
        if (curAndLenPrice < opt->price)
        {
          opt->price = curAndLenPrice;
          opt->posPrev = cur;
          opt->backPrev = curBack + LZMA_NUM_REPS;
          opt->prev1IsChar = FALSE;
        }

        if (/*_maxMode && */lenTest == matches[offs])
        {
          /* Try Match + Literal + Rep0 */
          const uint8_t *data2 = data - (curBack + 1);
          uint32_t lenTest2 = lenTest + 1;
          uint32_t limit = lenTest2 + p->numFastBytes;
          uint32_t nextRepMatchPrice;
          if (limit > numAvailFull)
            limit = numAvailFull;
          for (; lenTest2 < limit && data[lenTest2] == data2[lenTest2]; lenTest2++);
          lenTest2 -= lenTest + 1;
          if (lenTest2 >= 2)
          {
            uint32_t state2 = kMatchNextStates[state];
            uint32_t posStateNext = (position + lenTest) & p->pbMask;
            uint32_t curAndLenCharPrice = curAndLenPrice +
                GET_PRICE_0(p->isMatch[state2][posStateNext]) +
                LitEnc_GetPriceMatched(LIT_PROBS(position + lenTest, data[lenTest - 1]),
                    data[lenTest], data2[lenTest], p->ProbPrices);
            state2 = kLiteralNextStates[state2];
            posStateNext = (posStateNext + 1) & p->pbMask;
            nextRepMatchPrice = curAndLenCharPrice +
                GET_PRICE_1(p->isMatch[state2][posStateNext]) +
                GET_PRICE_1(p->isRep[state2]);
            
            /* for (; lenTest2 >= 2; lenTest2--) */
            {
              uint32_t offset = cur + lenTest + 1 + lenTest2;
              uint32_t curAndLenPrice;
              COptimal *opt;
              while (lenEnd < offset)
                p->opt[++lenEnd].price = kInfinityPrice;
              curAndLenPrice = nextRepMatchPrice + GetRepPrice(p, 0, lenTest2, state2, posStateNext);
              opt = &p->opt[offset];
              if (curAndLenPrice < opt->price)
              {
                opt->price = curAndLenPrice;
                opt->posPrev = cur + lenTest + 1;
                opt->backPrev = 0;
                opt->prev1IsChar = TRUE;
                opt->prev2 = TRUE;
                opt->posPrev2 = cur;
                opt->backPrev2 = curBack + LZMA_NUM_REPS;
              }
            }
          }
          offs += 2;
          if (offs == numPairs)
            break;
          curBack = matches[offs + 1];
          if (curBack >= kNumFullDistances)
            GetPosSlot2(curBack, posSlot);
        }
      }
    }
  }
}

#define ChangePair(smallDist, bigDist) (((bigDist) >> 7) > (smallDist))

static void WriteEndMarker(CLzmaEnc *p, uint32_t posState)
{
  uint32_t len;
  RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 1);
  RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
  p->state = kMatchNextStates[p->state];
  len = LZMA_MATCH_LEN_MIN;
  LenEnc_Encode2(&p->lenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, 1, p->ProbPrices);
  RcTree_Encode(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], kNumPosSlotBits, (1 << kNumPosSlotBits) - 1);
  RangeEnc_EncodeDirectBits(&p->rc, (((uint32_t)1 << 30) - 1) >> kNumAlignBits, 30 - kNumAlignBits);
  RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, kAlignMask);
}

static int CheckErrors(CLzmaEnc *p)
{
  if (p->result != SZ_OK)
    return p->result;
  if (p->rc.res != SZ_OK)
    p->result = SZ_ERROR_WRITE;
  if (p->matchFinderBase.result != SZ_OK)
    p->result = SZ_ERROR_READ;
  if (p->result != SZ_OK)
    p->finished = TRUE;
  return p->result;
}

static int Flush(CLzmaEnc *p, uint32_t nowPos)
{
  /* ReleaseMFStream(); */
  p->finished = TRUE;
  WriteEndMarker(p, nowPos & p->pbMask);
  RangeEnc_FlushData(&p->rc);
  RangeEnc_FlushStream(&p->rc);
  return CheckErrors(p);
}

static void FillAlignPrices(CLzmaEnc *p)
{
  uint32_t i;
  for (i = 0; i < kAlignTableSize; i++)
    p->alignPrices[i] = RcTree_ReverseGetPrice(p->posAlignEncoder, kNumAlignBits, i, p->ProbPrices);
  p->alignPriceCount = 0;
}

static void FillDistancesPrices(CLzmaEnc *p)
{
  uint32_t tempPrices[kNumFullDistances];
  uint32_t i, lenToPosState;
  for (i = kStartPosModelIndex; i < kNumFullDistances; i++)
  {
    uint32_t posSlot = GetPosSlot1(i);
    uint32_t footerBits = ((posSlot >> 1) - 1);
    uint32_t base = ((2 | (posSlot & 1)) << footerBits);
    tempPrices[i] = RcTree_ReverseGetPrice(p->posEncoders + base - posSlot - 1, footerBits, i - base, p->ProbPrices);
  }

  for (lenToPosState = 0; lenToPosState < kNumLenToPosStates; lenToPosState++)
  {
    uint32_t posSlot;
    const uint32_t *encoder = p->posSlotEncoder[lenToPosState];
    uint32_t *posSlotPrices = p->posSlotPrices[lenToPosState];
    for (posSlot = 0; posSlot < p->distTableSize; posSlot++)
      posSlotPrices[posSlot] = RcTree_GetPrice(encoder, kNumPosSlotBits, posSlot, p->ProbPrices);
    for (posSlot = kEndPosModelIndex; posSlot < p->distTableSize; posSlot++)
      posSlotPrices[posSlot] += ((((posSlot >> 1) - 1) - kNumAlignBits) << kNumBitPriceShiftBits);

    {
      uint32_t *distancesPrices = p->distancesPrices[lenToPosState];
      uint32_t i;
      for (i = 0; i < kStartPosModelIndex; i++)
        distancesPrices[i] = posSlotPrices[i];
      for (; i < kNumFullDistances; i++)
        distancesPrices[i] = posSlotPrices[GetPosSlot1(i)] + tempPrices[i];
    }
  }
  p->matchPriceCount = 0;
}

void LzmaEnc_Construct(CLzmaEnc *p)
{
  RangeEnc_Construct(&p->rc);
  MatchFinder_Construct(&p->matchFinderBase);
//   MatchFinderMt_Construct(&p->matchFinderMt);
//   p->matchFinderMt.MatchFinder = &p->matchFinderBase;

  {
    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    LzmaEnc_SetProps(p, &props);
  }

  LzmaEnc_FastPosInit(p->g_FastPos);

  LzmaEnc_InitPriceTables(p->ProbPrices);
  p->litProbs = 0;
  p->saveState.litProbs = 0;
}

CLzmaEncHandle LzmaEnc_Create()
{
  void *p;
  p = SYS_ALLOCATOR(sizeof(CLzmaEnc));
  if (p != 0)
    LzmaEnc_Construct((CLzmaEnc *)p);
  return p;
}

void LzmaEnc_FreeLits(CLzmaEnc *p)
{
  SYS_DEALLOCATOR(p->litProbs);
  SYS_DEALLOCATOR(p->saveState.litProbs);
  p->litProbs = 0;
  p->saveState.litProbs = 0;
}

void LzmaEnc_Destruct(CLzmaEnc *p)
{
  MatchFinder_Free(&p->matchFinderBase);
  LzmaEnc_FreeLits(p);
  RangeEnc_Free(&p->rc);
}

void LzmaEnc_Destroy(CLzmaEncHandle p)
{
  LzmaEnc_Destruct((CLzmaEnc *)p);
  SYS_DEALLOCATOR(p);
}

static int LzmaEnc_CodeOneBlock(CLzmaEnc *p, bool_t useLimits, uint32_t maxPackSize, uint32_t maxUnpackSize)
{
  uint32_t nowPos32, startPos32;
  if (p->needInit)
  {
    p->matchFinder.Init(p->matchFinderObj);
    p->needInit = 0;
  }

  if (p->finished)
    return p->result;
  RINOK(CheckErrors(p));

  nowPos32 = (uint32_t)p->nowPos64;
  startPos32 = nowPos32;

  if (p->nowPos64 == 0)
  {
    uint32_t numPairs;
    uint8_t curByte;
    if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
      return Flush(p, nowPos32);
    ReadMatchDistances(p, &numPairs);
    RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][0], 0);
    p->state = kLiteralNextStates[p->state];
    curByte = p->matchFinder.GetIndexByte(p->matchFinderObj, 0 - p->additionalOffset);
    LitEnc_Encode(&p->rc, p->litProbs, curByte);
    p->additionalOffset--;
    nowPos32++;
  }

  if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) != 0)
  for (;;)
  {
    uint32_t pos, len, posState;

    len = GetOptimum(p, nowPos32, &pos);

    posState = nowPos32 & p->pbMask;
    if (len == 1 && pos == (uint32_t)-1)
    {
      uint8_t curByte;
      uint32_t *probs;
      const uint8_t *data;

      RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 0);
      data = p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
      curByte = *data;
      probs = LIT_PROBS(nowPos32, *(data - 1));
      if (IsCharState(p->state))
        LitEnc_Encode(&p->rc, probs, curByte);
      else
        LitEnc_EncodeMatched(&p->rc, probs, curByte, *(data - p->reps[0] - 1));
      p->state = kLiteralNextStates[p->state];
    }
    else
    {
      RangeEnc_EncodeBit(&p->rc, &p->isMatch[p->state][posState], 1);
      if (pos < LZMA_NUM_REPS)
      {
        RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 1);
        if (pos == 0)
        {
          RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 0);
          RangeEnc_EncodeBit(&p->rc, &p->isRep0Long[p->state][posState], ((len == 1) ? 0 : 1));
        }
        else
        {
          uint32_t distance = p->reps[pos];
          RangeEnc_EncodeBit(&p->rc, &p->isRepG0[p->state], 1);
          if (pos == 1)
            RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 0);
          else
          {
            RangeEnc_EncodeBit(&p->rc, &p->isRepG1[p->state], 1);
            RangeEnc_EncodeBit(&p->rc, &p->isRepG2[p->state], pos - 2);
            if (pos == 3)
              p->reps[3] = p->reps[2];
            p->reps[2] = p->reps[1];
          }
          p->reps[1] = p->reps[0];
          p->reps[0] = distance;
        }
        if (len == 1)
          p->state = kShortRepNextStates[p->state];
        else
        {
          LenEnc_Encode2(&p->repLenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, 1, p->ProbPrices);
          p->state = kRepNextStates[p->state];
        }
      }
      else
      {
        uint32_t posSlot;
        RangeEnc_EncodeBit(&p->rc, &p->isRep[p->state], 0);
        p->state = kMatchNextStates[p->state];
        LenEnc_Encode2(&p->lenEnc, &p->rc, len - LZMA_MATCH_LEN_MIN, posState, 1, p->ProbPrices);
        pos -= LZMA_NUM_REPS;
        GetPosSlot(pos, posSlot);
        RcTree_Encode(&p->rc, p->posSlotEncoder[GetLenToPosState(len)], kNumPosSlotBits, posSlot);
        
        if (posSlot >= kStartPosModelIndex)
        {
          uint32_t footerBits = ((posSlot >> 1) - 1);
          uint32_t base = ((2 | (posSlot & 1)) << footerBits);
          uint32_t posReduced = pos - base;

          if (posSlot < kEndPosModelIndex)
            RcTree_ReverseEncode(&p->rc, p->posEncoders + base - posSlot - 1, footerBits, posReduced);
          else
          {
            RangeEnc_EncodeDirectBits(&p->rc, posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
            RcTree_ReverseEncode(&p->rc, p->posAlignEncoder, kNumAlignBits, posReduced & kAlignMask);
            p->alignPriceCount++;
          }
        }
        p->reps[3] = p->reps[2];
        p->reps[2] = p->reps[1];
        p->reps[1] = p->reps[0];
        p->reps[0] = pos;
        p->matchPriceCount++;
      }
    }
    p->additionalOffset -= len;
    nowPos32 += len;
    if (p->additionalOffset == 0)
    {
      uint32_t processed;
      if (p->matchPriceCount >= (1 << 7)) {
        FillDistancesPrices(p);
      }
      if (p->alignPriceCount >= kAlignTableSize) {
        FillAlignPrices(p);
      }
      if (p->matchFinder.GetNumAvailableBytes(p->matchFinderObj) == 0)
        break;
      processed = nowPos32 - startPos32;
      if (useLimits)
      {
        if (processed + kNumOpts + 300 >= maxUnpackSize ||
            RangeEnc_GetProcessed(&p->rc) + kNumOpts * 2 >= maxPackSize)
          break;
      }
      else if (processed >= (1 << 15))
      {
        p->nowPos64 += nowPos32 - startPos32;
        return CheckErrors(p);
      }
    }
  }
  p->nowPos64 += nowPos32 - startPos32;
  return Flush(p, nowPos32);
}

#define kBigHashDicLimit ((uint32_t)1 << 24)

static int LzmaEnc_Alloc(CLzmaEnc *p, uint32_t keepWindowSize)
{
  uint32_t beforeSize = kNumOpts;
  if (!RangeEnc_Alloc(&p->rc))
    return SZ_ERROR_MEM;

  {
    unsigned lclp = p->lc + p->lp;
    if (p->litProbs == 0 || p->saveState.litProbs == 0 || p->lclp != lclp)
    {
      LzmaEnc_FreeLits(p);
      p->litProbs = (uint32_t *)SYS_ALLOCATOR((0x300 << lclp) * sizeof(uint32_t));
      p->saveState.litProbs = (uint32_t *)SYS_ALLOCATOR((0x300 << lclp) * sizeof(uint32_t));
      if (p->litProbs == 0 || p->saveState.litProbs == 0)
      {
        LzmaEnc_FreeLits(p);
        return SZ_ERROR_MEM;
      }
      p->lclp = lclp;
    }
  }

  p->matchFinderBase.bigHash = (p->dictSize > kBigHashDicLimit);

  if (beforeSize + p->dictSize < keepWindowSize)
    beforeSize = keepWindowSize - p->dictSize;

    if (!MatchFinder_Create(&p->matchFinderBase, p->dictSize, beforeSize, p->numFastBytes, LZMA_MATCH_LEN_MAX))
      return SZ_ERROR_MEM;
    p->matchFinderObj = &p->matchFinderBase;
    MatchFinder_CreateVTable(&p->matchFinder);
  return SZ_OK;
}

void LzmaEnc_Init(CLzmaEnc *p)
{
  uint32_t i;
  p->state = 0;
  for (i = 0 ; i < LZMA_NUM_REPS; i++)
    p->reps[i] = 0;

  RangeEnc_Init(&p->rc);


  for (i = 0; i < kNumStates; i++)
  {
    uint32_t j;
    for (j = 0; j < LZMA_NUM_PB_STATES_MAX; j++)
    {
      p->isMatch[i][j] = kProbInitValue;
      p->isRep0Long[i][j] = kProbInitValue;
    }
    p->isRep[i] = kProbInitValue;
    p->isRepG0[i] = kProbInitValue;
    p->isRepG1[i] = kProbInitValue;
    p->isRepG2[i] = kProbInitValue;
  }

  {
    uint32_t num = 0x300 << (p->lp + p->lc);
    for (i = 0; i < num; i++)
      p->litProbs[i] = kProbInitValue;
  }

  {
    for (i = 0; i < kNumLenToPosStates; i++)
    {
      uint32_t *probs = p->posSlotEncoder[i];
      uint32_t j;
      for (j = 0; j < (1 << kNumPosSlotBits); j++)
        probs[j] = kProbInitValue;
    }
  }
  {
    for (i = 0; i < kNumFullDistances - kEndPosModelIndex; i++)
      p->posEncoders[i] = kProbInitValue;
  }

  LenEnc_Init(&p->lenEnc.p);
  LenEnc_Init(&p->repLenEnc.p);

  for (i = 0; i < (1 << kNumAlignBits); i++)
    p->posAlignEncoder[i] = kProbInitValue;

  p->optimumEndIndex = 0;
  p->optimumCurrentIndex = 0;
  p->additionalOffset = 0;

  p->pbMask = (1 << p->pb) - 1;
  p->lpMask = (1 << p->lp) - 1;
}

void LzmaEnc_InitPrices(CLzmaEnc *p)
{
  FillDistancesPrices(p);
  FillAlignPrices(p);

  p->lenEnc.tableSize =
  p->repLenEnc.tableSize =
      p->numFastBytes + 1 - LZMA_MATCH_LEN_MIN;
  LenPriceEnc_UpdateTables(&p->lenEnc, 1 << p->pb, p->ProbPrices);
  LenPriceEnc_UpdateTables(&p->repLenEnc, 1 << p->pb, p->ProbPrices);
}

static int LzmaEnc_AllocAndInit(CLzmaEnc *p, uint32_t keepWindowSize)
{
  uint32_t i;
  for (i = 0; i < (uint32_t)kDicLogSizeMaxCompress; i++)
    if (p->dictSize <= ((uint32_t)1 << i))
      break;
  p->distTableSize = i * 2;

  p->finished = FALSE;
  p->result = SZ_OK;
  RINOK(LzmaEnc_Alloc(p, keepWindowSize));
  LzmaEnc_Init(p);
  LzmaEnc_InitPrices(p);
  p->nowPos64 = 0;
  return SZ_OK;
}

static int LzmaEnc_Prepare(CLzmaEncHandle pp, ISeqOutStream *outStream, ISeqInStream *inStream)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  p->matchFinderBase.stream = inStream;
  p->needInit = 1;
  p->rc.outStream = outStream;
  return LzmaEnc_AllocAndInit(p, 0);
}

static void LzmaEnc_SetInputBuf(CLzmaEnc *p, const uint8_t *src, size_t srcLen)
{
  p->matchFinderBase.directInput = 1;
  p->matchFinderBase.bufferBase = (uint8_t *)src;
  p->matchFinderBase.directInputRem = srcLen;
}

int LzmaEnc_MemPrepare(CLzmaEncHandle pp, const uint8_t *src, size_t srcLen, uint32_t keepWindowSize)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  LzmaEnc_SetInputBuf(p, src, srcLen);
  p->needInit = 1;

  return LzmaEnc_AllocAndInit(p, keepWindowSize);
}

typedef struct
{
  ISeqOutStream funcTable;
  uint8_t *data;
  size_t rem;
  bool_t overflow;
} CSeqOutStreamBuf;

static size_t MyWrite(void *pp, const void *data, size_t size)
{
  CSeqOutStreamBuf *p = (CSeqOutStreamBuf *)pp;
  if (p->rem < size)
  {
    size = p->rem;
    p->overflow = TRUE;
  }
  memcpy(p->data, data, size);
  p->rem -= size;
  p->data += size;
  return size;
}

int LzmaEnc_Encode2(CLzmaEnc *p)
{
  int res = SZ_OK;

  #ifndef _7ZIP_ST
  uint8_t allocaDummy[0x300];
  int i = 0;
  for (i = 0; i < 16; i++)
    allocaDummy[i] = (uint8_t)i;
  #endif

  for (;;)
  {
    res = LzmaEnc_CodeOneBlock(p, FALSE, 0, 0);
    if (res != SZ_OK || p->finished != 0)
      break;
  }
  return res;
}

int LzmaEnc_Encode(CLzmaEncHandle pp, ISeqOutStream *outStream, ISeqInStream *inStream)
{
  RINOK(LzmaEnc_Prepare(pp, outStream, inStream));
  return LzmaEnc_Encode2((CLzmaEnc *)pp);
}

int LzmaEnc_WriteProperties(CLzmaEncHandle pp, uint8_t *props, size_t *size)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  int i;
  uint32_t dictSize = p->dictSize;
  if (*size < LZMA_PROPS_SIZE)
    return SZ_ERROR_PARAM;
  *size = LZMA_PROPS_SIZE;
  props[0] = (uint8_t)((p->pb * 5 + p->lp) * 9 + p->lc);

  for (i = 11; i <= 30; i++)
  {
    if (dictSize <= ((uint32_t)2 << i))
    {
      dictSize = (2 << i);
      break;
    }
    if (dictSize <= ((uint32_t)3 << i))
    {
      dictSize = (3 << i);
      break;
    }
  }

  for (i = 0; i < 4; i++)
    props[1 + i] = (uint8_t)(dictSize >> (8 * i));
  return SZ_OK;
}

int LzmaEnc_MemEncode(CLzmaEncHandle pp, uint8_t *dest, size_t *destLen, const uint8_t *src, size_t srcLen)
{
  int res;
  CLzmaEnc *p = (CLzmaEnc *)pp;

  CSeqOutStreamBuf outStream;

  LzmaEnc_SetInputBuf(p, src, srcLen);

  outStream.funcTable.Write = MyWrite;
  outStream.data = dest;
  outStream.rem = *destLen;
  outStream.overflow = FALSE;

  p->rc.outStream = &outStream.funcTable;
  res = LzmaEnc_MemPrepare(pp, src, srcLen, 0);
  if (res == SZ_OK)
    res = LzmaEnc_Encode2(p);

  *destLen -= outStream.rem;
  if (outStream.overflow)
    return SZ_ERROR_OUTPUT_EOF;
  return res;
}

int LzmaEncode(uint8_t *dest, size_t *destLen, const uint8_t *src, size_t srcLen, const CLzmaEncProps *props, uint8_t *propsEncoded, size_t *propsSize)
{
  CLzmaEnc *p = (CLzmaEnc *)LzmaEnc_Create();
  int res;
  if (p == 0)
    return SZ_ERROR_MEM;

  res = LzmaEnc_SetProps(p, props);
  if (res == SZ_OK)
  {
    res = LzmaEnc_WriteProperties(p, propsEncoded, propsSize);
    if (res == SZ_OK)
      res = LzmaEnc_MemEncode(p, dest, destLen, src, srcLen);
  }

  LzmaEnc_Destroy(p);
  return res;
}

#endif // USE_LZMA_COMPRESSOR

// Decompressor

#if USE_LZMA_DECOMPRESSOR

#define kNumTopBits 24
#define kTopValue ((uint32_t)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_INIT_SIZE 5

#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (uint32_t)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (uint32_t)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) \
{ UPDATE_0(p); i = (i + i); A0; } else \
{ UPDATE_1(p); i = (i + i) + 1; A1; }
#define GET_BIT(p, i) GET_BIT2(p, i, ; , ;)

#define TREE_GET_BIT(probs, i) { GET_BIT((probs + i), i); }
#define TREE_DECODE(probs, limit, i) \
{ i = 1; do { TREE_GET_BIT(probs, i); } while (i < limit); i -= limit; }

#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)

#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

/*#define kLenNumLowSymbols (1 << 3)*/
/*#define kLenNumMidSymbols (1 << 3)*/
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenLow 2
#define LenMid (LenLow + (kNumPosStatesMax << 3))
#define LenHigh (LenMid + (kNumPosStatesMax << 3))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)


#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

#if Literal != LZMA_BASE_SIZE
StopCompilingDueBUG
#endif

#ifdef WIN32

#pragma intrinsic(_byteswap_ulong)

#else

static uint32_t bswap32(uint32_t x)
{
#if defined(__LP64__)
    __asm__("bswapl %0" : "=r" (x) : "0" (x));
#else
    __asm__("bswap %0" : "=r" (x) : "0" (x));
#endif
    return x;
}

#endif // WIN32

int lzma_decode(uint8_t* outBuffer, uint32_t* pOutSize, const uint8_t* inBuffer, uint32_t inSize, ELzmaStatus* status)
{
    const uint8_t* realSrc = inBuffer + 5;
    int res = SZ_OK;
    uint32_t outSize = *pOutSize;
    uint32_t lc, lp, pb;
    uint8_t d = *inBuffer;
    uint32_t* probs;
    const uint8_t* buf;
    uint32_t range = 0xFFFFFFFF, code;
    uint32_t dicPos = 0;
    uint32_t processedPos = 0;
    uint32_t checkDicSize = 0;
    uint32_t state = 0;
    uint32_t rep0 = 1, rep1 = 1, rep2 = 1, rep3 = 1;
    uint32_t remainLen = 0;
    uint32_t dicSize;
    USE_GLOBAL_BLOCK

    *pOutSize = 0;
    if (inSize < 10) {
        return SZ_ERROR_INPUT_EOF;
    }
    inSize -= 10;

    dicSize = *(uint32_t*)(inBuffer + 1);
    lc = d % 9;
    d /= 9;
    pb = d / 5;
    lp = d % 5;

    probs = (uint32_t*)SYS_ALLOCATOR(((uint32_t)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp))) << 2);
    if (probs == NULL) {
        return SZ_ERROR_MEM;
    }

    *status = LZMA_STATUS_NOT_SPECIFIED;

    if (realSrc[0] != 0) {
        res = SZ_ERROR_DATA;
        goto end;
    }
#ifdef WIN32
    code = _byteswap_ulong(*(uint32_t*)(realSrc + 1));
#else
    code = bswap32(*(uint32_t*)(realSrc + 1));
#endif // WIN32

    realSrc += RC_INIT_SIZE;
#ifdef WIN32
    __stosd(probs, kBitModelTotal >> 1, Literal + ((uint32_t)LZMA_LIT_SIZE << (lc + lp)));
#else
	{
		uint32_t *ptr = probs, i;
		for (i = 0; i < Literal + ((uint32_t)LZMA_LIT_SIZE << (lc + lp)); ++i, ++ptr) {
			*ptr = kBitModelTotal >> 1;
		}
	}
#endif // WIN32

    while (remainLen != kMatchSpecLenStart) {
        if (dicPos >= outSize) {
            if (remainLen == 0 && code == 0) {
                *status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
            }
            else {
                *status = LZMA_STATUS_NOT_FINISHED;
            }            
            res = SZ_OK;
            goto end;
        }

        {
            size_t processed;
            const uint8_t* bufLimit = realSrc;

            if (inSize >= LZMA_REQUIRED_INPUT_MAX) {
                bufLimit += inSize - LZMA_REQUIRED_INPUT_MAX;
            }
            buf = realSrc;
            do {
                size_t limit2 = outSize;
                if (checkDicSize == 0) {
                    uint32_t rem = dicSize - processedPos;
                    if (outSize - dicPos > rem) {
                        limit2 = dicPos + rem;
                    }
                }
                {
                    uint32_t pbMask = ((uint32_t)1 << pb) - 1;
                    uint32_t lpMask = ((uint32_t)1 << lp) - 1;
                    uint32_t len = 0;

                    do {
                        uint32_t* prob;
                        uint32_t bound;
                        uint32_t ttt;
                        uint32_t posState = processedPos & pbMask;

                        prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
                        IF_BIT_0(prob) {
                            uint32_t symbol;
                            UPDATE_0(prob);
                            prob = probs + Literal;
                            if (checkDicSize != 0 || processedPos != 0) {
                                prob += (LZMA_LIT_SIZE * (((processedPos & lpMask) << lc) + (outBuffer[(dicPos == 0 ? outSize : dicPos) - 1] >> (8 - lc))));
                            }

                            if (state < kNumLitStates) {
                                state -= (state < 4) ? state : 3;
                                symbol = 1;
                                do {
                                    GET_BIT(prob + symbol, symbol)
                                } while (symbol < 0x100);
                            }
                            else {
                                uint32_t matchByte = outBuffer[(dicPos - rep0) + ((dicPos < rep0) ? outSize : 0)];
                                uint32_t offs = 0x100;
                                state -= (state < 10) ? 3 : 6;
                                symbol = 1;
                                do {
                                    uint32_t bit;
                                    uint32_t* probLit;
                                    matchByte <<= 1;
                                    bit = (matchByte & offs);
                                    probLit = prob + offs + bit + symbol;
                                    GET_BIT2(probLit, symbol, offs &= ~bit, offs &= bit)
                                } while (symbol < 0x100);
                            }
                            outBuffer[dicPos++] = (uint8_t)symbol;
                            processedPos++;
                            continue;
                        }
                else {
                    UPDATE_1(prob);
                    prob = probs + IsRep + state;
                    IF_BIT_0(prob) {
                        UPDATE_0(prob);
                        state += kNumStates;
                        prob = probs + LenCoder;
                    }
            else {
                UPDATE_1(prob);
                if (checkDicSize == 0 && processedPos == 0) {
                    return SZ_ERROR_DATA;
                }
                prob = probs + IsRepG0 + state;
                IF_BIT_0(prob) {
                    UPDATE_0(prob);
                    prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
                    IF_BIT_0(prob) {
                        UPDATE_0(prob);
                        outBuffer[dicPos] = outBuffer[(dicPos - rep0) + ((dicPos < rep0) ? outSize : 0)];
                        ++dicPos;
                        ++processedPos;
                        state = state < kNumLitStates ? 9 : 11;
                        continue;
                    }
                    UPDATE_1(prob);
                }
                else {
                    uint32_t distance;
                    UPDATE_1(prob);
                    prob = probs + IsRepG1 + state;
                    IF_BIT_0(prob) {
                        UPDATE_0(prob);
                        distance = rep1;
                    }
        else {
            UPDATE_1(prob);
            prob = probs + IsRepG2 + state;
            IF_BIT_0(prob) {
                UPDATE_0(prob);
                distance = rep2;
            }
    else {
        UPDATE_1(prob);
        distance = rep3;
        rep3 = rep2;
    }
    rep2 = rep1;
        }
        rep1 = rep0;
        rep0 = distance;
                }
                state = state < kNumLitStates ? 8 : 11;
                prob = probs + RepLenCoder;
            }
            {
                uint32_t limit = 8, offset;
                uint32_t* probLen = prob;
                IF_BIT_0(probLen) {
                    UPDATE_0(probLen);
                    probLen += LenLow + (posState << 3);
                    offset = 0;
                }
        else {
            UPDATE_1(probLen);
            probLen += 1;
            offset = kLenNumLowSymbols;
            IF_BIT_0(probLen) {
                UPDATE_0(probLen);
                probLen = prob + LenMid + (posState << 3);
            }
    else {
        UPDATE_1(probLen);
        probLen = prob + LenHigh;
        offset += kLenNumMidSymbols;
        limit = (1 << kLenNumHighBits);
    }
        }
        TREE_DECODE(probLen, limit, len);
        len += offset;
            }

            if (state >= kNumStates) {
                uint32_t distance;
                prob = probs + PosSlot + ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
                TREE_6_DECODE(prob, distance);
                if (distance >= kStartPosModelIndex) {
                    uint32_t posSlot = distance;
                    int numDirectBits = (int)(((distance >> 1) - 1));
                    distance = (2 | (distance & 1));
                    if (posSlot < kEndPosModelIndex) {
                        distance <<= numDirectBits;
                        prob = probs + SpecPos + distance - posSlot - 1;
                        {
                            uint32_t mask = 1;
                            uint32_t i = 1;
                            do {
                                GET_BIT2(prob + i, i, ; , distance |= mask);
                                mask <<= 1;
                            } while (--numDirectBits != 0);
                        }
                    }
                    else {
                        numDirectBits -= kNumAlignBits;
                        do {
                            NORMALIZE
                                range >>= 1;                                          
                            {
                                uint32_t t;
                                code -= range;
                                t = (0 - ((uint32_t)code >> 31));
                                distance = (distance << 1) + (t + 1);
                                code += range & t;
                            }
                        } while (--numDirectBits != 0);
                        prob = probs + Align;
                        distance <<= kNumAlignBits;
                        {
                            uint32_t i = 1;
                            GET_BIT2(prob + i, i, ; , distance |= 1);
                            GET_BIT2(prob + i, i, ; , distance |= 2);
                            GET_BIT2(prob + i, i, ; , distance |= 4);
                            GET_BIT2(prob + i, i, ; , distance |= 8);
                        }
                        if (distance == (uint32_t)0xFFFFFFFF) {
                            len += kMatchSpecLenStart;
                            state -= kNumStates;
                            break;
                        }
                    }
                }
                rep3 = rep2;
                rep2 = rep1;
                rep1 = rep0;
                rep0 = distance + 1;
                if (checkDicSize == 0) {
                    if (distance >= processedPos) {
                        res = SZ_ERROR_DATA;
                        goto end;
                    }
                }
                else if (distance >= checkDicSize) {
                    res = SZ_ERROR_DATA;
                    goto end;
                }
                state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
            }
            len += kMatchMinLen;

            if (outSize == dicPos) {
                return SZ_ERROR_DATA;
            }
            {
                size_t rem = outSize - dicPos;
                uint32_t curLen = ((rem < len) ? (uint32_t)rem : len);
                size_t pos = (dicPos - rep0) + ((dicPos < rep0) ? outSize : 0);

                processedPos += curLen;

                len -= curLen;
                if (pos + curLen <= outSize) {
                    uint8_t* dest = outBuffer + dicPos;
                    ptrdiff_t realSrc = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
                    const uint8_t* lim = dest + curLen;
                    dicPos += curLen;
                    do {
                        *dest = *(dest + realSrc);
                    } while (++dest != lim);
                }
                else {
                    do {
                        outBuffer[dicPos++] = outBuffer[pos];
                        if (++pos == outSize) {
                            pos = 0;
                        }
                    } while (--curLen != 0);
                }
            }
                }
                    } while (dicPos < outSize && buf < bufLimit);
                    NORMALIZE;
                    remainLen = len;
                }

                if (processedPos >= dicSize) {
                    checkDicSize = dicSize;
                }
                if (remainLen != 0 && remainLen < kMatchSpecLenStart) {
                    uint32_t len = remainLen;

                    if (outSize - dicPos < len) {
                        len = (uint32_t)(outSize - dicPos);
                    }

                    if (checkDicSize == 0 && dicSize - processedPos <= len) {
                        checkDicSize = dicSize;
                    }

                    processedPos += len;
                    remainLen -= len;
                    while (len-- != 0) {
                        outBuffer[dicPos] = outBuffer[(dicPos - rep0) + ((dicPos < rep0) ? outSize : 0)];
                        ++dicPos;
                    }
                }
            } while (dicPos < outSize && buf < bufLimit && remainLen < kMatchSpecLenStart);

            if (remainLen > kMatchSpecLenStart) {
                remainLen = kMatchSpecLenStart;
            }
            processed = (size_t)(buf - realSrc);
            realSrc += processed;
            inSize -= processed;
        }
    }
    if (code == 0)
        *status = LZMA_STATUS_FINISHED_WITH_MARK;
    res = (code == 0) ? SZ_OK : SZ_ERROR_DATA;
end:

    if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT) {
        res = SZ_ERROR_INPUT_EOF;
    }

    (*pOutSize) = dicPos;
    SYS_DEALLOCATOR(probs);
    return res;
}

#endif // USE_LZMA_DECOMPRESSOR
