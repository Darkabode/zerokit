#ifndef __OVERLORD_EXT_H_
#define __OVERLORD_EXT_H_

typedef enum _overlord_request_id
{
    ORID_NETWORK_INFO,
    ORID_HIPS_INFO,
    ORID_NETCOMM_CHECK,
    ORID_NETCOMM_TRANSACT,
    ORID_NETCOMM_GET_NTP,
    ORIG_NETCOMM_GETHOSTBYNAME,
    ORIG_SHELL_GET_NAME,
    
} overlord_request_id_e;


typedef enum _arp_entry_type
{
    Other = 1,
    Invalid = 2,
    Dynamic = 3,
    Static = 4
} arp_entry_type_e;

#pragma pack(push, 1)

typedef struct _overlord_request_info
{
    overlord_request_id_e orid;
    uint8_t* moduleBase;
    uint32_t moduleSize;
    uint8_t* outData;
    uint32_t outSize;
    uint32_t inSize;
    uint8_t inData[1];
} overlord_request_info_t, *poverlord_request_info_t;

typedef struct _arp_entry
{
    uint32_t ip;
    uint8_t mac[/*MAX_ADAPTER_ADDRESS_LENGTH*/8]; // mac address
    arp_entry_type_e entryType;
} arp_entry_t, *parp_entry_t;

typedef struct _adapter_entry
{
    uint32_t index; // the index of the adapter

    char name[/*MAX_ADAPTER_NAME_LENGTH*/256 + 4]; // name (GUID) of the adapter
    char description[/*MAX_ADAPTER_DESCRIPTION_LENGTH*/128 + 4]; // description (human readable name)
    uint8_t mac[/*MAX_ADAPTER_ADDRESS_LENGTH*/8]; // mac address

    uint32_t ip; // IP address as dotted decimal
    uint32_t subnet; // subnet mask
    uint32_t gateway; // gateway

    bool_t dhcpEnabled;
    uint32_t dhcpServer;

    uint32_t dnsServers[7];

    uint32_t arpEntryOffset;
    uint32_t arpEntriesCount;
} adapter_entry_t, *padapter_entry_t;

typedef struct _http_request
{
    char server[200];
    uint16_t port;
    uint32_t httpSize;
    char http[1];
} http_request_t, *phttp_request_t;

#pragma pack(pop)

#endif // __OVERLORD_EXT_H_
