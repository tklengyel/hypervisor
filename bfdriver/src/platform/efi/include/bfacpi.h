#ifndef BFACPI_H
#define BFACPI_H

EFI_STATUS acpi_init(EFI_HANDLE image_in, EFI_SYSTEM_TABLE* st_in);

uintptr_t acpi_locate(const char *table);

#endif
