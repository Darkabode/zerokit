#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <ndis.h>

#include "../../mod_shared/headers.h"

#if (_NON_STD_DRIVER == 0)

typedef struct _WP_GLOBALS
{
    void* ptr;
    PMDL pMDL;
} WP_GLOBALS;

#endif // (_NON_STD_DRIVER == 0)

#define _SOLID_DRIVER 1

#include "../../mod_fs/code/main.c"
#include "../../mod_protector/code/main.c"
#include "../../mod_tasks/code/main.c"
#include "../../mod_launcher/code/main.c"
#include "../../mod_network/code/main.c"
#include "../../mod_tcpip/code/main.c"
#include "../../mod_netcomm/code/main.c"
#include "../../mod_userio/code/main.c"
#include "../../mod_logic/code/main.c"

#include "../../mod_common/code/main.c"

typedef NTSTATUS (*Fnmod_commonEntry)(uintptr_t modBase, uintptr_t loadMode);

NTSTATUS GsDriverEntry(uintptr_t driverBase, PUCHAR dummyVal/*IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath*/)
{
    FnmodEntryPoint fnmodEntryPoint = mod_commonEntry;

#if _NON_STD_DRIVER && !defined(_WIN64)
    (PUCHAR)fnmodEntryPoint += driverBase;
#endif

#ifdef _WIN64
    __debugbreak();
#else
    __asm int 3
#endif // _WIN64

    return fnmodEntryPoint(driverBase, NULL);
}
