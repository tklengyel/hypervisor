
#ifndef HYP_MODULES_H
#define HYP_MODULES_H

#include "bfefi.h"
#include "common.h"

#define HYPERVISOR_MODULE(mod) \
extern unsigned char mod[];    \
extern unsigned int mod##_len;

#include "modlist.h"
#undef HYPERVISOR_MODULE

#define HYPERVISOR_MODULE(mod)                                      \
    ret = common_add_module((const char*)mod, (uint64_t)mod##_len); \
    if (ret < 0) {                                                  \
        Print(L"common_add_module returned %a\n", ec_to_str(ret));  \
        break;                                                      \
    }

uint64_t add_hypervisor_modules()
{
    int64_t ret;
    Print(L"Adding BF modules\n");
    while (1) {
#include "modlist.h"
        break;
    }

    return ret;

}
#undef HYPERVISOR_MODULE

#endif
