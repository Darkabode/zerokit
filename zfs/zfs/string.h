#ifndef __ZFS_STRING_H_
#define __ZFS_STRING_H_

void zfs_tolower(char* string, uint32_t strLen);
//void zfs_toupper(char* string, uint32_t strLen);
char zfs_strmatch(const char* str1, const char* str2, uint16_t len);
char* zfs_strtok(const char* string, char* token, uint16_t *tokenNumber, char* last, uint16_t Length);
char zfs_wildcompare(const char* pszWildCard, const char* pszString);

#endif // __ZFS_STRING_H_
