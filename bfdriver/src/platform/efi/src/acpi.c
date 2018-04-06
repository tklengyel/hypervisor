#define _DECLARE_GLOBALS
#include "acpidump.h"

ACPI_EFI_STATUS ac_efi_main (ACPI_EFI_HANDLE Image, ACPI_EFI_SYSTEM_TABLE *SystemTab);

ACPI_EFI_STATUS acpi_init(ACPI_EFI_HANDLE Image, ACPI_EFI_SYSTEM_TABLE *SystemTab)
{
    return ac_efi_main(Image, SystemTab);
}

int ACPI_SYSTEM_XFACE acpi_main (int argc, char *argv[])
{
    return 0;
}

uint64_t acpi_locate(const char *table) {
    char                    LocalSignature [ACPI_NAME_SIZE + 1];
    UINT32                  Instance;
    ACPI_TABLE_HEADER       *Table;
    ACPI_STATUS             Status;
    ACPI_PHYSICAL_ADDRESS   Address = 0;

    strcpy (LocalSignature, table);
    AcpiUtStrupr (LocalSignature);

    for (Instance = 0; Instance < AP_MAX_ACPI_FILES; Instance++)
    {
        Status = AcpiOsGetTableByName (LocalSignature, Instance, &Table, &Address);
        if (ACPI_FAILURE (Status))
        {
            /* AE_LIMIT means that no more tables are available */

            if (Status == AE_LIMIT)
            {
                return 0;
            }

            fprintf (stderr,
                "Could not get ACPI table with signature [%s], %s\n",
                LocalSignature, AcpiFormatException (Status));
            return 0;
        }

        ACPI_FREE (Table);
    }

    return Address;
}
