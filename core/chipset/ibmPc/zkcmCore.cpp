
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kclib/string8.h>
#include "vgaTerminal.h"


zkcmCoreC::zkcmCoreC(utf8Char *chipsetName, utf8Char *chipsetVendor)
{
	strcpy8(zkcmCoreC::chipsetName, chipsetName);
	strcpy8(zkcmCoreC::chipsetVendor, chipsetVendor);

	debug[0] = &ibmPcVgaTerminal;
	debug[1] = debug[2] = debug[3] = __KNULL;
}

error_t zkcmCoreC::initialize(void) { return ERROR_SUCCESS; }
error_t zkcmCoreC::shutdown(void) { return ERROR_SUCCESS; }
error_t zkcmCoreC::suspend(void) { return ERROR_SUCCESS; }
error_t zkcmCoreC::restore(void) { return ERROR_SUCCESS; }

zkcmCoreC		zkcmCore(CC"IBM PC compatible", CC"Unknown");

