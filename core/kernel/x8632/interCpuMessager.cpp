#include <scaling.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>


#if __SCALING__ >= SCALING_SMP
error_t cpuStreamC::InterCpuMessager::flushTlbRange(void *vaddr, uarch_t nPages)
{
	error_t		err;
	Message	*msg, *msg2=NULL;
	statusE		needToIpi;
	ubit8		extraFlushNeeded=0;

	needToIpi = getStatus();
	if (needToIpi == NOT_TAKING_REQUESTS)
	{
		printf(WARNING CPUMSG"%d: Refused request "
			"(NOT_TAKING_REQUESTS).\n",
			parent->cpuId);

		return ERROR_RESOURCE_BUSY;
	};

	msg = new (cache->allocate(
		SLAMCACHE_ALLOC_LOCAL_FLUSH_ONLY, &extraFlushNeeded)) Message;

	if (msg == NULL) { return ERROR_MEMORY_NOMEM; };

	set(
		msg, CPUMESSAGE_TYPE_TLBFLUSH,
		(uarch_t)vaddr, nPages);

	if (extraFlushNeeded)
	{
		msg2 = new (cache->allocate(SLAMCACHE_ALLOC_LOCAL_FLUSH_ONLY))
			Message;

		if (msg2 == NULL)
		{
			cache->free(msg);
			return ERROR_MEMORY_NOMEM;
		};

		set(
			msg2, CPUMESSAGE_TYPE_TLBFLUSH,
			(uarch_t)msg, 1);
	};

	// Add the message to the CPU's message queue.
	if (extraFlushNeeded) {
		messageQueue.insert(msg2);
	};
	messageQueue.insert(msg);

	// Check to see if an IPI will be needed.
	if (getStatus() == NOT_PROCESSING)
	{
		for (ubit32 i=0; i<1000; i++) {};
		err = cpuTrib.getCurrentCpuStream()->lapic.ipi.sendPhysicalIpi(
			x86LAPIC_IPI_TYPE_FIXED,
			x86LAPIC_VECTOR_IPI,
			x86LAPIC_IPI_SHORTDEST_NONE,
			parent->cpuId);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR CPUMSG"%d: IPI sent from CPU %d "
				"failed.\n",
				parent->cpuId,
				cpuTrib.getCurrentCpuStream()->cpuId);

			return err;
		}
	};

	return ERROR_SUCCESS;
}
#endif /* if __SCALING__ >= SCALING_SMP */

