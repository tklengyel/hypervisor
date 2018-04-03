
#include "bfefi.h"
#include "bflib.h"
#include "bfloader.h"

EFI_STATUS bf_boot_next_by_order()
{
    EFI_GUID global_guid = EFI_GLOBAL_VARIABLE;

    EFI_STATUS status;
    EFI_LOADED_IMAGE* li;
    status = gBS->HandleProtocol(this_image_h,
                                 &gEfiLoadedImageProtocolGuid,
                                 (VOID**)&li);
    //bf_dump_hex(0,0,li->LoadOptionsSize,li->LoadOptions);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to fetch loaded image information.\n");
        return status;
    }

    // Print(L"size: %u\n", li->Revision);
    // Print(L"addr: %x\n", li->LoadOptions);

    EFI_DEVICE_PATH* dev = DevicePathFromHandle(li->DeviceHandle);
    if (!dev)
    {
        Print(L"Unable to get boot device path.\n");
        return EFI_NOT_FOUND;
    }

    EFI_DEVICE_PATH* loaded = AppendDevicePath(dev, li->FilePath);
    if (!loaded)
    {
        Print(L"Unable to assemble image device path.\n");
        return EFI_NOT_FOUND;
    }
    Print(DevicePathToStr(loaded));

    VOID* order = NULL;
    UINTN size;
    order = bf_get_variable(L"BootOrder",&global_guid, &size);
    if (!order)
    {
        Print(L"Unable to fetch BootOrder variable.\n");
        return EFI_NOT_FOUND;
    }

    UINTN count = size / 2;
    UINT16* next = order;

    UINTN bootnow = 0;
    while (count > 0)
    {
        CHAR16 buf[18] = {0};
        SPrint(buf, 18, L"Boot%04x", (UINT32)*next);
        Print(buf);
        Print(L"\n");
        VOID* boot = bf_get_variable(buf, &global_guid, &size);
        if (!boot)
        {
            Print(L"Unable to fetch variable ");
            Print(buf);
            Print(L".\n");
            return EFI_NOT_FOUND;
        }

        if (bootnow)
        {
            Print(L"Booting "); 
            Print(buf); 
            Print(L".\n"); 
            return EFI_SUCCESS;
        }

        VOID* desc = boot + sizeof(EFI_LOAD_OPTION);
        VOID* dp = desc + StrSize(desc);
        if (LibMatchDevicePaths(loaded, dp))
        {
            Print(L"Match!\n");
            Print(DevicePathToStr(dp));
            bootnow = 1;
        }

        //bf_dump_hex(0,0,DevicePathSize(dp),dp);

        next++;
        count--;
    }

    return EFI_NOT_FOUND;
}
