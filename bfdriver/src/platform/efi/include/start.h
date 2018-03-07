
#ifndef BF_START_H
#define BF_START_H

#include "bfefi.h"

VOID bf_start_hypervisor_on_core(VOID*);
EFI_STATUS bf_start_by_startupallaps();
EFI_STATUS bf_start_by_switchbsp();


#endif