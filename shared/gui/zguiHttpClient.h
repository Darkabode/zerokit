#ifndef __ZGUI_HTTP_CLIENT_H_
#define __ZGUI_HTTP_CLIENT_H_

#include <winhttp.h>

namespace zgui
{

static const uint32_t INT_RETRYTIMES = 3;
static const int INT_BUFFERSIZE = 10240;    // Initial 10 KB temporary buffer, double if it is not enough.

class HttpClient
{
public:
    HttpClient(const String& url);
    ~HttpClient(void);

    inline bool sendHttpRequest(const String& httpVerb = "GET");
    inline String GetResponseHeader(void);
    inline String GetResponseStatusCode(void);
    inline String GetResponseLocation(void);
    inline String GetRequestHost(void);
    inline const uint8_t* GetRawResponseContent(void);
    inline uint32_t GetRawResponseContentLength(void);
    inline uint32_t GetRawResponseReceivedContentLength(void);
    void setHost(const String &host);
    bool setAdditionalDataToSend(uint8_t *data, uint32_t dataSize);
    void setAdditionalRequestHeaders(const String& additionalRequestHeaders);
    DWORD GetLastError(void);
    void setUserAgent(const String &userAgent);
    void setTimeouts(uint32_t resolveTimeout = 0, uint32_t connectTimeout = 60000, uint32_t sendTimeout = 30000, uint32_t receiveTimeout = 30000);

private:
    HttpClient(const HttpClient &other);
    HttpClient &operator=(const HttpClient &other);

    HINTERNET _sessionHandle;
    String _requestURL;
    String _requestHost;
    String _responseHeader;
    uint8_t* _pResponse;
    uint32_t m_responseByteCountReceived;   // Up to 4GB.
    uint32_t _responseByteCount;
    uint8_t *m_pDataToSend;
    uint32_t m_dataToSendSize;
    String _additionalHeaders;
    DWORD _dwLastError;
    String m_statusCode;
    String _userAgent;
    uint32_t m_resolveTimeout;
    uint32_t m_connectTimeout;
    uint32_t m_sendTimeout;
    uint32_t m_receiveTimeout;
};

}

#endif // __ZGUI_HTTP_CLIENT_H_
