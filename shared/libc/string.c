#include <Windows.h>
#include <string.h>
#include <errno.h>
#include "libct.h"

extern HANDLE gHeap;

#undef malloc
#define malloc(sz) HeapAlloc(gHeap, 0, sz)

#undef free
#define free(ptr) HeapFree(gHeap, 0, ptr)

int __cdecl strcmpi(const char *s1, const char *s2)
{
    return lstrcmpiA(s1, s2);
}

int __cdecl stricmp(const char *s1, const char *s2) {
    return _stricmp(s1, s2);
}

int __cdecl _stricmp(const char *s1, const char *s2)
{
    return lstrcmpiA(s1, s2);
}

int __cdecl strcmp(const char *s1, const char *s2)
{
	return lstrcmpA(s1, s2);
}

int __cdecl strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;
    const unsigned char *p1 = (const unsigned char*)s1;
    const unsigned char *p2 = (const unsigned char*)s2;

	if (!n)
		return 0;

	for (i = 0; i < n; ++i) {
		if (!p1[i] || p1[i] != p2[i])
			return p1[i] - p2[i];
	}

	return 0;
}

int __cdecl strnicmp(const char *s1, const char *s2, size_t n)
{
	return CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, s1, n, s2, n) - CSTR_EQUAL;
}

char* __cdecl strdup(const char *src)
{
    char *dst;

    if (src == NULL) {
		return 0;
    }

	dst = (char*)malloc(strlen(src) + 1);
	strcpy(dst, src);
	return dst;
}

char* __cdecl _strdup(const char *src)
{
	return strdup(src);
}

char* __cdecl strcpy(char *dest, const char *src)
{
	return lstrcpyA(dest, src);
}

char* __cdecl strncpy(char *dest, const char *src, size_t n)
{
    size_t len;

	__movsb(dest, src, n);
	len = strlen(src);
	if (n > len)
		__stosb(&dest[len], 0, n-len);
	return dest;
}

size_t __cdecl strlen(const char *str)
{
	return lstrlenA(str);
}

char* __cdecl strchr(const char *str, int ch)
{
	while (*str)
	{
		if (*str == ch)
			return (char*)str;
		str++;
	}
	return NULL;
}

char* __cdecl strrchr(const char *str, int ch)
{
	const char *end = str + strlen(str) + 1;
	while (end != str)
	{
		end--;
		if (*end == ch)
			return (char*)end;
	}
	return NULL;
}

// char* __cdecl strcat(char *dst, const char *src)
// {
// 	return lstrcatA(dst, src);
// }

char* __cdecl strstr(const char *str, const char *substr)
{
    int i;
	int str_len = strlen(str);
	int substr_len = strlen(substr);

	if (substr_len == 0)
		return (char*)str;
	if (str_len < substr_len)
		return 0;
	for (i = 0; i < (int)(str_len-substr_len+1); i++)
	{
		if (!strcmp(&str[i], substr))
			return (char*)(&str[i]);
	}
	return NULL;
}


/* flag values */
#define FL_UNSIGNED   1       /* strtoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */


unsigned long strtoxl (/*_locale_t plocinfo, */const char *nptr, const char **endptr, int ibase, int flags)
{
    const char *p;
    char c;
    unsigned long number;
    unsigned digval;
    unsigned long maxval;
//    _LocaleUpdate _loc_update(plocinfo);

    /* validation section */
    if (endptr != NULL)
    {
        /* store beginning of string in endptr */
        *endptr = (char *)nptr;
    }
//     _VALIDATE_RETURN(nptr != NULL, EINVAL, 0L);
//     _VALIDATE_RETURN(ibase == 0 || (2 <= ibase && ibase <= 36), EINVAL, 0L);

    p = nptr;                       /* p is our scanning pointer */
    number = 0;                     /* start with zero */

    c = *p++;                       /* read char */
    while (isspace((int)(unsigned char)c))
        c = *p++;               /* skip whitespace */

    if (c == '-') {
        flags |= FL_NEG;        /* remember minus sign */
        c = *p++;
    }
    else if (c == '+')
        c = *p++;               /* skip sign */

    if (ibase < 0 || ibase == 1 || ibase > 36) {
        /* bad base! */
        if (endptr)
            /* store beginning of string in endptr */
            *endptr = nptr;
        return 0L;              /* return 0 */
    }
    else if (ibase == 0) {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != '0')
            ibase = 10;
        else if (*p == 'x' || *p == 'X')
            ibase = 16;
        else
            ibase = 8;
    }

    if (ibase == 0) {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != '0')
            ibase = 10;
        else if (*p == 'x' || *p == 'X')
            ibase = 16;
        else
            ibase = 8;
    }

    if (ibase == 16) {
        /* we might have 0x in front of number; remove if there */
        if (c == '0' && (*p == 'x' || *p == 'X')) {
            ++p;
            c = *p++;       /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval = ULONG_MAX / ibase;


    for (;;) {      /* exit in middle of loop */
        /* convert c to value */
        if (isdigit((int)(unsigned char)c))
            digval = c - '0';
        else if (isalpha((int)(unsigned char)c) )
            digval = toupper(c) - 'A' + 10;
        else
            break;
        if (digval >= (unsigned)ibase)
            break;          /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags |= FL_READDIGIT;

        /* we now need to compute number = number * base + digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if (number < maxval || (number == maxval &&
                    (unsigned long)digval <= ULONG_MAX % ibase)) {
            /* we won't overflow, go ahead and multiply */
            number = number * ibase + digval;
        }
        else {
            /* we would have overflowed -- set the overflow flag */
            flags |= FL_OVERFLOW;
            if (endptr == NULL) {
                /* no need to keep on parsing if we
                   don't have to return the endptr. */
                break;
            }
        }

        c = *p++;               /* read next digit */
    }

    --p;                            /* point to place that stopped scan */

    if (!(flags & FL_READDIGIT)) {
        /* no number there; return 0 and point to beginning of
           string */
        if (endptr)
            /* store beginning of string in endptr later on */
            p = nptr;
        number = 0L;            /* return 0 */
    }
    else if ( (flags & FL_OVERFLOW) ||
            ( !(flags & FL_UNSIGNED) &&
              ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
                ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
    {
        /* overflow or signed overflow occurred */
        //errno = ERANGE;
        if ( flags & FL_UNSIGNED )
            number = ULONG_MAX;
        else if ( flags & FL_NEG )
            number = (unsigned long)(-LONG_MIN);
        else
            number = LONG_MAX;
    }

    if (endptr != NULL)
        /* store pointer to char that stopped the scan */
        *endptr = p;

    if (flags & FL_NEG)
        /* negate result if there was a neg sign */
        number = (unsigned long)(-(long)number);

    return number;                  /* done. */
}


long __cdecl strtol(const char *nptr, char **endptr, int ibase)
{
//     if (__locale_changed == 0)
//     {
//         return (long) strtoxl(&__initiallocalestructinfo, nptr, (const char **)endptr, ibase, 0);
//     }
//     else
//     {
        return (long)strtoxl(/*NULL, */nptr, (const char **)endptr, ibase, 0);
//     }
}


unsigned long __cdecl strtoul (const char *nptr, char **endptr, int ibase)
{
//     if (__locale_changed == 0)
//     {
//         return strtoxl(&__initiallocalestructinfo, nptr, (const char **)endptr, ibase, FL_UNSIGNED);
//     }
//     else
//     {
        return strtoxl(/*NULL, */nptr, (const char **)endptr, ibase, FL_UNSIGNED);
//     }
}

size_t __cdecl strspn(const char *s, const char *accept)
{
    size_t l = 0;
    const char *a;

    for (; *s; s++) {
        for (a = accept; *a && *s != *a; a++);

        if (!*a)
            break;
        else
            l++;
    }

    return l;
}


char* __cdecl strpbrk(const char *s, const char *accept)
{
    register unsigned int i;
    for (; *s; s++)
        for (i=0; accept[i]; i++)
            if (*s == accept[i])
                return (char*)s;
    return 0;
}