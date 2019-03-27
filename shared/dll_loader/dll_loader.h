#ifndef __SHARED_DLL_LOADER_H_
#define __SHARED_DLL_LOADER_H_

typedef struct dll_handle
{
#ifndef OS_RING3
    PEPROCESS pep;
    PETHREAD pet;
#endif // OS_RING3
    uint8_t* scBuffer;
    uint32_t scSize;
	PIMAGE_NT_HEADERS pNtHeaders;
	unsigned char* moduleBase;
	int initialized;
} dll_handle_t, *pdll_handle_t;

#endif // __SHARED_DLL_LOADER_H_
