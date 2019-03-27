#include <Windows.h>
#include <string.h>
#include "libct.h"

char* __cdecl strupr(char *s)
{
    CharUpperBuffA(s, lstrlenA(s));
    return s;
}

wchar_t* __cdecl wcsupr(wchar_t *s)
{
    CharUpperBuffW(s, lstrlenW(s));
    return s;
}

char* __cdecl strlwr(char *s)
{
    CharLowerBuffA(s, lstrlenA(s));
    return s;
}

wchar_t* __cdecl wcslwr(wchar_t *s)
{
    CharLowerBuffW(s, lstrlenW(s));
    return s;
}

int __cdecl toupper(int c)
{
	if (c < 'a' || c > 'z')
		return c;
	return c-0x20;
}

wint_t __cdecl towupper(wint_t c)
{
	// yes, that's right:  no &c - see the docs
	return (wint_t)CharUpperW((LPWSTR)c);
}

int __cdecl tolower(int c)
{
	if (c < 'A' || c > 'Z')
		return c;
	return c+0x20;
}

wint_t __cdecl towlower(wint_t c)
{
	// yes, that's right:  no &c - see the docs
	return (wint_t)CharLowerW((LPWSTR)c);
}
