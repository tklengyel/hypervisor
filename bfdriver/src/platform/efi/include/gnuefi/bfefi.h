
// uefi-related includes using gnu-efi

#ifndef BFEFI_H
#define BFEFI_H

#include "efi.h"
#include "efilib.h"
#include "extras.h"
#include "MpService.h"

#ifndef __GNUC__
#define __FILE__ __FILENAME__
#endif

#define CHERROR(status)                                                    \
if(EFI_ERROR(status)) {                                                    \
    Print(L"\n %a: %d: returned status %r\n", __FILE__, __LINE__, status); \
    goto fail;                                                             \
}

#endif
