#include <Windows.h>
#include "platform.h"
#include "types.h"
#include "utils.h"


// int utils_isctype(int c, wctype_t type)
// {
//     WORD ret;
// 
//     GetStringTypeA(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, (LPCSTR)&c, 1, &ret);
//     if ((ret & type) != 0) {
//         return 1;
//     }
//     return 0;
// }
// 
// int utils_isalpha(int c)
// {
//     return utils_isctype(c, _ALPHA);
// }
// 
// 
// void utils_to_lower(char* str)
// {
//     char* ptr = str;
// 
//     for ( ; *ptr != '\0'; ++ptr) {
//         if (utils_isalpha(*ptr)) {
//             *ptr |= 0x20;
//         }
//     }
// }

int utils_abs(int n)
{
    return (n >= 0 ? n : -n);
}

int64_t utils_atoi64(char* str)
{
    LONGLONG val64;
    if (fn_StrToInt64ExA(str, STIF_DEFAULT, &val64)) {
        return (int64_t)val64;
    }
    return 0;
}

void* memalloc(size_t sz)
{
    return fn_HeapAlloc(fn_GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
}

void* memrealloc(void* pBuffer, size_t newSize)
{
    if (pBuffer == 0) {
        return memalloc(newSize);
    }
    return fn_HeapReAlloc(fn_GetProcessHeap(), HEAP_ZERO_MEMORY, pBuffer, newSize);
}

void memfree(void* pBuffer)
{
    fn_HeapFree(fn_GetProcessHeap(), 0, pBuffer);
}

int utils_strncmp(const char* s1, const char* s2, size_t n)
{
    size_t i;
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;

    if (n == 0) {
        return 0;
    }

    for (i = 0; i < n; ++i) {
        if (p1[i] == '\0' || p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }

    return 0;
}

// uint32_t utils_strnicmp(const char* s1, const char* s2, size_t n)
// {
//     uint32_t i, sz = min(min((uint32_t)lstrlenA(s1), (uint32_t)lstrlenA(s2)), (uint32_t)n);
//     char lowerStr1[512];
//     char lowerStr2[512];
// 
//     memset((unsigned char*)lowerStr1, 0, sizeof(lowerStr1));
//     memset((unsigned char*)lowerStr2, 0, sizeof(lowerStr2));
//     for (i = 0; i < sz; ++i) {
//         lowerStr1[i] = s1[i] | 0x20;
//         lowerStr2[i] = s2[i] | 0x20;
//     }
//     return utils_strncmp(lowerStr1, lowerStr2, n);
// }

int utils_memcmp(const void* buf1, const void* buf2, size_t count)
{
    if (count == 0) {
        return 0;
    }

    while (--count && *(char*)buf1 == *(char*)buf2) {
        buf1 = (char*)buf1 + 1;
        buf2 = (char*)buf2 + 1;
    }

    return(*((uint8_t*)buf1) - *((uint8_t*)buf2));
}

uint32_t utils_strhash(const char* pszString)
{
    uint32_t hashVal = 0;
    const char* ptr = pszString;

    for ( ; *ptr != '\0';  ++ptr) {
        hashVal = ((hashVal >> 11) | (hashVal << (32 - 11)));
        hashVal += *ptr | 0x20;
    }
    return hashVal;
}

uint32_t utils_strihash(const char* pszString)
{
    uint32_t i, sz = fn_lstrlenA(pszString);
    char lowerStr[1024];

    __stosb((unsigned char*)lowerStr, 0, sizeof(lowerStr));
    for (i = 0; i < sz; ++i) {
        lowerStr[i] = pszString[i] | 0x20;
    }
    return utils_strhash(lowerStr);
}

// const char* utils_get_base_name(const char* fullName, uint32_t* pSize)
// {
//     uint32_t len = 0xFFFFFFFF;
//     const char* ptr;
// 
//     ptr = fullName + lstrlenA(fullName);
//     for ( ; ptr >= fullName && *ptr != '\\'; --ptr, ++len);
// 
//     if (pSize != NULL) {
//         *pSize = len;
//     }
// 
//     return ++ptr;
// }

char** utils_tokenize(const char* str, const char* separators, int options, uint32_t* pCount)
{
    const char* itr = str;
    const char* end = str + fn_lstrlenA(str);
    char** tokens;
    uint32_t count = 0;
    char* token = (char*)memalloc(fn_lstrlenA(str) + 1);
    char* tokenItr = token;
    char* pLastToken = token;
    int doTrim = ((options & TOK_TRIM) != 0);
    int ignoreEmpty = ((options & TOK_IGNORE_EMPTY) != 0);
    int lastToken = 0;

    for ( ; itr != end; ++itr) {
        if (fn_StrChrA(separators, *itr) != NULL) {
            if (pLastToken[0] != '\0' || !ignoreEmpty) {
                ++count;
                *(tokenItr++) = '\0';
            }
            if (!ignoreEmpty) {
                lastToken = 1;
            }
            pLastToken = tokenItr;
        }
        else {
            if (doTrim && *itr != ' ' && *itr != '\t') {
                *(tokenItr++) = *itr;
            }
            else if (!doTrim) {
                *(tokenItr++) = *itr;
            }
            
            lastToken = 0;
        }
    }

    if (pLastToken[0] != '\0') {
        ++count;
    }
    else if (lastToken) {
        ++count;
    }
    *(tokenItr++) = 0;

    tokens = (char**)memalloc(sizeof(char*) * count);
    *pCount = count;

    for (lastToken = 0, tokenItr = token; lastToken < count; ++tokenItr) {
        tokens[lastToken++] = tokenItr;
        for ( ; *tokenItr != '\0'; ++tokenItr);
    }

    return tokens;
}

wchar_t* utils_ansi2wide(const char* strA)
{
    int wLen;
    wchar_t* strW;

    wLen = fn_MultiByteToWideChar(CP_ACP, 0, strA, -1, NULL, 0);
    strW = (wchar_t*)memalloc(wLen * sizeof(wchar_t));
    fn_MultiByteToWideChar(CP_ACP, 0, strA, -1, strW, wLen);

    return strW;
}

PVOID utils_map_file(const wchar_t* lpPath, DWORD dwFileAccess, DWORD dwFileFlags, DWORD dwPageAccess, DWORD dwMapAccess, DWORD mapSize, uint32_t* pdwSize)
{
    PVOID pMap = NULL;
    HANDLE hMapping;
    HANDLE hFile;
    DWORD fileSize;

    hFile = fn_CreateFileW(lpPath, dwFileAccess, FILE_SHARE_READ, NULL, OPEN_EXISTING, dwFileFlags, 0);
    if (hFile != INVALID_HANDLE_VALUE) {
        fileSize = fn_GetFileSize(hFile, NULL);
        if (fileSize == 0) {
            fn_CloseHandle(hFile);
            return pMap;
        }
		if (mapSize == 0 || mapSize > fileSize) {
            mapSize = fileSize;
        }
        hMapping = fn_CreateFileMappingW(hFile, NULL, dwPageAccess, 0, mapSize, 0);
        if (hMapping != NULL) {
            pMap = fn_MapViewOfFile(hMapping, dwMapAccess, 0, 0, 0);
            if (pMap != NULL) {
				*pdwSize = mapSize;
            }
            fn_CloseHandle(hMapping);
        }
        fn_CloseHandle(hFile);
    }

    return pMap;
}

BOOL utils_file_write(const wchar_t* filePath, DWORD dwFlags, uint8_t* pBuffer, DWORD dwSize)
{
    BOOL bRet = FALSE;
    HANDLE hFile;

    hFile = fn_CreateFileW(filePath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, dwFlags, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD remain = dwSize, written = 0;
        while (remain != 0) {
            bRet = fn_WriteFile(hFile, pBuffer + written, remain, &dwSize, NULL);
            remain -= dwSize;
            written += dwSize;
        }
        fn_SetEndOfFile(hFile);
		fn_FlushFileBuffers(hFile);
        fn_CloseHandle(hFile);
    }

    return bRet;
}

uint8_t* utils_file_read(const wchar_t* lpFile, uint32_t* pdwSize)
{
    uint8_t* pvBuffer = NULL;
    HANDLE hFile;

    hFile = fn_CreateFileW(lpFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD dwSize;

        dwSize = fn_GetFileSize(hFile, NULL);
        pvBuffer = (uint8_t*)memalloc(dwSize);
        if (pvBuffer != NULL) {
            DWORD remain = dwSize, readed = 0;
            while (remain != 0 && dwSize != 0) {
                fn_ReadFile(hFile, pvBuffer + readed, remain, &dwSize, NULL);
                remain -= dwSize;
                readed += dwSize;
            }

            if (pdwSize != NULL) {
                *pdwSize = (uint32_t)readed;
            }
        }
        fn_CloseHandle(hFile);
    }

    return pvBuffer;
}

uint8_t* utils_decrypt_buffer(const uint8_t* cryptedData, uint32_t size, uint32_t* pOutSize)
{
    int prefixSize;
    int keySize = (int)cryptedData[1];
    int rndPos = (int)cryptedData[0];
    int i, k;
    char* ptr;
    uint8_t* key = (uint8_t*)memalloc(keySize);
    uint8_t* buffer;
    uint32_t realSize;

    prefixSize = keySize * 7 + 2;
    realSize = size - (uint32_t)prefixSize;
    if (pOutSize != NULL) {
        *pOutSize = realSize;
    }

    buffer = (uint8_t*)memalloc(realSize + 1);
    buffer[realSize] = 0;
    __movsb(buffer, cryptedData + prefixSize, realSize);

    for (k = 0, i = 2; i < prefixSize; ++i) {
        if ((i - 2) % 7 == rndPos) {
            key[k++] = cryptedData[i];
        }
    }

    arc4_crypt_self(buffer, realSize, key, keySize);
    memfree(key);

    return buffer;
}

/*
 * Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
 */
#define EPOCH_BIAS  116444736000000000i64

#define _MAX__TIME32_T     0x7fffd27f           /* number of seconds from
                                                   00:00:00, 01/01/1970 UTC to
                                                   23:59:59, 01/18/2038 UTC */

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
} FT;

uint32_t utils_get_current_unixtime()
{
    SYSTEMTIME st;
    __time64_t unixTime;
    FT nt_time;

    fn_GetLocalTime(&st);
    fn_SystemTimeToFileTime(&st, &nt_time.ft_struct);

    //GetSystemTimeAsFileTime(&nt_time.ft_struct);
#ifdef _WIN64
    unixTime = (__time64_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);
#else
    unixTime = (__time64_t)fn__aulldiv(nt_time.ft_scalar - EPOCH_BIAS, 10000000i64);
#endif // _WIN64

    if (unixTime > (__time64_t)(_MAX__TIME32_T)) {
        unixTime = (__time64_t)-1;
    }

    return (uint32_t)unixTime;
}

