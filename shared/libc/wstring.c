#include <Windows.h>
#include <string.h>
#include "libct.h"

extern HANDLE gHeap;

#undef malloc
#define malloc(sz) HeapAlloc(gHeap, 0, sz)

#undef free
#define free(ptr) HeapFree(gHeap, 0, ptr)

int __cdecl _wcsicmp(const wchar_t *s1, const wchar_t *s2) {
    return wcsicmp(s1, s2);
}

int __cdecl wcsicmp(const wchar_t *s1, const wchar_t *s2)
{
    return lstrcmpiW(s1, s2);
}

// int __cdecl wcscmp(const wchar_t *s1, const wchar_t *s2)
// {
// 	return lstrcmpW(s1, s2);
// }

int __cdecl wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
    size_t i;

	if (!n)
		return 0;

	for (i = 0; i < n; i++)
	{
		if (!s1[i] || s1[i] != s2[i])
			return s1[i] - s2[i];
	}

	return 0;
}

int __cdecl wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
	return CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, s1, n, s2, n);
}

wchar_t* __cdecl wcsdup(const wchar_t *src)
{
    wchar_t *dst;

	if (!src)
		return 0;

	dst = (wchar_t*)malloc((wcslen(src)+1)*sizeof(wchar_t));
	wcscpy(dst, src);
	return dst;
}

wchar_t* __cdecl _wcsdup(const wchar_t *src)
{
	return wcsdup(src);
}

// wchar_t* __cdecl wcscpy(wchar_t *dest, const wchar_t *src)
// {
// 	return lstrcpyW(dest, src);
// }

wchar_t* __cdecl wcsncpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    size_t len;
	
    __movsb(dest, src, n*sizeof(wchar_t));
	len = wcslen(src);
	if (n > len)
		__stosb(&dest[len], 0, (n-len)*sizeof(wchar_t));
	return dest;
}

// size_t __cdecl wcslen(const wchar_t *str)
// {
// 	return lstrlenW(str);
// }

wchar_t* __cdecl wcschr(const wchar_t *str, wchar_t ch)
{
	while (*str) {
		if (*str == ch)
			return (wchar_t*)str;
		str++;
	}
	return NULL;
}

wchar_t* __cdecl wcsrchr(const wchar_t *str, wchar_t ch)
{
	const wchar_t *end = str + wcslen(str) + 1;
	while (end != str) {
		end--;
		if (*end == ch)
			return (wchar_t*)end;
	}
	return NULL;
}

// wchar_t* __cdecl wcscat(wchar_t *dst, const wchar_t *src)
// {
// 	return lstrcatW(dst, src);
// }

wchar_t* __cdecl wcsstr(const wchar_t *str, const wchar_t *substr)
{
    int i;
	int str_len = wcslen(str);
	int substr_len = wcslen(substr);

	if (substr_len == 0)
		return (wchar_t*)str;
	if (str_len < substr_len)
		return 0;
	for (i = 0; i < (int)(str_len-substr_len+1); i++)
	{
		if (!wcscmp(&str[i], substr))
			return (wchar_t*)(&str[i]);
	}
	return NULL;
}
