namespace zgui
{

HttpClient::HttpClient(const String &url) :
_requestURL(url),
_sessionHandle(0),
_pResponse(0),
_responseByteCount(0),
m_pDataToSend(0),
m_dataToSendSize(0),
_dwLastError(0),
m_resolveTimeout(0),
m_connectTimeout(60000),
m_sendTimeout(30000),
m_receiveTimeout(30000)
{
    char userAgent[260];
    DWORD dwUARet = sizeof(userAgent);
    if (fn_ObtainUserAgentString(0, userAgent, &dwUARet) == NOERROR) {
        _userAgent = userAgent;
    }
    else {
        _userAgent = "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT %d.%d;%s Trident/5.0)";
    }
}

HttpClient::~HttpClient(void)
{
    if (_pResponse != 0) {
        fn_memfree(_pResponse);
    }
    if (m_pDataToSend != 0) {
        fn_memfree(m_pDataToSend);
    }

    if (_sessionHandle != 0) {
        fn_WinHttpCloseHandle(_sessionHandle);
    }
}

bool HttpClient::sendHttpRequest(const String& httpVerb)
{
    bool bRetVal = true;
    wchar_t* szHostName;
    wchar_t* szURLPath;
    URL_COMPONENTS urlComp;

    if (_requestURL.length() <= 0) {
        _dwLastError = ERROR_PATH_NOT_FOUND;
        return false;
    }    

    if (_sessionHandle == NULL) {
        _sessionHandle = fn_WinHttpOpen(_userAgent.toWideCharPointer(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (_sessionHandle == 0) {
            _dwLastError = fn_GetLastError();
            return false;
        }
    }

    szHostName = (wchar_t*)fn_memalloc(MAX_PATH * sizeof(wchar_t));
    szURLPath = (wchar_t*)fn_memalloc(7 * MAX_PATH * sizeof(wchar_t));

    __stosb((uint8_t*)&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = szHostName;
    urlComp.dwHostNameLength = MAX_PATH;
    urlComp.lpszUrlPath = szURLPath;
    urlComp.dwUrlPathLength = 7 * MAX_PATH;
    urlComp.dwSchemeLength = 1; // None zero

    fn_WinHttpSetTimeouts(_sessionHandle, m_resolveTimeout, m_connectTimeout, m_sendTimeout, m_receiveTimeout);

    if (fn_WinHttpCrackUrl(_requestURL.toWideCharPointer(), _requestURL.length(), 0, &urlComp)) {
        if (_requestHost.isEmpty()) {
            _requestHost = szHostName;
        }
        _additionalHeaders += "Host: ";
        _additionalHeaders += _requestHost;
        _additionalHeaders += "\r\n";
        HINTERNET hConnect = NULL;
        hConnect = fn_WinHttpConnect(_sessionHandle, szHostName, urlComp.nPort, 0);
        if (hConnect != NULL) {
            HINTERNET hRequest = NULL;
            hRequest = fn_WinHttpOpenRequest(hConnect, httpVerb.toWideCharPointer(), urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            if (hRequest != NULL) {
                bool bGetReponseSucceed = false;
                uint32_t iRetryTimes = 0;

                // Retry for several times if fails.
                while (!bGetReponseSucceed && iRetryTimes++ < INT_RETRYTIMES) {
                    if (_additionalHeaders.length() > 0) {
                        if (!fn_WinHttpAddRequestHeaders(hRequest, _additionalHeaders.toWideCharPointer(), _additionalHeaders.length(), WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON)) {
                            _dwLastError = fn_GetLastError();
                        }
                    }

                    DWORD dwDisableFeature = WINHTTP_DISABLE_REDIRECTS;
                    if (!fn_WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &dwDisableFeature, sizeof(dwDisableFeature))) {
                        _dwLastError = fn_GetLastError();
                    }
                    bool bSendRequestSucceed = false;
                    if (fn_WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL)) {
                        bSendRequestSucceed = true;
                    }
                    else {
                        // Query the proxy information from IE setting and set the proxy if any.
                        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
                        __stosb((uint8_t*)&proxyConfig, 0, sizeof(proxyConfig));
                        if (fn_WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig)) {
                            if (proxyConfig.lpszAutoConfigUrl != NULL) {
                                WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;
                                __stosb((uint8_t*)&autoProxyOptions, 0, sizeof(autoProxyOptions));
                                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_CONFIG_URL;
                                autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP;
                                autoProxyOptions.lpszAutoConfigUrl = proxyConfig.lpszAutoConfigUrl;
                                autoProxyOptions.fAutoLogonIfChallenged = TRUE;
                                autoProxyOptions.dwReserved = 0;
                                autoProxyOptions.lpvReserved = NULL;

                                WINHTTP_PROXY_INFO proxyInfo;
                                __stosb((uint8_t*)&proxyInfo, 0, sizeof(proxyInfo));

                                if (fn_WinHttpGetProxyForUrl(_sessionHandle, _requestURL.toWideCharPointer(), &autoProxyOptions, &proxyInfo)) {
                                    if (fn_WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo))) {
                                        if (fn_WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL)) {
                                            bSendRequestSucceed = true;
                                        }
                                    }
                                    if (proxyInfo.lpszProxy != NULL) {
                                        fn_GlobalFree(proxyInfo.lpszProxy);
                                    }
                                    if (proxyInfo.lpszProxyBypass != NULL) {
                                        fn_GlobalFree(proxyInfo.lpszProxyBypass);
                                    }
                                }
                                else {
                                    _dwLastError = fn_GetLastError();
                                }
                            }
                            else if (proxyConfig.lpszProxy != NULL) {
                                WINHTTP_PROXY_INFO proxyInfo;
                                __stosb((uint8_t*)&proxyInfo, 0, sizeof(proxyInfo));
                                proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                                wchar_t szProxy[MAX_PATH];

                                __stosb((uint8_t*)szProxy, 0, sizeof(szProxy));
                                fn_lstrcpynW(szProxy, proxyConfig.lpszProxy, MAX_PATH);
                                proxyInfo.lpszProxy = szProxy;

                                if (proxyConfig.lpszProxyBypass != 0) {
                                    wchar_t szProxyBypass[MAX_PATH];
                                    __stosb((uint8_t*)szProxyBypass, 0, sizeof(szProxyBypass));
                                    
                                    fn_lstrcpynW(szProxyBypass, proxyConfig.lpszProxyBypass, MAX_PATH);
                                    proxyInfo.lpszProxyBypass = szProxyBypass;
                                }

                                if (!fn_WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo))) {
                                    _dwLastError = fn_GetLastError();
                                }
                            }

                            if (proxyConfig.lpszAutoConfigUrl != 0) {
                                fn_GlobalFree(proxyConfig.lpszAutoConfigUrl);
                            }
                            if (proxyConfig.lpszProxy != 0) {
                                fn_GlobalFree(proxyConfig.lpszProxy);
                            }
                            if (proxyConfig.lpszProxyBypass != 0) {
                                fn_GlobalFree(proxyConfig.lpszProxyBypass);
                            }
                        }
                        else {
                            _dwLastError = fn_GetLastError();
                        }
                    }
                    if (bSendRequestSucceed) {
                        if (m_pDataToSend != 0) {
                            DWORD dwWritten = 0;
                            if (!fn_WinHttpWriteData(hRequest, m_pDataToSend, m_dataToSendSize, &dwWritten)) {
                                _dwLastError = fn_GetLastError();
                            }
                        }
                        if (fn_WinHttpReceiveResponse(hRequest, NULL)) {
                            DWORD dwSize = 0;
                            BOOL bResult = FALSE;
                            bResult = fn_WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, WINHTTP_NO_HEADER_INDEX);
                            if (bResult || (!bResult && (fn_GetLastError() == ERROR_INSUFFICIENT_BUFFER))) {
                                wchar_t* szHeader = (wchar_t*)fn_memalloc(sizeof(wchar_t) * dwSize);
                                if (szHeader != NULL) {
                                    __stosb((uint8_t*)szHeader, 0, dwSize * sizeof(wchar_t));
                                    if (fn_WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, szHeader, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
                                        _responseHeader = szHeader;
                                        int index = _responseHeader.indexOf("Content-Length: ");
                                        if (index != -1) {
                                            _responseByteCount = (uint32_t)_responseHeader.substring(index + 16).getLargeIntValue();
                                        }
                                    }
                                    fn_memfree(szHeader);
                                }
                            }

                            dwSize = 0;
                            bResult = fn_WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, WINHTTP_NO_HEADER_INDEX);
                            if (bResult || (!bResult && (fn_GetLastError() == ERROR_INSUFFICIENT_BUFFER))) {
                                wchar_t* szStatusCode = (wchar_t*)fn_memalloc(sizeof(wchar_t) * dwSize);
                                if (szStatusCode != NULL) {
                                    __stosb((uint8_t*)szStatusCode, 0, dwSize* sizeof(wchar_t));
                                    if (fn_WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, szStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
                                        m_statusCode = szStatusCode;
                                    }
                                    fn_memfree(szStatusCode);
                                }
                            }

                            uint32_t iMaxBufferSize = INT_BUFFERSIZE;
                            uint32_t iCurrentBufferSize = 0;
                            if (_pResponse != 0) {
                                fn_memfree(_pResponse);
                                _pResponse = 0;
                            }
                            _pResponse = (uint8_t*)fn_memalloc(iMaxBufferSize);
                            if (_pResponse == 0) {
                                bRetVal = false;
                                break;
                            }
                            __stosb(_pResponse, 0, iMaxBufferSize);
                            do {
                                dwSize = 0;
                                if (fn_WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                    uint8_t* pResponse = (uint8_t*)fn_memalloc(dwSize + 1);
                                    if (pResponse != NULL) {
                                        __stosb(pResponse, 0, (dwSize + 1)*sizeof(uint8_t));
                                        DWORD dwRead = 0;
                                        if (fn_WinHttpReadData(hRequest, pResponse, dwSize, &dwRead)) {
                                            if (dwRead + iCurrentBufferSize > iMaxBufferSize) {
                                                uint8_t* pOldBuffer = _pResponse;
                                                _pResponse = (uint8_t*)fn_memalloc(iMaxBufferSize * 2);
                                                if (_pResponse == 0) {
                                                    _pResponse = pOldBuffer;
                                                    bRetVal = false;
                                                    break;
                                                }
                                                iMaxBufferSize *= 2;
                                                __stosb(_pResponse, 0, iMaxBufferSize);
                                                __movsb(_pResponse, pOldBuffer, iCurrentBufferSize);
                                                fn_memfree(pOldBuffer);
                                            }
                                            __movsb(_pResponse + iCurrentBufferSize, pResponse, dwRead);
                                            iCurrentBufferSize += dwRead;
                                        }
                                        fn_memfree(pResponse);
                                    }
                                }
                                else {
                                    _dwLastError = fn_GetLastError();
                                }
                            } while (dwSize > 0);
                            m_responseByteCountReceived = iCurrentBufferSize;
                            bGetReponseSucceed = true;
                        }
                        else {
                            _dwLastError = fn_GetLastError();
                        }
                    }
                } // while
                if (!bGetReponseSucceed) {
                    bRetVal = false;
                }

                fn_WinHttpCloseHandle(hRequest);
            }
            fn_WinHttpCloseHandle(hConnect);
        }
    }

    fn_memfree(szHostName);
    fn_memfree(szURLPath);

    return bRetVal;
}

String HttpClient::GetResponseHeader()
{
    return _responseHeader;
}

String HttpClient::GetRequestHost()
{
    return _requestHost;
}

bool HttpClient::setAdditionalDataToSend(uint8_t *data, uint32_t dataSize)
{
    if (data == 0 || dataSize < 0) {
        return false;
    }

    if (m_pDataToSend != 0) {
        fn_memfree(m_pDataToSend);
    }
    m_pDataToSend = (uint8_t*)fn_memalloc(dataSize);
    if (m_pDataToSend != 0) {
        __movsb(m_pDataToSend, data, dataSize);
        m_dataToSendSize = dataSize;
        return true;
    }

    return false;
}

void HttpClient::setAdditionalRequestHeaders(const String& additionalHeaders)
{
    _additionalHeaders = additionalHeaders;
}

const uint8_t *HttpClient::GetRawResponseContent(void)
{
    return _pResponse;
}

uint32_t HttpClient::GetRawResponseContentLength(void)
{
    return _responseByteCount;
}

uint32_t HttpClient::GetRawResponseReceivedContentLength(void)
{
    return m_responseByteCountReceived;
}

DWORD HttpClient::GetLastError(void)
{
    return _dwLastError;
}

String HttpClient::GetResponseStatusCode(void)
{
    return m_statusCode;
}

void HttpClient::setHost(const String &host)
{
    _requestHost = host;
}

void HttpClient::setUserAgent(const String &userAgent)
{
    _userAgent = userAgent;
}

void HttpClient::setTimeouts(uint32_t resolveTimeout, uint32_t connectTimeout, uint32_t sendTimeout, uint32_t receiveTimeout)
{
    m_resolveTimeout = resolveTimeout;
    m_connectTimeout = connectTimeout;
    m_sendTimeout = sendTimeout;
    m_receiveTimeout = receiveTimeout;
}

}
