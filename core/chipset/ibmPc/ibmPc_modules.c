
#include "memoryMod.h"
#include "terminalMod.h"


struct memoryModS	ibmPc_memoryMod =
{
	&ibmPc_memoryMod_initialize,
	&ibmPc_memoryMod_shutdown,
	&ibmPc_memoryMod_suspend,
	&ibmPc_memoryMod_restore,

	&ibmPc_memoryMod_getNumaMap,
	&ibmPc_memoryMod_getMemoryMap,
	&ibmPc_memoryMod_getMemoryConfig
};

// struct cpuModS		ibmPc_cpuMod;
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

