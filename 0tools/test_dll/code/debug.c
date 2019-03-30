#include "types.h"
#include <Windows.h>
#include <stdarg.h>

#include "debug.h"


void __cdecl DbgPrintf(char* fmt, ...)
{
    char szBuf[1024];
    va_list arglist;

    __stosb(szBuf, 0, sizeof(szBuf));

    va_start(arglist, fmt);
    wvsprintfA(szBuf, fmt, arglist);
    va_end(arglist);

    OutputDebugString(szBuf);
}

void __cdecl DbgPrintfArr(char* name, uint8_t* ptr, uint32_t size)
{
    uint32_t i;
    char str[512];
    char alphas[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    __stosb(str, 0, sizeof(str));
    for (i = 0; i < size; ++i) {
        str[2 * i] = alphas[ptr[i] & 0x0F];
        str[2 * i + 1] = alphas[ptr[i] >> 4];
    }

    DbgPrintf("%s: %s\n", name, &str);
}
