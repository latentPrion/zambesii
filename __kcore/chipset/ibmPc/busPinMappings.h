#ifndef _IBM_PC_BUS_PIN_MAPPINGS_H
	#define _IBM_PC_BUS_PIN_MAPPINGS_H

	#include <__kstdlib/__ktypes.h>

#define IBMPCBPM		"BusPinMapping: "

namespace ibmPcBpm
{
	namespace isa
	{
		status_t loadBusPinMappings(void);
		error_t get__kpinFor(ubit32 busIrqId, ubit16 *__kpin);
		status_t maskIrq(ubit32 busIrqId);
		status_t unmaskIrq(ubit32 busIrqId);
	}

	namespace pci
	{
		/*status_t loadBusPinMappings(void);
		error_t get__kpinFor(ubit32 busIrqId, ubit16 *__kpin);
		status_t maskIrq(ubit32 busIrqId);
		status_t unmaskIrq(ubit32 busIrqId);*/
	}

	namespace acpi
	{
		/*status_t loadBusPinMappings(void);
		error_t get__kpinFor(ubit32 busIrqId, ubit16 *__kpin);
		status_t maskIrq(ubit32 busIrqId);
		status_t unmaskIrq(ubit32 busIrqId);*/
	}
}

#endif

