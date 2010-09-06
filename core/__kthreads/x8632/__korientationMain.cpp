
#include <__ksymbols.h>
#include <arch/paging.h>
#include <arch/paddr_t.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <__kclasses/memoryBog.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;
	chipsetMemMapS	*map;

	__koptimizationHacks();

	// Prepare the kernel by zeroing .BSS and calling constructors.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// Initialize BSP CPU stream now; Locking in constructors needs it.
	__korientationPreConstruct::__kprocessInit();
	__korientationPreConstruct::__korientationThreadInit();
	__korientationPreConstruct::bspInit();

	cxxrtl::callGlobalConstructors();
	DO_OR_DIE(timerTrib, initialize(), ret);
	DO_OR_DIE(interruptTrib, initialize(), ret);

	// Initialize the kernel swamp.
	DO_OR_DIE(
		memoryTrib,
		initialize(
			reinterpret_cast<void *>( 0xC0000000 + 0x400000 ),
			0x3FB00000, __KNULL),
		ret);

	DO_OR_DIE(numaTrib, initialize(), ret);
	DO_OR_DIE(firmwareTrib, initialize(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);

	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)
		|| !__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1))
	{
		if (__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1)) {
			__kprintf(WARNING ORIENT"No debug buffer allocated.\n");
		};
	};
	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	DO_OR_DIE(memReservoir, initialize(), ret);

	// Initialize both firmware streams.
	ret = (*chipsetFwStream.initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	ret = (*firmwareFwStream.initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	(*firmwareTrib.getMemInfoRiv()->getMemoryConfig)();
	map = (*firmwareTrib.getMemInfoRiv()->getMemoryMap)();
	if (map == __KNULL) {
		__kprintf(ERROR ORIENT"getMemoryMap() return __KNULL.\n");
	};

	for (uarch_t i=0; i<map->nEntries; i++)
	{
		__kprintf(NOTICE ORIENT"Map: Base 0x%X_%X, size 0x%X_%X type "
			"%d.\n",
			0, map->entries[i].baseAddr, 0, map->entries[i].size,
			map->entries[i].memType);
	};

	__kprintf(NOTICE ORIENT"%d entries in all.\n", map->nEntries);

	assert_fatal(firmwareTrib.getMemInfoRiv() != __KNULL);
	(*firmwareTrib.getMemInfoRiv()->getNumaMap)();
	__kprintf(NOTICE ORIENT"Successful!\n");
}

