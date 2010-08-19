
#include <__ksymbols.h>
#include <arch/paging.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/moduleApis/chipsetSupportPackage.h>

#define NP		1
#define VA		0xC0400000

void		*f00=(void *)0xF0000000;

status_t bar(void) __attribute__((noinline));
status_t bar(void)
{
	return walkerPageRanger::mapNoInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		f00, PAGING_LEAF_FAKEMAPPED, NP,
		PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR);
}

extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;
	status_t	status;
	paddr_t		p;
	uarch_t		f;

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
			static_cast<paddr_t>( 0x3FB00000 ),
			__KNULL),
		ret);

	DO_OR_DIE(numaTrib, initialize(), ret);
	DO_OR_DIE(firmwareTrib, initialize(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);

	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)
		|| !__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1))
	{
		if (__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1))
		{
			__kprintf(WARNING"No debug buffer allocated.\n");
		};
	};
	__kdebug.refresh();
	__kprintf(NOTICE"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	timerTrib.dump();

	__kprintf(NOTICE"Main: Vaddr 0x%X's status is: %d.\n",
		VA,
		walkerPageRanger::lookup(
			&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
			(void *)VA, &p, &f));

__kprintf(NOTICE"Main: Returns from lookup on %X: pmap: %X, __kf: %X.\n",
	VA, p, f);

	vaddrSpaceC	*va =
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace;

#define L1	769
#define FL1	960

__kprintf(NOTICE"Main: Explicit read l0[%d]: %X.\n",
	L1, va->level0Accessor.rsrc->entries[L1]);

	p = va->level0Accessor.rsrc->entries[L1];
	*level1Modifier = p | PAGING_L1_PRESENT | PAGING_L1_WRITE;

	tlbControl::flushSingleEntry((void*)level1Accessor);

	__kprintf(NOTICE"Main: Explicit read l0[%d] l1[0]: %X.\n",
		L1, level1Accessor->entries[0]);

	status = bar();
	if (status < NP)
	{
		__kprintf(NOTICE"Only able to map %d pages out of %d.\n",
			status, NP);

		for (;;){};
	};

	__kprintf(NOTICE"Main: Vaddr 0x%X's status is: %d.\n",
		VA,
		walkerPageRanger::lookup(
			&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
			(void *)VA, &p, &f));

__kprintf(NOTICE"Main: Returns from lookup on %X: pmap: %X, __kf: %X.\n",
	VA, p, f);


__kprintf(NOTICE"Main: Explicit read l0[%d]: %X.\n",
	L1, va->level0Accessor.rsrc->entries[L1]);

	p = va->level0Accessor.rsrc->entries[L1];
	*level1Modifier = p | PAGING_L1_PRESENT | PAGING_L1_WRITE;

	tlbControl::flushSingleEntry((void*)level1Accessor);
	__kprintf(NOTICE"Main: Explicit read l0[%d] l1[0]: %X.\n",
		L1, level1Accessor->entries[0]);

__kprintf(NOTICE"Main: Explicit read l0[%d]: %X.\n",
	FL1, va->level0Accessor.rsrc->entries[FL1]);

	p = va->level0Accessor.rsrc->entries[FL1];
	*level1Modifier = p | PAGING_L1_PRESENT | PAGING_L1_WRITE;

	tlbControl::flushSingleEntry((void*)level1Accessor);
	__kprintf(NOTICE"Main: Explicit read l0[%d] l1[0]: %X.\n",
		FL1, level1Accessor->entries[0]);

	*(char *)f00 = 'A';
	if (*(char *)f00 == 'A') {
		__kprintf(NOTICE"Success!\n");
	};
}

