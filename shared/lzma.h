#ifndef __SHARED_LZMA_H_
#define __SHARED_LZMA_H_

// Compressor

#define LZMA_PROPS_SIZE 5

typedef struct _CLzmaEncProps
{
  uint32_t dictSize; /* (1 << 12) <= dictSize <= (1 << 27) for 32-bit version
                      (1 << 12) <= dictSize <= (1 << 30) for 64-bit version
                       default = (1 << 24) */
  int lc;          /* 0 <= lc <= 8, default = 3 */
  int lp;          /* 0 <= lp <= 4, default = 0 */
  int pb;          /* 0 <= pb <= 4, default = 2 */
  int fb;          /* 5 <= fb <= 273, default = 32 */
} CLzmaEncProps;


#define SZ_OK 0
#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12

#define SZ_ERROR_ARCHIVE 16
#define SZ_ERROR_NO_ARCHIVE 17

#define RINOK(x) { int __result__ = (x); if (__result__ != 0) return __result__; }

/* The following interfaces use first parameter as pointer to structure */

typedef struct
{
  int (*Read)(void *p, void *buf, size_t *size);
    /* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
       (output(*size) < input(*size)) is allowed */
} ISeqInStream;

typedef struct
{
  size_t (*Write)(void *p, const void *buf, size_t size);
    /* Returns: result - the number of actually written bytes.
       (result < size) means error */
} ISeqOutStream;


#define CHAR_PATH_SEPARATOR '\\'
#define WCHAR_PATH_SEPARATOR L'\\'
#define STRING_PATH_SEPARATOR "\\"


// Decompressor

/* LZMA_REQUIRED_INPUT_MAX = number of required input bytes for worst case.
   Num bits = log2((2^11 / 31) ^ 22) + 26 < 134 + 26 = 160; */

#define LZMA_REQUIRED_INPUT_MAX 20

/* ELzmaFinishMode has meaning only if the decoding reaches output limit !!!

   You must use LZMA_FINISH_END, when you know that current output buffer
   covers last bytes of block. In other cases you must use LZMA_FINISH_ANY.

   If LZMA decoder sees end marker before reaching output limit, it returns SZ_OK,
   and output value of destLen will be less than output buffer size limit.
   You can check status result also.

   You can use multiple checks to test data integrity after full decompression:
     1) Check Result and "status" variable.
     2) Check that output(destLen) = uncompressedSize, if you know real uncompressedSize.
     3) Check that output(srcLen) = compressedSize, if you know real compressedSize.
        You must use correct finish mode in that case. */

typedef enum
{
  LZMA_STATUS_NOT_SPECIFIED,               /* use main error code instead */
  LZMA_STATUS_FINISHED_WITH_MARK,          /* stream was finished with end mark. */
  LZMA_STATUS_NOT_FINISHED,                /* stream was not finished */
  LZMA_STATUS_NEEDS_MORE_INPUT,            /* you must provide more input bytes */
  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK  /* there is probability that stream was finished without end mark */
} ELzmaStatus;

#endif // __SHARED_LZMA_H_
