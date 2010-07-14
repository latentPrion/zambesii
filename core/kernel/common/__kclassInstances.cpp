
#include <kernel/common/memoryTrib/memoryTrib.h>

/* Part of a trick to prevent reference optimizing linkers from excluding
 * this file.
 **/
int __kclassInstancesInit(void)
{
	volatile memoryTribC		*mtp = &memoryTrib;

	while (mtp) { return 0x00FF; };
	return 0xFFFF;
}

