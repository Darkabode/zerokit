#ifndef __DEBUG_H_
#define __DEBUG_H_

#define USE_DBGPRINT 1

#ifdef USE_DBGPRINT

#define DBG_PRINTF(x) DbgPrintf x
#define DBG_PRINTF_ARR(name, ptr, size) DbgPrintfArr(name, ptr, size)

void __cdecl DbgPrintf(char* fmt, ...);
void __cdecl DbgPrintfArr(char* name, uint8_t* ptr, uint32_t size);

#else

#define DBG_PRINTF(x)
#define DBG_PRINTF_ARR(name, ptr, size)

#endif // USE_DBGPRINT

#endif // __DEBUG_H_
