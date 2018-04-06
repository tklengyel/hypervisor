
#ifndef HYP_LOADER_H
#define HYP_LOADER_H

#include "bfefi.h"

extern EFI_HANDLE this_image_h;
extern EFI_MP_SERVICES_PROTOCOL* g_mp_services;
extern EFI_SYSTEM_TABLE* gST;
extern EFI_BOOT_SERVICES* gBS;
extern EFI_RUNTIME_SERVICES* gRT;

VOID bf_init_lib(EFI_HANDLE, EFI_SYSTEM_TABLE*);
EFI_STATUS console_get_keystroke(EFI_INPUT_KEY*);
EFI_STATUS bf_boot_next_by_order();
void bf_start(CHAR16 *loader_name);
void bf_start_image(CHAR16 *loader_name);

#endif
