
#include <chipset/zkcm/irqControl.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>

#include "irqControl.h"
#include "busPinMappings.h"

status_t ibmPc_irqControl_bpm_loadBusPinMappings(utf8Char *bus)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPc_bpm_isa_loadBusPinMappings();
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		__kprintf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

error_t ibmPc_irqControl_bpm_get__kpinFor(
	utf8Char *bus, ubit32 busIrqId, ubit16 *__kpin
	)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPc_bpm_isa_get__kpinFor(busIrqId, __kpin);
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		__kprintf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};
	
	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

status_t ibmPc_irqControl_bpm_maskIrq(utf8Char *bus, ubit32 busIrqId)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPc_bpm_isa_maskIrq(busIrqId);
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		__kprintf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

status_t ibmPc_irqControl_bpm_unmaskIrq(utf8Char *bus, ubit32 busIrqId)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPc_bpm_isa_unmaskIrq(busIrqId);
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		__kprintf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

