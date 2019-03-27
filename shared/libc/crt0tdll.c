#include "ec.h"
#include "libct.h"

BOOL WINAPI DllMain(HANDLE, DWORD, LPVOID);

EXTERN_C BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, DWORD reason, LPVOID imp)
{
    BOOL ret;

	if (reason == DLL_PROCESS_ATTACH)
	{
		_init_atexit();
		_init_file();
		_initterm(__xc_a, __xc_z);
	}

	ret = DllMain(hInst, reason, imp);

	if (reason == DLL_PROCESS_DETACH)
	{
		_doexit();
	}

	return ret;
}