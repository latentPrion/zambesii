
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/cpuTrib/cpuStream.h>


void cpuStreamC::interCpuMessagerC::flushTlbRange(void *vaddr, uarch_t nPages)
{
	error_t		err;

	set(
		CPUMESSAGE_TYPE_TLBFLUSH,
		(uarch_t)vaddr,
		nPages);

	err = x86Lapic::ipi::sendPhysicalIpi(
		x86LAPIC_IPI_TYPE_FIXED,
		x86LAPIC_IPI_VECTOR,
		x86LAPIC_IPI_SHORTDEST_NONE,
		parent->cpuId);

	if (err != ERROR_SUCCESS)
	{
		__kprintf(ERROR"CPU messager: Failed to send IPI to CPU %d.\n",
			parent->cpuId);
	}

__kprintf(NOTICE"Just did set.\n");
}

