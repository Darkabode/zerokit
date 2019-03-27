#ifndef __MOD_PROTECTORAPI_H_
#define __MOD_PROTECTORAPI_H_

#define FN_COUNT 100


//
// The following structures is returned on an IOCTL_DISK_GET_DRIVE_LAYOUT
// request and given as input to an IOCTL_DISK_SET_DRIVE_LAYOUT request.
//

typedef struct _DRIVE_LAYOUT_INFORMATION {
    ulong_t PartitionCount;
    ulong_t Signature;
    PARTITION_INFORMATION PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION, *PDRIVE_LAYOUT_INFORMATION;

typedef struct tag_QUERY_DIRECTORY
{
    ulong_t Length;
    PUNICODE_STRING FileName;
    FILE_INFORMATION_CLASS FileInformationClass;
    ulong_t FileIndex;
} QUERY_DIRECTORY, *PQUERY_DIRECTORY;


typedef struct _COMPL_ROUTINE_TRACE
{
    ulong_t dId;
    PIO_COMPLETION_ROUTINE CompletionRoutine;
    pvoid_t Context;
    uint8_t Control;
    PIO_STACK_LOCATION pIrpSt;
    FILE_INFORMATION_CLASS FileInformationClass;
    pvoid_t Buffer;
    ulong_t uBuffLen;
} COMPL_ROUTINE_TRACE, *PCOMPL_ROUTINE_TRACE;

typedef struct SYSTEM_MODULE_INFORMATION_EX
{
    ulong_t ModulesCount;
    SYSTEM_MODULE_INFORMATION Modules[1];
} SYSTEM_MODULE_INFORMATION_EX, *PSYSTEM_MODULE_INFORMATION_EX;

typedef struct _sectors_region
{
    LIST_ENTRY;
    pdisk_info_t pDiskInfo;
    PDEVICE_OBJECT pDeviceObject;
    uint64_t startOffset;   // Смещение в байтах до первого сектора.
    uint32_t size;          // Размер региона в байтах.
    uint8_t* pFakeData;     // Сожержимое фейковых секторов, которые подсовываются при чтении с диска и которые реально перезаписываются при записи на диск.
    uint8_t* pRealData;     // Данные, которые реально должны записываться на диск при записи.
    bool_t needProtect;     // Флаг, указывающий на необходимость протекции региона.
    bool_t fakeAllocated;   // Флаг, указывающий на необходимость освобождения памяти под фейковые данные.
} sectors_region_t, *psectors_region_t;

#define MAX_FILE_LEN 65

typedef struct _disk_device_list
{
    LIST_ENTRY list;
    PDEVICE_OBJECT pDevice;
    ulong_t uBytesPerSect;    
} disk_device_list_t, *pdisk_device_list_t;

#define MAX_FILES 25
#define MAX_DISKS 25

// typedef enum {
//     Par_FAT32,
//     Par_NTFS,
//     Par_Unknown
// } partition_e;

#define FN_COUNT 100

#define SIZE_EXT_REGION 30

typedef struct _completion_context
{
    Fncommon_dio_rw_completion origFunc;
    pvoid_t pOrigContext;
    PDEVICE_OBJECT pDeviceObject;
    uint8_t* pData;
    uint32_t dataSize;
    uint64_t offset;
    PMDL pMdl;
    bool_t isMdlAllocated;
} completion_context_t, *pcompletion_context_t;

typedef void (*Fnprotector_critical_callback)(psplice_hooker_data_t pHookerData);
typedef NTSTATUS (*Fnprotector_common)(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
typedef NTSTATUS (*Fnprotector_irp_internal_devctl_hook)(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
typedef NTSTATUS (*Fnprotector_irp_devctl_hook)(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
typedef NTSTATUS (*Fnprotector_irp_completion_routine)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN pvoid_t Context);
typedef pcompletion_context_t (*Fnprotector_set_completion_routine)(PIRP pIrp, PIO_STACK_LOCATION pIrpStack, uint8_t* pData, uint32_t dataSize, uint64_t dataOffset);
typedef NTSTATUS (*Fnprotector_verify_write_sectors)(psectors_region_t pRegion, uint8_t* pData, uint32_t dataSize, uint64_t dataOffset);


typedef psectors_region_t (*Fnprotector_add_sector_space)(pdisk_info_t pDiskInfo, uint64_t sectorOffset, uint32_t size, uint64_t* pFakeRegionOffset, uint8_t* pFakeData);
typedef void (*Fnprotector_release)(psectors_region_t pRegion);

typedef void (*Fnzfs_wrapper)(pvoid_t ptr);


typedef struct _mod_protector_private
{
    Fnprotector_critical_callback fnprotector_critical_callback;
    Fnprotector_common fnprotector_common;
    Fnprotector_irp_internal_devctl_hook fnprotector_irp_internal_devctl_hook;
    Fnprotector_irp_devctl_hook fnprotector_irp_devctl_hook;


    Fnprotector_irp_completion_routine fnprotector_irp_completion_routine;

    FnMajorFunction majorFunctions[IRP_MJ_MAXIMUM_FUNCTION + 1];
    psplice_hooker_data_t pIrpInternalDevCtlHook;     // Хук обработчика IRP_MJ_SCSI уровня 2.
    psplice_hooker_data_t pIrpDevCtlHook;  // Хук на функцию DriverStartIo уровня 2.

    uint8_t* pModBase;

    sectors_region_t headSectorRegion;
    wchar_t* g_w_names;

//     wchar_t partMgr[16]; // "\PartMgr"
//     wchar_t hdVol1[24]; //  "\Device\HarddiskVolume1"
} mod_protector_private_t, *pmod_protector_private_t;

typedef void (*Fnprotector_shutdown_routine)();

typedef struct _mod_protector_block
{
    Fnprotector_shutdown_routine fnprotector_shutdown_routine;
    Fnprotector_add_sector_space fnprotector_add_sector_space;
    Fnprotector_release fnprotector_release;

    psectors_region_t hVBRRegion;
    psectors_region_t hZkRegion;
    psectors_region_t hConfRegion;

    mod_protector_private_t;
} mod_protector_block_t, *pmod_protector_block_t;

#endif // __MOD_PROTECTORAPI_H_
