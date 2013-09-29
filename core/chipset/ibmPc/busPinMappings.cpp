
#include <chipset/zkcm/irqControl.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>

#include "busPinMappings.h"

status_t zkcmIrqControlModC::bpmC::loadBusPinMappings(utf8Char *bus)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPcBpm::isa::loadBusPinMappings();
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		printf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

error_t zkcmIrqControlModC::bpmC::get__kpinFor(
	utf8Char *bus, ubit32 busIrqId, ubit16 *__kpin
	)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPcBpm::isa::get__kpinFor(busIrqId, __kpin);
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		printf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};
	
	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

#if 0
/* Deprecated in favour of forcing all calls to maskIrq() and unmaskIrq() to
 * go through Interrupt Trib and the IRQ Control module.
 **/
status_t zkcmIrqControlModC::bpmC::maskIrq(utf8Char *bus, ubit32 busIrqId)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPcBpm::isa::maskIrq(busIrqId);
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		printf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTL_BPM_UNSUPPORTED_BUS;
}

status_t zkcmIrqControlModC::bpmC::unmaskIrq(utf8Char *bus, ubit32 busIrqId)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPcBpm::isa::unmaskIrq(busIrqId);
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		printf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTL_BPM_UNSUPPORTED_BUS;
}
#endif

