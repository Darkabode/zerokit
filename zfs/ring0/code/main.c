#include <Ntifs.h>
#include <Ntstrsafe.h>

NTSTATUS common_strcpy_s(char* pszDest, size_t cchDest, const char* pszSrc)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchToCopy = NTSTRSAFE_MAX_LENGTH;

    if ((cchDest == 0) || (cchDest > NTSTRSAFE_MAX_CCH)) {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status))	{
        while (cchDest && cchToCopy && (*pszSrc != '\0')) {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchToCopy--;
        }

        if (cchDest == 0) {
            // we are going to truncate pszDest
            pszDest--;

            status = STATUS_BUFFER_OVERFLOW;
        }

        *pszDest = '\0';
    }

    return status;
}
#define FN_STRCPY_S common_strcpy_s

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/ring0/kernel_native.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/pe.h"
#include "../../../shared_code/aes.h"

#include "../../../shared_code/aes.c"

typedef struct ServiceDescriptorEntry
{
    PUCHAR KiServiceTable;
    PULONG CounterBaseTable;
    ULONG nSystemCalls;
    PULONG KiArgumentTable;
} SDE, *PSDE;

extern PSDE KeServiceDescriptorTable;
PSDE pKeServiceDescriptorTable;
PUCHAR pKiServiceTable;

#define HDE64_TABLE g_hde64_table
#define HDE32_TABLE g_hde32_table
#include "../../../shared_code/ring0/hde.h"
#include "../../../shared_code/ring0/hde.c"
Fndissasm fndissasm;

#define FN_ZFS_CREATE_MUTEX(pMutex) KeInitializeMutex(pMutex, FALSE)
#define FN_ZFS_LOCK_MUTEX(pMutex) KeWaitForSingleObject(pMutex, Executive, KernelMode, FALSE, NULL)
#define FN_ZFS_UNLOCK_MUTEX(pMutex) KeReleaseMutex(pMutex, FALSE);
#define FN_ZFS_DESTROY_MUTEX(pMutex)
#define FN_ZFS_THREAD_YIELD()

#include "../../zfs/time.h"

int	zfs_get_system_time(zfs_system_time_t* pTime)
{
    TIME_FIELDS tf;
    LARGE_INTEGER systemTm, localTm;
    USE_GLOBAL_BLOCK

#ifdef _WIN64
    KeQuerySystemTime(&systemTm);
#else
    KeQuerySystemTime(&systemTm);
#endif // _WIN64
    ExSystemTimeToLocalTime(&systemTm, &localTm);
    RtlTimeToTimeFields(&localTm, &tf);

    pTime->Hour = tf.Hour;
    pTime->Minute = tf.Minute;
    pTime->Second = tf.Second;
    pTime->Day = tf.Day;
    pTime->Month = tf.Month;
    pTime->Year = tf.Year;

    return 0;
}

#define FN_ZFS_GET_SYSTEM_TIME zfs_get_system_time

#include "../../zfs/zfs.c"

VOID common_thread(PVOID arg)
{
    int err = ERR_OK;
    pzfs_io_manager_t pIoman;
    zfs_file_t* ffFile;

    do {
        pIoman = zfs_create_io_manager(16 * BDEV_BLOCK_SIZE);

        if (pIoman) {
            if (zfs_open_device(pIoman, "\\??\\C:\\test.img", (uint8_t*)"qweasdzxc", 9) != ERR_OK) {
                bdev_create("\\??\\C:\\test.img", 1024 * 1024 * 1024 * 3);

                if (zfs_open_device(pIoman, "\\??\\C:\\test.img", (uint8_t*)"qweasdzxc", 9) != ERR_OK) {
                    zfs_destroy_io_manager(pIoman);
                    break;
                }

                zfs_format(pIoman);
            }

            if (zfs_mount(pIoman) != ERR_OK) {
                zfs_close_device(pIoman);
                zfs_destroy_io_manager(pIoman);
                break;
            }

            zfs_remove_dir(pIoman, "\\test_dir");

            err = zfs_mkdir(pIoman, "\\test_dir");

            if (!ZFS_isERR(err)) {
                ffFile = zfs_open(pIoman, "\\test_dir\\test.txt", ZFS_MODE_CREATE | ZFS_MODE_WRITE, &err);

                if (err == ERR_OK) {
                    zfs_write(ffFile, 1, 22, "Hello, Zerokit World!");
                    zfs_close(ffFile);

                    ffFile = zfs_open(pIoman, "\\test_dir\\test.txt", ZFS_MODE_READ, &err);

                    if (err == ERR_OK) {
                        char buff[32];
                        zfs_read(ffFile, 1, 22, buff);
#if RING3
                        printf("%s\n", buff);
#else
                        DbgPrint("%s\n", buff);
#endif // RING3
                        zfs_close(ffFile);
                    }

                    zfs_remove_file(pIoman, "\\test_dir\\test.txt");
                    ffFile = zfs_open(pIoman, "\\test_dir\\test.txt", ZFS_MODE_READ, &err);
                    if (ZFS_isERR(err)) {
#if RING3
                        printf("\\test_dir\\test.txt successfully deleted!\n");
#else
                        DbgPrint("\\test_dir\\test.txt successfully deleted!\n");
#endif // RING3
                    }

                }
            }
#if RING3
            printf("FS free space: %lld Mb\n", zfs_get_free_size(pIoman, &err) / 1024 / 1024);
#else
            DbgPrint("FS free space: %d Mb\n", zfs_get_free_size(pIoman, &err) / 1024 / 1024);
#endif // RING3
            zfs_unmount(pIoman);
            zfs_close_device(pIoman);
            zfs_destroy_io_manager(pIoman);
        }
    } while (0);

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
    KdPrint(("Driver unloaded\n"));

    return;
}

NTSTATUS GsDriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pusRegistryPath)
{
	HANDLE hThread = NULL;
	OBJECT_ATTRIBUTES objAttrs;
    zfs_system_time_t zfsTime;

	// Инициализируем дизаасемблер
#ifdef _WIN64
	fndissasm = (Fndissasm)dissasm64;
#else
	fndissasm = (Fndissasm)dissasm32;
#endif

	// Ищем адреса KeServiceDescriptorTable и KiServiceTable.
#ifdef _WIN64
	{
		PUCHAR pFunc = (PUCHAR)pe_find_export_by_hash(find_module_base_by_inner_ptr((PUCHAR)RtlInitUnicodeString), KeAddSystemServiceTable_Hash);

		if (pFunc != NULL) {
			dissasm_info_t hde;
			ULONG i;
			PUCHAR ptr = pFunc;

			for (i = 0; i < 0x40; ) {
				fndissasm(ptr, &hde);

				//ud_set_input_buffer(&ud_obj, Inst, MAX_INST_LEN);

				/*
					Check for the following code

					nt!KeAddSystemServiceTable:
					fffff800`012471c0 448b542428         mov     r10d,dword ptr [rsp+28h]
					fffff800`012471c5 4183fa01           cmp     r10d,1
					fffff800`012471c9 0f871ab70c00       ja      nt!KeAddSystemServiceTable+0x78
					fffff800`012471cf 498bc2             mov     rax,r10
					fffff800`012471d2 4c8d1d278edbff     lea     r11,0xfffff800`01000000
					fffff800`012471d9 48c1e005           shl     rax,5
					fffff800`012471dd 4a83bc1880bb170000 cmp     qword ptr [rax+r11+17BB80h],0
					fffff800`012471e6 0f85fdb60c00       jne     nt!KeAddSystemServiceTable+0x78
				*/

				if ((*(PULONG)ptr & 0x00ffffff) == 0x1d8d4c && (*(PUSHORT)(ptr + 0x0b) == 0x834b || *(PUSHORT)(ptr + 0x0b) == 0x834a)) {
					// calculate nt!KeServiceDescriptorTableAddress
					LARGE_INTEGER Addr;
					Addr.QuadPart = (ULONGLONG)ptr + hde.len;
					Addr.LowPart += *(PULONG)(ptr + 0x03) + *(PULONG)(ptr + 0x0f);

					pKeServiceDescriptorTable = (PVOID)Addr.QuadPart;

					break;
				}

				ptr += hde.len;
			}
		}
	}

	if (pKeServiceDescriptorTable != NULL)
		pKiServiceTable = pKeServiceDescriptorTable->KiServiceTable;

#else
	pKeServiceDescriptorTable = KeServiceDescriptorTable;
	pKiServiceTable = pKeServiceDescriptorTable->KiServiceTable;
#endif

    zfs_get_system_time(&zfsTime);

    pDriverObject->DriverUnload = DriverUnload;

	InitializeObjectAttributes(&objAttrs, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &objAttrs, 0, 0, (PKSTART_ROUTINE)common_thread, NULL);

	return STATUS_SUCCESS;
}
