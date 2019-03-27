#ifndef __UTILS_H_
#define __UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void* memalloc(size_t sz);
void* memrealloc(void* pBuffer, size_t newSize);
void memfree(void* pBuffer);

// int utils_isctype(int c, wctype_t type);
// int utils_isalpha(int c);
// void utils_to_lower(char* str);
int utils_abs(int n);
int64_t utils_atoi64(char* str);
int utils_strncmp(const char* s1, const char* s2, size_t n);
uint32_t utils_strnicmp(const char* s1, const char* s2, size_t n);
int utils_memcmp(const void* buf1, const void* buf2, size_t count);

uint32_t utils_strhash(const char* pszString);
uint32_t utils_strihash(const char* pszString);

const char* utils_get_base_name(const char* fullName, uint32_t* pSize);

#define TOK_TRIM 1
#define TOK_IGNORE_EMPTY 2
char** utils_tokenize(const char* str, const char* separators, int options, uint32_t* pCount);
wchar_t* utils_ansi2wide(const char* strA);

PVOID utils_map_file(const wchar_t* lpPath, DWORD dwFileAccess, DWORD dwFileFlags, DWORD dwPageAccess, DWORD dwMapAccess, DWORD mapSize, uint32_t* pdwSize);
BOOL utils_file_write(const wchar_t* filePath, DWORD dwFlags, uint8_t* pBuffer, DWORD dwSize);
uint8_t* utils_file_read(const wchar_t* lpFile, uint32_t* pdwSize);

uint8_t* utils_decrypt_buffer(const uint8_t* cryptedData, uint32_t size, uint32_t* pOutSize);

uint32_t utils_get_current_unixtime();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __UTILS_H_
