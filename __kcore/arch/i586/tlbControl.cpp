
#include <__kclasses/debugPipe.h>

#include <arch/x8632/memory.h>
#include <arch/tlbControl.h>
#include <arch/x8632/paging.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

void tlbControl::flushSingleEntry(void *vaddr)
{
	asm volatile (
		"pushl	%%eax \n\t \
		movl	%0, %%eax \n\t \
		invlpg (%%eax) \n\t \
		popl %%eax \n\t"
		:
		: "m" (vaddr)
		: "memory"
	);
}

void tlbControl::saveContext(VaddrSpace *)
{
}

void tlbControl::loadContext(VaddrSpace *context)
{
	asm volatile("movl	%0, %%cr3\n\t"
		:
		: "r" (context->level0Paddr.getLow()));
}

void tlbControl::flushEntryRange(void *vaddr, uarch_t nPages)
{

	for (; nPages > 0;
		vaddr = reinterpret_cast<void *>(
			reinterpret_cast<uintptr_t>( vaddr ) + PAGING_BASE_SIZE ),
			nPages--)
	{
		asm volatile (
			"pushl	%%eax \n\t \
			movl	%0, %%eax \n\t \
			invlpg (%%eax) \n\t \
			popl %%eax \n\t"
			:
			: "m" (vaddr)
			: "memory"
		);
	};
}

void tlbControl::smpFlushEntryRange(void *vaddr, uarch_t nPages)
{
	Bitmap		*bmp;

	/**	EXPLANATION:
	 * There are two cases for SMP range flushing:
	 *	* Flushing a range of addresses in the kernel high vaddrspace.
	 *	* Flushing a range of addresses in a userspace low vaddrspace.
	 *
	 * When flushing from the kernel's vaddrspace, we use the
	 * "availableCPUs" BMP from the CPU Trib to determine which CPUs to send
	 * the flush message to. This is because all CPUs are likely to
	 * access kernel memory.
	 *
	 * For userspace flushing, we use the process's "cpuTrace" BMP so that
	 * we only flush on CPUs which have run the process in question.
	 **/
	flushEntryRange(vaddr, nPages);

	if (!cpuTrib.usingChipsetSmpMode())
	{
		flushEntryRange(vaddr, nPages);
		return;
	};

	if ((uarch_t)vaddr >= ARCH_MEMORY___KLOAD_VADDR_BASE
		&& (uarch_t)vaddr < ARCH_MEMORY___KLOAD_VADDR_END)
	{
		// Flushing kernel range. Pass availableCpus BMP.
		bmp = &cpuTrib.availableCpus;
	}
	else
	{
		// Flushing userspace process range. Pass cpuTrace BMP.
		bmp = &cpuTrib.getCurrentCpuStream()
			->taskStream.getCurrentThread()->parent->cpuTrace;
	};

	bmp->lock();
	for (ubit32 i=0; i<bmp->getNBits(); i++)
	{
		if (bmp->test(i) && i != cpuTrib.getCurrentCpuStream()->cpuId)
		{
			bmp->unlock();

			cpuTrib.getStream(i)->interCpuMessager.flushTlbRange(
				vaddr, nPages);

			bmp->lock();
		};
	};
	bmp->unlock();
}

