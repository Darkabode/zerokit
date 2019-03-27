#include <stdlib.h>
#include <ctype.h>
#include "libct.h"

long __cdecl atol(const char *str)
{
    int cur, neg;
    long total;

    while (isspace(*str))			// skip whitespace
        ++str;

    cur = *str++;
    neg = cur;					// Save the negative sign, if it exists

    if (cur == '-' || cur == '+')
        cur = *str++;

    // While we have digits, add 'em up.

	total = 0;
    while (isdigit(cur))
    {
        total = 10*total + (cur-'0');			// Add this digit to the total.
        cur = *str++;							// Do the next character.
    }

    // If we have a negative sign, convert the value.
    if (neg == '-')
        return -total;
    else
        return total;
}

int __cdecl atoi(const char *str)
{
    return (int)atol(str);
}



long __cdecl wtol(const wchar_t *str)
{
    wint_t cur, neg;
    long total;

    while (iswspace(*str))			// skip whitespace
        ++str;

    cur = *str++;
    neg = cur;					// Save the negative sign, if it exists

    if (cur == L'-' || cur == L'+')
        cur = *str++;

    // While we have digits, add 'em up.

	total = 0;
    while (iswdigit(cur))
    {
        total = 10*total + (cur-L'0');			// Add this digit to the total.
        cur = *str++;							// Do the next character.
    }

    // If we have a negative sign, convert the value.
    if (neg == L'-')
        return -total;
    else
        return total;
}

int __cdecl wtoi(const wchar_t *str)
{
    return (int)wtol(str);
}
