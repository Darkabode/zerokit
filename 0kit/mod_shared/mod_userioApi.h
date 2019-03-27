#ifndef __MOD_USERIOAPI_H_
#define __MOD_USERIOAPI_H_

#define NUMBER_OF_FUNCS 23

#include "..\..\mod_shared\userio.h"
#include <ntddkbd.h>

typedef uint32_t (*Fnuserio_allocate_handle)(uintptr_t zfsHandle);
typedef uintptr_t (*Fnuserio_get_zfs_handle)(uint32_t handle);
typedef void (*Fnuserio_free_zfs_handle)(uint32_t handle);

typedef NTSTATUS (*Fnuserio_null_irp_ioctl_hook)(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

typedef struct _client_entry
{
    LIST_ENTRY;
    uint8_t clientId[16/*CLIENT_ID_SIZE*/];
    char clientModulePath[4 * ZFS_MAX_FILENAME];
    char clientPath[2 * ZFS_MAX_FILENAME];
    uint32_t clientPathLen;
    int refCount; // Счётчик ссылок. После создания объекта должен быть равен 1.
} client_entry_t, *pclient_entry_t;

typedef pclient_entry_t (*Fnuserio_add_client)(char* clientModule);
typedef void (*Fnuserio_remove_client)(char* clientModule);
typedef pclient_entry_t (*Fnuserio_find_client_id)(pfile_packet_t pFilePacket);


typedef VOID (*FnKeyboardClassServiceCallbackHook)(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);
typedef void (*Fnuserio_keyboard_hook_internal)(int needHook);

typedef struct _mod_userio_private
{
    Fnuserio_allocate_handle fnuserio_allocate_handle;
    Fnuserio_get_zfs_handle fnuserio_get_zfs_handle;
    Fnuserio_free_zfs_handle fnuserio_free_zfs_handle;
    Fnuserio_null_irp_ioctl_hook fnuserio_null_irp_ioctl_hook;

    FnKeyboardClassServiceCallbackHook fnKeyboardClassServiceCallbackHook;
    Fnuserio_keyboard_hook_internal fnuserio_keyboard_hook_internal;
    
    psplice_hooker_data_t pNullUnloadHook;
    psplice_hooker_data_t pKeybSrvCbHook;

    uint8_t* nullModuleBase;
    uint32_t nullModuleSize;
    PDEVICE_OBJECT pNullDevObject;
    pvoid_t pNullDriverUnload;
    pvoid_t pNullDriverDevIo;
    wchar_t sNullDev[16];
    
    Fnzfs_wrapper zfs_wrapper_FNs[NUMBER_OF_FUNCS];
    uint32_t zfs_wrapper_IDs[NUMBER_OF_FUNCS];
    uint32_t zfs_wrapper_CtrlCodes[NUMBER_OF_FUNCS];

    uintptr_t* pHandles;
    KSPIN_LOCK slHandles;

    client_entry_t clientsHead;
    
    uint8_t* pKeyboardClassServiceCallback;
    int keyboardBlocked;
} mod_userio_private_t, *pmod_userio_private_t;

typedef void (*Fnuserio_shutdown_routine)();

typedef struct _mod_userio_block
{
    Fnuserio_add_client fnuserio_add_client;
    Fnuserio_remove_client fnuserio_remove_client;
    Fnuserio_find_client_id fnuserio_find_client_id;

    Fnuserio_shutdown_routine fnuserio_shutdown_routine;
	mod_userio_private_t;
} mod_userio_block_t, *pmod_userio_block_t;

#endif // __MOD_USERIOAPI_H_
