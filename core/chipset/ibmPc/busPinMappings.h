#ifndef _IBM_PC_BUS_PIN_MAPPINGS_H
	#define _IBM_PC_BUS_PIN_MAPPINGS_H

	#include <__kstdlib/__ktypes.h>

#define IBMPCBPM		"BusPinMapping: "

status_t ibmPc_bpm_isa_loadBusPinMappings(void);
error_t ibmPc_bpm_isa_get__kpinFor(ubit32 busIrqId, ubit16 *__kpin);
status_t ibmPc_bpm_isa_maskIrq(ubit32 busIrqId);
status_t ibmPc_bpm_isa_unmaskIrq(ubit32 busIrqId);

/*status_t ibmPc_bpm_pci_loadBusPinMappings(void);
error_t ibmPc_bpm_pci_get__kpinFor(ubit32 busIrqId, ubit16 *__kpin);
status_t ibmPc_bpm_pci_maskIrq(ubit32 busIrqId);
status_t ibmPc_bpm_pci_unmaskIrq(ubit32 busIrqId);*/

/*status_t ibmPc_bpm_acpi_loadBusPinMappings(void);
error_t ibmPc_bpm_acpi_get__kpinFor(ubit32 busIrqId, ubit16 *__kpin);
status_t ibmPc_bpm_acpi_maskIrq(ubit32 busIrqId);
status_t ibmPc_bpm_acpi_unmaskIrq(ubit32 busIrqId);*/

#endif

