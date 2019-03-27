#define IOCTL_STORAGE_BASE 0x0000002d
#define IOCTL_DISK_BASE 0x00000007


#define IOCTL_STORAGE_QUERY_PROPERTY \
CTL_CODE(IOCTL_STORAGE_BASE, 0x0500,METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SMART_RCV_DRIVE_DATA \
CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define ATA_IDENTIFY_DEVICE 0xec

#pragma pack(1)

typedef struct ServiceDescriptorEntry {
    unsigned int *ServiceTableBase;
    unsigned int *ServiceCounterTableBase; //Used only in checked build
    unsigned int NumberOfServices;
    unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;

struct ata_identify_device {
  unsigned short words000_009[10];
  unsigned char  serial_no[20];
  unsigned short words020_022[3];
  unsigned char  fw_rev[8];
  unsigned char  model[40];
  unsigned short words047_079[33];
  unsigned short major_rev_num;
  unsigned short minor_rev_num;
  unsigned short command_set_1;
  unsigned short command_set_2;
  unsigned short command_set_extension;
  unsigned short cfs_enable_1;
  unsigned short word086;
  unsigned short csf_default;
  unsigned short words088_255[168];
};

typedef enum _STORAGE_BUS_TYPE {
    BusTypeUnknown = 0x00,
    BusTypeScsi,
    BusTypeAtapi,
    BusTypeAta,
    BusType1394,
    BusTypeSsa,
    BusTypeFibre,
    BusTypeUsb,
    BusTypeRAID,
    BusTypeMaxReserved = 0x7F
} STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

// retrieve the storage device descriptor data for a device.
typedef struct _STORAGE_DEVICE_DESCRIPTOR {
  ULONG  Version;
  ULONG  Size;
  UCHAR  DeviceType;
  UCHAR  DeviceTypeModifier;
  BOOLEAN  RemovableMedia;
  BOOLEAN  CommandQueueing;
  ULONG  VendorIdOffset;
  ULONG  ProductIdOffset;
  ULONG  ProductRevisionOffset;
  ULONG  SerialNumberOffset;
  STORAGE_BUS_TYPE  BusType;
  ULONG  RawPropertiesLength;
  UCHAR  RawDeviceProperties[1];

} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;


#pragma pack()

#pragma pack(1)

typedef struct _GETVERSIONOUTPARAMS {
    UCHAR  bVersion;
    UCHAR  bRevision;
    UCHAR  bReserved;
    UCHAR  bIDEDeviceMap;
    ULONG  fCapabilities;
    ULONG  dwReserved[4];
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;


typedef struct _IDEREGS {
    UCHAR  bFeaturesReg;
    UCHAR  bSectorCountReg;
    UCHAR  bSectorNumberReg;
    UCHAR  bCylLowReg;
    UCHAR  bCylHighReg;
    UCHAR  bDriveHeadReg;
    UCHAR  bCommandReg;
    UCHAR  bReserved;
} IDEREGS, *PIDEREGS, *LPIDEREGS;

typedef struct _SENDCMDINPARAMS {
    ULONG  cBufferSize;
    IDEREGS  irDriveRegs;
    UCHAR  bDriveNumber;
    UCHAR  bReserved[3];
    ULONG  dwReserved[4];
    UCHAR  bBuffer[1];
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;


typedef struct _DRIVERSTATUS {
    UCHAR  bDriverError;
    UCHAR  bIDEError;
    UCHAR  bReserved[2];
    ULONG  dwReserved[2];
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

typedef struct _SENDCMDOUTPARAMS {
    ULONG  cBufferSize;
    DRIVERSTATUS  DriverStatus;
    UCHAR  bBuffer[1];
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;


#pragma pack()


__declspec(dllimport) ServiceDescriptorTableEntry_t
KeServiceDescriptorTable;


// smartmontools code to deal with non big endian system

// Copies n bytes (or n-1 if n is odd) from in to out, but swaps adjacents
// bytes.
void swapbytes(char *out, const char *in, size_t n)
{
  size_t i;

  for (i = 0; i < n; i += 2) {
    out[i]   = in[i+1];
    out[i+1] = in[i];
  }
}

// Copies in to out, but removes leading and trailing whitespace.
void trim(char *out, const char *in)
{
  int i, first, last;

  // Find the first non-space character (maybe none).
  first = -1;
  for (i = 0; in[i]; i++)
    if (!isspace((int)in[i])) {
      first = i;
      break;
    }

  if (first == -1) {
    // There are no non-space characters.
    out[0] = '\0';
    return;
  }

  // Find the last non-space character.
  for (i = strlen(in)-1; i >= first && isspace((int)in[i]); i--)
    ;
  last = i;

  strncpy(out, in+first, last-first+1);
  out[last-first+1] = '\0';
}

// Convenience function for formatting strings from ata_identify_device
void formatdriveidstring(char *out, const char *in, int n)
{
  char tmp[65];
  n = n > 64 ? 64 : n;
  swapbytes(tmp, in, n);
  tmp[n] = '\0';
  trim(out, tmp);
}



NTSYSAPI
NTSTATUS
NTAPI ZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
);


typedef NTSTATUS (*ZWDEVICEIOCONTROLFILE)(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
);


ZWDEVICEIOCONTROLFILE OldZwDeviceIoControlFile;

NTSTATUS NewZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength)
{

    PSTORAGE_DEVICE_DESCRIPTOR output;
    PSENDCMDINPARAMS  cmdinput;
    PSENDCMDOUTPARAMS cmdoutput;
    struct ata_identify_device *hdid;

    NTSTATUS rc;
    rc = ((ZWDEVICEIOCONTROLFILE)(OldZwDeviceIoControlFile)) (
             FileHandle,
             Event,
             ApcRoutine,
             ApcContext,
             IoStatusBlock,
             IoControlCode,
             InputBuffer,
             InputBufferLength,
             OutputBuffer,
             OutputBufferLength
         );



    if(IoControlCode != IOCTL_STORAGE_QUERY_PROPERTY && IoControlCode != SMART_RCV_DRIVE_DATA)
        return(rc);

    if(NT_SUCCESS(rc))
    {
        switch( IoControlCode )
        {
        case IOCTL_STORAGE_QUERY_PROPERTY:

            output = (PSTORAGE_DEVICE_DESCRIPTOR) OutputBuffer;
            if( output->SerialNumberOffset )
            {
                char* serialnum = (char*)output + output->SerialNumberOffset;
                formatdriveidstring(serialnum, "FAKE SERIAL", 40);
            }

            if( output->ProductIdOffset )
            {
                char* productid = (char*)output + output->ProductIdOffset;
                strncpy( productid, "STUPID PB", strlen(productid) );
            }

            if( output->VendorIdOffset )
            {
                char* vendorid = (char*)output + output->VendorIdOffset;
                strncpy( vendorid, "asdfghjkl", strlen(vendorid) );
            }

            break;

        case SMART_RCV_DRIVE_DATA:

            cmdinput  = (PSENDCMDINPARAMS)  InputBuffer;
            cmdoutput = (PSENDCMDOUTPARAMS) OutputBuffer;

            if (cmdinput->irDriveRegs.bCommandReg == ATA_IDENTIFY_DEVICE)
            {
                hdid = (struct ata_identify_device*) (cmdoutput->bBuffer);
                formatdriveidstring(hdid->model, "spoofed model!", 40 );
                formatdriveidstring( hdid->serial_no, "serial goes here", 20 );
            }
            break;
        }
    }

    return(rc);

}


NTSTATUS DriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest (Irp,IO_NO_INCREMENT);

    return Irp->IoStatus.Status;
}



void DriverUnload(IN PDRIVER_OBJECT DriverObject)
{

    _asm
    {
        CLI //dissable interrupt
        MOV EAX, CR0 //move CR0 register into EAX
        AND EAX, NOT 10000H //disable WP bit
        MOV CR0, EAX //write register back
    }

    (KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)ZwDeviceIoControlFile+1)])
    = (ULONG)OldZwDeviceIoControlFile;

    _asm

    {
        MOV EAX, CR0 //move CR0 register into EAX
        OR EAX, 10000H //enable WP bit
        MOV CR0, EAX //write register back
        STI //enable interrupt
    }
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDispatch;

    DriverObject->DriverUnload = DriverUnload;

    // save old system call locations

    OldZwDeviceIoControlFile = (ZWDEVICEIOCONTROLFILE)(KeServiceDescriptorTable.ServiceTableBase[
                                   *(PULONG)((PUCHAR)ZwDeviceIoControlFile+1)]);

    _asm
    {
        CLI //dissable interrupt
        MOV EAX, CR0 //move CR0 register into EAX
        AND EAX, NOT 10000H //disable WP bit
        MOV CR0, EAX //write register back
    }

    (KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)ZwDeviceIoControlFile+1)])
    = (ULONG)NewZwDeviceIoControlFile;

    _asm
    {
        MOV EAX, CR0 //move CR0 register into EAX
        OR EAX, 10000H //enable WP bit
        MOV CR0, EAX //write register back
        STI //enable interrupt
    }

    return STATUS_SUCCESS;
}