#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>

void LoadRequest()
{
    MessageBoxW(NULL, L"OK", L"Done!", MB_OK);
}