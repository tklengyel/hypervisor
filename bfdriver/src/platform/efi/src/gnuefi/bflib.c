
// gnu-efi wrappers 

#include "bfefi.h"
#include "bfloader.h"


VOID bf_init_lib(EFI_HANDLE hnd, EFI_SYSTEM_TABLE* systab)
{
    //gST = systab;
    //gBS = systab->BootServices;
    //gRT = systab->RuntimeServices;
    InitializeLib(hnd, systab);
    this_image_h = hnd;
}

UINTN bf_num_cpus()
{
    EFI_STATUS status;
    UINTN cpus;
    UINTN ecpus;
    EFI_MP_SERVICES_PROTOCOL* mp_services;

    EFI_GUID gEfiMpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
    status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid,
                                 NULL,
                                 (VOID **)&mp_services);
    CHERROR(status);

    status = g_mp_services->GetNumberOfProcessors(g_mp_services,
             &cpus,
             &ecpus);
    CHERROR(status);

    if (cpus != ecpus)
    {
        Print(L"Warning: disabled cpus present\n");
    }
    return ecpus;

fail:
    return 0;
}

VOID bf_dump_hex(UINTN indent, UINTN offset, UINTN size, VOID* ptr)
{
    DumpHex(indent, offset, size, ptr);
}

BOOLEAN bf_match_device_paths(EFI_DEVICE_PATH* multi, EFI_DEVICE_PATH* single)
{
    return LibMatchDevicePaths(multi, single);
}


VOID* bf_allocate_runtime_zero_pool(UINTN size)
{
    EFI_STATUS status;
    VOID* ret = NULL;
    status = gBS->AllocatePool(EfiRuntimeServicesData,
                               size,
                               &ret);

    if (!EFI_ERROR(status))
    {
        gBS->SetMem(ret, size, 0);
    }

    return ret;
}

VOID* bf_allocate_zero_pool(UINTN size)
{
    return AllocateZeroPool(size);
}

VOID* bf_get_variable(CHAR16* name, EFI_GUID* guid, UINTN* size)
{
    return LibGetVariableAndSize(name, guid, size);
}

VOID bf_free_pool(VOID* ptr)
{
    FreePool(ptr);
}