
#ifndef HYP_LIB_H
#define HYP_LIB_H

#include "bfefi.h"

VOID bf_init_lib(EFI_HANDLE, EFI_SYSTEM_TABLE*);

UINTN bf_num_cpus();
VOID* bf_get_variable(CHAR16*, EFI_GUID*, UINTN*);
VOID bf_dump_hex(UINTN, UINTN, UINTN, VOID*);

BOOLEAN bf_match_device_paths(EFI_DEVICE_PATH*, EFI_DEVICE_PATH*);

VOID* bf_allocate_zero_pool(UINTN);
VOID* bf_allocate_runtime_zero_pool(UINTN);
VOID* bf_allocate_zero_pool(UINTN);
VOID bf_free_pool(VOID*);

#endif
