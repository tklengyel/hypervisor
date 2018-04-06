#include "bfefi.h"
#include "bflib.h"
#include "bfloader.h"
#include "PeImage.h"

#define PAGE_SIZE (1ul<<12)
static EFI_STATUS (EFIAPI *entry_point) (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);

void bf_start_image(CHAR16* loader_name)
{
    EFI_LOADED_IMAGE *li = NULL, *loader_li;
    EFI_HANDLE handle = NULL;
    EFI_DEVICE_PATH *DevicePath = NULL;
    EFI_STATUS status;
    EFI_SYSTEM_TABLE *dST = NULL;

    status = gBS->HandleProtocol(this_image_h, &gEfiLoadedImageProtocolGuid, (VOID**)&li);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to fetch loaded image information.\n");
        return;
    }

    DevicePath = FileDevicePath(li->DeviceHandle, loader_name);

    status = gBS->LoadImage(0, this_image_h, DevicePath, NULL, 0, &handle);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to load %s.\n", loader_name);
        return;
    }

    status = gBS->HandleProtocol(handle, &gEfiLoadedImageProtocolGuid, (VOID**)&loader_li);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to fetch loaded image information for %s.\n", loader_name);
        return;
    }

    dST = AllocatePool(sizeof(EFI_SYSTEM_TABLE));
    CopyMem(dST, gST, sizeof(EFI_SYSTEM_TABLE));

    li->SystemTable = dST;

    gBS->StartImage(handle, 0, 0);

    /* not reached */
}

static EFI_STATUS read_header(void *data, uint32_t size, PE_COFF_LOADER_IMAGE_CONTEXT* context)
{
    EFI_IMAGE_DOS_HEADER *DosHdr = data;
    EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr = data;

    if (size < sizeof (PEHdr->Pe32))
        return EFI_UNSUPPORTED;
    if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE)
        return EFI_UNSUPPORTED;
    else
        PEHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((char *)data + DosHdr->e_lfanew);

    if ( PEHdr->Pe32Plus.OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC )
        return EFI_UNSUPPORTED;

    context->PEHdr = PEHdr;
    context->NumberOfSections = PEHdr->Pe32.FileHeader.NumberOfSections;
    context->NumberOfRvaAndSizes = PEHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
    context->SizeOfHeaders = PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;
    context->ImageSize = PEHdr->Pe32Plus.OptionalHeader.SizeOfImage;
    context->ImageAddress = PEHdr->Pe32Plus.OptionalHeader.ImageBase;
    context->EntryPoint = PEHdr->Pe32Plus.OptionalHeader.AddressOfEntryPoint;
    context->RelocDir = &PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];

    context->SectionAlignment = PEHdr->Pe32Plus.OptionalHeader.SectionAlignment ?: 4096;
    if ( context->SectionAlignment < PEHdr->Pe32Plus.OptionalHeader.FileAlignment )
        context->SectionAlignment = PEHdr->Pe32Plus.OptionalHeader.FileAlignment;

    context->FirstSection = (EFI_IMAGE_SECTION_HEADER *)((char *)PEHdr
                            + PEHdr->Pe32.FileHeader.SizeOfOptionalHeader
                            + sizeof(UINT32) + sizeof(EFI_IMAGE_FILE_HEADER));
    return EFI_SUCCESS;
}

static EFI_STATUS relocate(void *data, uint32_t size, PE_COFF_LOADER_IMAGE_CONTEXT *context, void **relocated_buffer)
{
    int i;
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS alloc_address;
    EFI_IMAGE_BASE_RELOCATION *reloc_base, *reloc_base_end;
    EFI_IMAGE_SECTION_HEADER *section = context->FirstSection, *reloc_section = NULL;
    UINT32 alloc_size = ALIGN_VALUE(context->ImageSize + context->SectionAlignment + size, PAGE_SIZE);

    status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderCode, alloc_size / PAGE_SIZE, &alloc_address);
    if (EFI_ERROR(status))
        return status;

    *relocated_buffer = (void *)ALIGN_VALUE((unsigned long)alloc_address, context->SectionAlignment);

    /* We copy the whole image as-is to allow the firmware to parse the image */
    CopyMem(*relocated_buffer, data, size);

    for (i = 0; i < context->NumberOfSections; i++, section++)
    {
        if (!CompareMem(section->Name, ".reloc\0\0", 8) && section->SizeOfRawData && section->Misc.VirtualSize)
            reloc_section = section;

        CopyMem(*relocated_buffer + section->VirtualAddress, data + section->PointerToRawData, section->SizeOfRawData);
    }

    if ( reloc_section )
    {
        UINT64 adjust = (UINTN)(*relocated_buffer) - context->ImageAddress;

        reloc_base = data + reloc_section->PointerToRawData;
        reloc_base_end = data + reloc_section->PointerToRawData + reloc_section->Misc.VirtualSize;

        while (reloc_base < reloc_base_end)
        {
            UINT16 *reloc = (UINT16 *) ((char *) reloc_base + sizeof (EFI_IMAGE_BASE_RELOCATION));
            UINT16 *reloc_end = (UINT16 *) ((char *) reloc_base + reloc_base->SizeOfBlock);
            char *fixup_base = *relocated_buffer + reloc_base->VirtualAddress;

            while (reloc < reloc_end)
            {
                char *fixup = fixup_base + (*reloc & 0xFFF);

                switch ((*reloc) >> 12) {
                case EFI_IMAGE_REL_BASED_HIGH:
                    *(UINT16*)fixup = (UINT16) (*(UINT16*)fixup + ((UINT16) ((UINT32) adjust >> 16)));
                    break;

                case EFI_IMAGE_REL_BASED_LOW:
                    *(UINT16*)fixup = (UINT16) (*(UINT16*)fixup + (UINT16) adjust);
                    break;

                case EFI_IMAGE_REL_BASED_HIGHLOW:
                    *(UINT32*)fixup = *(UINT32*)fixup + (UINT32) adjust;
                    break;

                case EFI_IMAGE_REL_BASED_DIR64:
                    *(UINT64*)fixup = *(UINT64*)fixup + (UINT64) adjust;
                    break;

                default:
                    break;

                };

                reloc++;
            }

            reloc_base = (EFI_IMAGE_BASE_RELOCATION *) reloc_end;
        }
    }

    return status;
}

static EFI_STATUS load_mmapped_file(void* addr, UINT32 size, EFI_HANDLE *handle)
{
    MEMMAP_DEVICE_PATH mempath[2];

    mempath[0].Header.Type = HARDWARE_DEVICE_PATH;
    mempath[0].Header.SubType = HW_MEMMAP_DP;
    mempath[0].Header.Length[0] = (UINT8)sizeof(MEMMAP_DEVICE_PATH);
    mempath[0].Header.Length[1] = (UINT8)(sizeof(MEMMAP_DEVICE_PATH)>> 8);
    mempath[0].MemoryType = EfiLoaderCode;
    mempath[0].StartingAddress = (UINTN)addr;
    mempath[0].EndingAddress = (UINTN)addr + size;

    mempath[1].Header.Type = END_DEVICE_PATH_TYPE;
    mempath[1].Header.SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    mempath[1].Header.Length[0] = (UINT8)sizeof(EFI_DEVICE_PATH);
    mempath[1].Header.Length[1] = (UINT8)(sizeof(EFI_DEVICE_PATH)>> 8);

    return gBS->LoadImage(0, this_image_h, (EFI_DEVICE_PATH*)&mempath, addr, size, handle);
}

/* Duplicate system table to allow ACPI fixups later */
static EFI_SYSTEM_TABLE *dup_st(EFI_HANDLE handle, EFI_LOADED_IMAGE *parent_li)
{

    EFI_LOADED_IMAGE *li = NULL;
    EFI_STATUS status;
    EFI_SYSTEM_TABLE *dST = AllocatePool(sizeof(EFI_SYSTEM_TABLE));

    if ( !dST )
        return NULL;

    /* Change systemtable in LOADED_IMAGE as well */
    status = gBS->HandleProtocol(handle, &gEfiLoadedImageProtocolGuid, (VOID**)&li);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to fetch second loaded image information.\n");
        return NULL;
    }

    CopyMem(dST, gST, sizeof(EFI_SYSTEM_TABLE));

    li->SystemTable = dST;
    li->DeviceHandle = parent_li->DeviceHandle;
    li->FilePath = parent_li->FilePath;

    return dST;
}

void bf_start(CHAR16 *loader_name)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE *li = NULL;
    EFI_FILE_IO_INTERFACE *drive;
    EFI_FILE *root, *loader_file;
    void *loader_contents = NULL, *relocated = NULL;
    EFI_FILE_INFO *fileinfo = NULL;
    PE_COFF_LOADER_IMAGE_CONTEXT context;
    UINTN buffersize = sizeof(EFI_FILE_INFO);
    EFI_HANDLE handle = NULL;
    EFI_SYSTEM_TABLE *dST = NULL;

    Print(L"Starting '%s'\n", loader_name);

    status = gBS->HandleProtocol(this_image_h, &gEfiLoadedImageProtocolGuid, (VOID**)&li);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to fetch loaded image information.\n");
        return;
    }

    status = gBS->HandleProtocol(li->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&drive);
    if (EFI_ERROR(status))
    {
        Print(L"Unable to fetch simple file system protocol.\n");
        return;
    }

    status = drive->OpenVolume(drive, &root);
    if (EFI_ERROR(status))
        return;

    status = root->Open(root, &loader_file, loader_name, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        return;

    fileinfo = AllocatePool(sizeof(EFI_FILE_INFO));
    if (!fileinfo)
        return;

    status = loader_file->GetInfo(loader_file, &gEfiFileInfoGuid, &buffersize, fileinfo);
    if (status == EFI_BUFFER_TOO_SMALL)
    {
        FreePool(fileinfo);
        fileinfo = AllocatePool(buffersize);
        status = loader_file->GetInfo(loader_file, &gEfiFileInfoGuid, &buffersize, fileinfo);
    }

    if (EFI_ERROR(status))
        goto err;

    loader_contents = AllocatePool(fileinfo->FileSize);
    if (!loader_contents)
        goto err;

    status = loader_file->Read(loader_file, &fileinfo->FileSize, loader_contents);
    if (EFI_ERROR(status))
        goto err;

    status = read_header(loader_contents, fileinfo->FileSize, &context);
    if (EFI_ERROR(status))
        goto err;

    status = relocate(loader_contents, fileinfo->FileSize, &context, &relocated);
    if (EFI_ERROR(status))
        goto err;

    status = load_mmapped_file(relocated, fileinfo->FileSize, &handle);
    if (EFI_ERROR(status))
        goto err;


    FreePool(fileinfo);
    fileinfo = NULL;

    entry_point = relocated + context.EntryPoint;
    if ( !entry_point )
        goto err;

    dST = dup_st(handle, li);
    if ( !dST )
        goto err;

    Print(L"Reached entry point\n");

    entry_point(handle, dST);

    /* not reached */

err:
    if ( fileinfo )
        FreePool(fileinfo);
    if ( loader_contents )
        FreePool(loader_contents);

    return;
}

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
