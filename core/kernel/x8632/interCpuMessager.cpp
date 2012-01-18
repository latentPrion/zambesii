
#include <debug.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>


error_t cpuStreamC::interCpuMessagerC::flushTlbRange(void *vaddr, uarch_t nPages)
{
	error_t		err;
	messageS	*msg, *msg2=__KNULL;
	uarch_t		needToIpi=CPUMSGR_STATUS_NORMAL;
	ubit8		extraFlushNeeded=0;

	msg = new (cache->allocate(
		SLAMCACHE_ALLOC_LOCAL_FLUSH_ONLY, &extraFlushNeeded)) messageS;

	if (msg == __KNULL) { return ERROR_MEMORY_NOMEM; };

	set(
		msg, CPUMESSAGE_TYPE_TLBFLUSH,
		(uarch_t)vaddr, nPages);

	if (extraFlushNeeded)
	{
		msg2 = new (cache->allocate(SLAMCACHE_ALLOC_LOCAL_FLUSH_ONLY))
			messageS;

		if (msg2 == __KNULL)
		{
			cache->free(msg);
			return ERROR_MEMORY_NOMEM;
		};

		set(
			msg2, CPUMESSAGE_TYPE_TLBFLUSH,
			(uarch_t)msg, 1);
	};

	// Check to see if an IPI will be needed.
	statusFlag.lock.acquire();

	if (statusFlag.rsrc == CPUMSGR_STATUS_NORMAL) {
		needToIpi = statusFlag.rsrc = CPUMSGR_STATUS_PROCESSING;
	};

	statusFlag.lock.release();

	// Add the message to the CPU's message queue.
	if (extraFlushNeeded) {
		messageQueue.insert(msg2);
	};
	messageQueue.insert(msg);

	if (needToIpi == CPUMSGR_STATUS_PROCESSING)
	{
		for (ubit32 i=0; i<1000; i++) {};
		err = x86Lapic::ipi::sendPhysicalIpi(
			x86LAPIC_IPI_TYPE_FIXED,
			x86LAPIC_IPI_VECTOR,
			x86LAPIC_IPI_SHORTDEST_NONE,
			parent->cpuId);

		if (err != ERROR_SUCCESS)
		{
			__kprintf(ERROR"CPU messager: Failed to send IPI to CPU %d.\n",
				parent->cpuId);

			return err;
		}
	};

	return ERROR_SUCCESS;
}

