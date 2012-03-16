
#include "memoryDetection.h"
#include "cpuDetection.h"
#include "terminalMod.h"
#include "irqControl.h"

struct zkcmIrqControlModS	ibmPc_irqControlMod =
{
	&ibmPc_irqControl_initialize,
	&ibmPc_irqControl_detectPins,
	&ibmPc_irqControl_shutdown,
	&ibmPc_irqControl_suspend,
	&ibmPc_irqControl_restore,

	&ibmPc_irqControl___kregisterPinIds,
	&ibmPc_irqControl_maskIrq,
	&ibmPc_irqControl_unmaskIrq,
	&ibmPc_irqControl_maskAll,
	&ibmPc_irqControl_unmaskAll,
	&ibmPc_irqControl_maskIrqsByPriority,
	&ibmPc_irqControl_unmaskIrqsByPriority,
	&ibmPc_irqControl_getIrqStatus
};

struct zkcmMemoryDetectionModS	ibmPc_memoryDetectionMod =
{
	&ibmPc_memoryMod_initialize,
	&ibmPc_memoryMod_shutdown,
	&ibmPc_memoryMod_suspend,
	&ibmPc_memoryMod_restore,

	&ibmPc_memoryMod_getMemoryConfig,
	&ibmPc_memoryMod_getNumaMap,
	&ibmPc_memoryMod_getMemoryMap
};

struct debugModS	ibmPc_terminalMod =
{
	&ibmPc_terminalMod_initialize,
	&ibmPc_terminalMod_shutdown,
	&ibmPc_terminalMod_suspend,
	&ibmPc_terminalMod_awake,

	&ibmPc_terminalMod_isInitialized,
	&ibmPc_terminalMod_syphon,
	&ibmPc_terminalMod_clear
};

struct zkcmCpuDetectionModS		ibmPc_cpuDetectionMod =
{
	&ibmPc_cpuMod_initialize,
	&ibmPc_cpuMod_shutdown,
	&ibmPc_cpuMod_suspend,
	&ibmPc_cpuMod_restore,

	&ibmPc_cpuMod_checkSmpSanity,
	&ibmPc_cpuMod_getBspId,
	&ibmPc_cpuMod_getSmpMap,
	&ibmPc_cpuMod_getNumaMap,

	&ibmPc_cpuMod_powerControl
};

