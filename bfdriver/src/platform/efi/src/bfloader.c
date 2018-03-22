
#include "bfefi.h"
#include "bflib.h"
#include "bfloader.h"
#include "bfmodule.h"
#include "start.h"
#include "common.h"

EFI_HANDLE this_image_h;
EFI_MP_SERVICES_PROTOCOL* g_mp_services;

EFI_STATUS efi_main(EFI_HANDLE image_in, EFI_SYSTEM_TABLE* st_in)
{   

    bf_init_lib(image_in, st_in);

    Print(L"=======================================\n");
    Print(L" ___                __ _           _   \n");
    Print(L"| _ ) __ _ _ _ ___ / _| |__ _ _ _ | |__\n");
    Print(L"| _ \\/ _` | '_/ -_)  _| / _` | ' \\| / /\n");
    Print(L"|___/\\__,_|_| \\___|_| |_\\__,_|_||_|_\\_\\\n");
    Print(L"     EFI Loader  \n");
    Print(L"=======================================\n");

    EFI_STATUS status;

    EFI_GUID gEfiMpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
    status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid,
                                 NULL,
                                 (VOID **)&g_mp_services);
    CHERROR(status);

    Print(L"Adding hypervisor modules..\n");
    add_hypervisor_modules();

    Print(L"Loading modules..\n");
    int64_t ret = common_load_vmm();
    if (ret < 0) {
        Print(L"common_load_vmm returned %a\n", ec_to_str(ret));
        goto fail;
    }

    //bf_start_by_interactive();
    bf_start_by_startupallaps();
    //bf_start_by_switchbsp();

    Print(L"Booting next image in BootOrder.\n");
    console_get_keystroke(NULL);

    bf_boot_next_by_order();

    return EFI_NOT_FOUND;

fail:

    console_get_keystroke(NULL);

    return status;

}
