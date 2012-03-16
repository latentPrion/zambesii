
#include <chipset/zkcm/zkcmCore.h>
#include "memoryDetection.h"
#include "cpuDetection.h"
#include "irqControl.h"
#include "terminalMod.h"


static error_t ibmPc_pkg_initialize(void) { return ERROR_SUCCESS; }
static error_t ibmPc_pkg_shutdown(void) { return ERROR_SUCCESS; }
static error_t ibmPc_pkg_suspend(void) { return ERROR_SUCCESS; }
static error_t ibmPc_pkg_restore(void) { return ERROR_SUCCESS; }

struct zkcmCoreS		zkcmCore =
{
	"IBM PC compatible",
	"Unknown",

	&ibmPc_pkg_initialize,
	&ibmPc_pkg_shutdown,
	&ibmPc_pkg_suspend,
	&ibmPc_pkg_restore,

	&ibmPc_memoryDetectionMod,
	&ibmPc_cpuDetectionMod,
	__KNULL,
	&ibmPc_irqControlMod,

	{
		&ibmPc_terminalMod,
		__KNULL,
		__KNULL,
		__KNULL
	}
};

