#include <stdint.h>
#include <bfplatform.h>

#include "bfefi.h"
#include "bfloader.h"
#include "bfacpi.h"

/**
 * Allocate Memory
 *
 * Used by the common code to allocate virtual memory.
 *
 * @param len the size of virtual memory to be allocated in bytes.
 * @return a virtual address pointing to the newly allocated memory
 */
void *platform_alloc_rw(uint64_t len)
{
  EFI_STATUS status;
  EFI_PHYSICAL_ADDRESS ret;
  status = gBS->AllocatePages(AllocateAnyPages,
                              EfiRuntimeServicesCode,
                              (len / EFI_PAGE_SIZE) + 1,
                              &ret);
  CHERROR(status);
  
  return (void*)ret;

fail:
  return NULL;

}

/**
 * Allocate Executable Memory
 *
 * Used by the common code to allocate executable virtual memory.
 *
 * @param len the size of virtual memory to be allocated in bytes.
 * @return a virtual address pointing to the newly allocated memory
 */
void *platform_alloc_rwe(uint64_t len)
{
  EFI_STATUS status;
  EFI_PHYSICAL_ADDRESS ret;
  status = gBS->AllocatePages(AllocateAnyPages,
                              EfiRuntimeServicesCode,
                              (len / EFI_PAGE_SIZE) + 1,
                              &ret);
  CHERROR(status);
  
  return (void*)ret;

fail:
  return NULL;
}

/**
 * Free Memory
 *
 * Used by the common code to free virtual memory that was allocated
 * using the platform_alloc function.
 *
 * @param addr the virtual address returned from platform_alloc
 * @param len the size of the memory allocated
 */
void platform_free_rw(const void *addr, uint64_t len)
{
  EFI_STATUS status;
  status = gBS->FreePages((EFI_PHYSICAL_ADDRESS)addr, 
                          (len / EFI_PAGE_SIZE) + 1);
  CHERROR(status);

fail:
  return;

}

/**
 * Free Executable Memory
 *
 * Used by the common code to free virtual memory that was allocated
 * using the platform_alloc_exec function.
 *
 * @param addr the virtual address returned from platform_alloc_exec
 * @param len the size of the memory allocated
 */
void platform_free_rwe(const void *addr, uint64_t len)
{
  EFI_STATUS status;
  status = gBS->FreePages((EFI_PHYSICAL_ADDRESS)addr, 
                          (len / EFI_PAGE_SIZE) + 1);
  CHERROR(status);

fail:
  return;

}

/**
 * Convert Virtual Address to Physical Address
 *
 * Given a virtual address, this function returns the associated physical
 * address. Note that any page pool issues should be handle by the platform
 * (i.e. the users of this function should be able to provide any virtual
 * address, regardless of where the address originated from).
 *
 * @param virt the virtual address to convert
 * @return the physical address assocaited with the provided virtual address
 */
void *platform_virt_to_phys(void *virt)
{
  return virt;
}

/**
 * Memset
 *
 * @param ptr a pointer to the memory to set
 * @param value the value to set each byte to
 * @param num the number of bytes to set
 */
void* platform_memset(void *ptr, char value, uint64_t num)
{
  gBS->SetMem(ptr, num, value);
  return ptr;
}

/**
 * Memcpy
 *
 * @param dst a pointer to the memory to copy to
 * @param src a pointer to the memory to copy from
 * @param num the number of bytes to copy
 */
void* platform_memcpy(void *dst, const void *src, uint64_t num)
{
  gBS->CopyMem((VOID*)dst, (VOID*)src, num);
  return dst;
}

/**
 * Start
 *
 * Run after the start function has been executed.
 */
void platform_start(void)
{
  ;
}

/**
 * Stop
 *
 * Run after the stop function has been executed.
 */
void platform_stop(void)
{
  ;
}

/**
 * Get Number of CPUs
 *
 * @return returns the total number of CPUs available to the driver.
 */
int64_t
platform_num_cpus(void)
{
  EFI_STATUS status = EFI_SUCCESS;
  UINTN N;
  UINTN NEnabled;

  if (g_mp_services != NULL)
    status = g_mp_services->GetNumberOfProcessors(g_mp_services,
                                                  &N,
                                                  &NEnabled);

  if (!EFI_ERROR(status))
  {
    return N;
  }
  else
  {
    return 0;
  }    
}

/**
 * Set CPU affinity
 *
 * Changes the current core that the driver is running on.
 *
 * @param affinity the cpu number to change to
 * @return The affinity mask of the CPU before the change
 */
int64_t
platform_set_affinity(int64_t affinity)
{
  EFI_STATUS status;
  Print(L"platform_set_affinity %d\n", affinity);

  UINTN ret;
  status = g_mp_services->WhoAmI(g_mp_services,
                               &ret);
  if (status != EFI_SUCCESS) {
    Print(L"WhoAmI error %r\n", status);
    return -1;
  }

  if (ret == (UINTN)affinity)
  {
    return ret;
  }

  status = g_mp_services->EnableDisableAP(g_mp_services,
                                          affinity,
                                          TRUE,
                                          NULL);
  if (status != EFI_SUCCESS) 
  {
    Print(L"EnableDisableAP error %r\n", status);
    return -1;
  }

  status = g_mp_services->SwitchBSP(g_mp_services,
                                    affinity,
                                    TRUE);
  if (status != EFI_SUCCESS) 
  {
    Print(L"SwitchBSP error %r\n", status);
    return -1;
  }

  Print(L"platform_set_affinity success\n");
  return ret;

}

/**
 * Restore CPU affinity
 *
 * If an OS requires the cores used by the user space thread
 * to match on return from a call into the kernel (e.g. IOCTL),
 * reset the affinity to it's previous state.
 *
 * @param affinity the cpu affinity mask.
 *
 */
void
platform_restore_affinity(int64_t affinity)
{
  Print(L"platform_restore_affinity %d\n", affinity);
  platform_set_affinity(affinity);
}

int64_t platform_get_current_cpu_num(void)
{
  EFI_STATUS status;

  UINTN ret;
  status = g_mp_services->WhoAmI(g_mp_services,
                                 &ret);
  if (EFI_ERROR(status)) goto fail;

  return ret;

fail:
  Print(L"platform_get_current_cpu_num error %r\n", status);
  return -1;
}

void platform_restore_preemption(void)
{
  ;
}

int64_t
platform_populate_info(struct platform_info_t *info)
{
    info->acpi_madt = acpi_locate("MADT");
    return BF_SUCCESS;
}

void
platform_unload_info(struct platform_info_t *info)
{
    (void) info;
}
