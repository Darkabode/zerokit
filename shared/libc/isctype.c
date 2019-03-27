#include <Windows.h>
#include <ctype.h>
#include "libct.h"

int __cdecl iswctype(wint_t c, wctype_t type)
{
	WORD ret;

	GetStringTypeW(CT_CTYPE1, (LPCWSTR)&c, 1, &ret);
	if ((ret & type) != 0)
		return 1;
	return 0;
}

//int _ismbcspace(int c)	{return isspace(c);}
int __cdecl isspace(int c)
{
    return ((c >= 0x09 && c <= 0x0D) || (c == 0x20));
}

// int iswspace(wint_t c)
// {
//     return iswctype(c, _BLANK);
// }

//int _ismbcupper(int c)	{return isupper(c);}
int __cdecl isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

// int iswupper(wint_t c)
// {
//     return iswctype(c, _UPPER);
// }

int __cdecl islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

//int ismbcalpha(int c)	{return isalpha(c);}
int __cdecl isalpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// int iswalpha(wint_t c)
// {
//     return iswctype(c, _ALPHA);
// }

//int ismbcdigit(int c)	{return isdigit(c);}
int __cdecl isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

// int iswdigit(wint_t c)
// {
//     return iswctype(c, _DIGIT);
// }

//int ismbcxdigit(int c)	{return isxdigit(c);}
int __cdecl isxdigit(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// int iswxdigit(wint_t c)
// {
//     return iswctype(c, _HEX);
// }

//int ismbcalnum(int c)	{return isalnum(c);}
int __cdecl isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}

// int iswalnum(wint_t c)
// {
//     return iswctype(c, _ALPHA|_DIGIT);
// }

//int ismbcprint(int c)	{return isprint(c);}
int __cdecl isprint(int c)
{
    return c >= ' ';
}

// int iswprint(wint_t c)
// {
//     return iswctype(c, (wctype_t)(~_CONTROL));
// }

int __cdecl isgraph(int ch)
{
    return (unsigned int)(ch - '!') < 127u - '!';
}

int __cdecl ispunct( int ch ) 
{
    return isprint (ch)  &&  !isalnum (ch)  &&  !isspace (ch);
}

int __cdecl iscntrl( int ch )
{
    return (unsigned int)ch < 32u  ||  ch == 127;
}