
#include <chipset/pkg/chipsetPackage.h>
#include "memoryMod.h"
#include "terminalMod.h"
#include "pic.h"


static error_t ibmPc_pkg_initialize(void) { return ERROR_SUCCESS; }
static error_t ibmPc_pkg_shutdown(void) { return ERROR_SUCCESS; }
static error_t ibmPc_pkg_suspend(void) { return ERROR_SUCCESS; }
static error_t ibmPc_pkg_restore(void) { return ERROR_SUCCESS; }

struct chipsetPackageS		chipsetPkg =
{
	&ibmPc_pkg_initialize,
	&ibmPc_pkg_shutdown,
	&ibmPc_pkg_suspend,
	&ibmPc_pkg_restore,

	__KNULL,
	&ibmPc_memoryMod,
	__KNULL,
	&ibmPc_intControllerMod,

	{
		&ibmPc_terminalMod,
		__KNULL,
		__KNULL,
		__KNULL
	}
};

