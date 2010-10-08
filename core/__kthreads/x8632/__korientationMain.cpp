
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
#include <commonlibs/libacpi/libacpi.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>

int oo=0;

extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;

	__koptimizationHacks();

	// Prepare the kernel by zeroing .BSS and calling constructors.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// processTrib initializes __kprocess & __korientation.
	DO_OR_DIE(processTrib, initialize(), ret);
	DO_OR_DIE(cpuTrib, initialize(), ret);

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
	DO_OR_DIE(numaTrib, initialize2(), ret);
	DO_OR_DIE(processTrib, initialize2(), ret);


	void		*h=__KNULL;
	acpi_sdtS	*t;

	acpi::flushCache();
	if (acpi::findRsdp() == ERROR_SUCCESS)
	{
		__kprintf(NOTICE ORIENT"RSDP found by libacpi.\n");
		if (acpi::testForRsdt())
		{
			acpi::mapRsdt();
			if (acpi::getRsdt() != __KNULL)
			{
				__kprintf(NOTICE ORIENT"RSDT mapped. v 0x%p.\n",
					acpi::getRsdt());

				t = (acpi_sdtS *)acpiRsdt::getNextMadt(
					acpi::getRsdt(), &h);

				__kprintf(NOTICE ORIENT"FACP: 0x%p, 0x%p.\n", t, t->tableLength);
			};
		};
	};

	DO_OR_DIE(cpuTrib, initialize2(), ret);
	__kprintf(NOTICE ORIENT"Successful!\n");
}

