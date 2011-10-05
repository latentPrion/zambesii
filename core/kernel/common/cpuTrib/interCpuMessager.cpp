
#include <asm/cpuControl.h>
#include <arch/tlbControl.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t cpuStreamC::interCpuMessagerC::initialize(void)
{
	message.lock.unlock();
	/*	NOTE:
	 * Signifies that this CPU is now "in synch" with the kernel and usable.
	 * The reason this must only be set after the CPU is ready to receive
	 * IPIs is because if it is set any earlier, there is a likelihood that
	 * since the inter-cpu-messager lock is initialized in the "locked"
	 * state, any CPU which allocates memory and needs to broadcast a
	 * "TLB flush" message will deadlock on this CPU's message lock.
	 *
	 * If however, this CPU's message lock isn't set in the global
	 * "availableCpus" BMP, that likelihood is erased, since no CPU will
	 * try to send messages to this CPU if its bit isn't set.
	 **/
	cpuControl::enableInterrupts();
	cpuTrib.availableCpus.setSingle(parent->cpuId);

	return ERROR_SUCCESS;
}

void cpuStreamC::interCpuMessagerC::set(
	ubit8 type, uarch_t val0, uarch_t val1, uarch_t val2, uarch_t val3
	)
{
	__kprintf(NOTICE"message.lock's value: %d.\n", message.lock.lock);
	message.lock.acquire();

	message.rsrc.type = type;
	message.rsrc.val0 = val0;
	message.rsrc.val1 = val1;
	message.rsrc.val2 = val2;
	message.rsrc.val3 = val3;
}

error_t cpuStreamC::interCpuMessagerC::dispatch(void)
{
	messageS	tmp;

	tmp.type = message.rsrc.type;
	tmp.val0 = message.rsrc.val0;
	tmp.val1 = message.rsrc.val1;
	tmp.val2 = message.rsrc.val2;
	tmp.val3 = message.rsrc.val3;

	message.lock.release();
__kprintf(NOTICE"Just released lock in dispatch. Message type is: %d, 0x%p, %d pages. Halting.\n", tmp.type, tmp.val0, tmp.val1);
	switch (tmp.type)
	{
	case CPUMESSAGE_TYPE_TLBFLUSH:
		tlbControl::flushEntryRange(
			(void *)tmp.val0,
			tmp.val1);

		return ERROR_SUCCESS;

	default:
		__kprintf(ERROR CPUMSG"%d: Unknown message type %d.\n",
			parent->cpuId, tmp.type);

	}

	return ERROR_UNKNOWN;
}

