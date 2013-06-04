#ifndef _TLB_CONTROL_H
	#define _TLB_CONTROL_H

	#include <arch/tlbContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>

/**	EXPLANATION:
 * Architecture specific, per-cpu TLB-state class. This class is useful only on
 * architectures which lack hardware assisted TLB loading. For an arch which
 * doesn't walk the page tables by itself, we have to emulate this service.
 *
 * The process is essentially started with a call to loadTranslation(). This
 * will return ERROR_SUCCESS on a successful lookup and load. Anything else
 * means that the process has accessed a page and the kernel does not know of
 * the mapping for that page, which means that something malignant has gone on
 * in userspace. The process must be terminated.
 *
 * For an arch without TLB loading, the idea is: CPU gets a linear address. CPU
 * looks in TLB for a translation. CPU has no translation for that vaddr in its
 * TLB. CPU triggers a page fault.
 *
 * The kernel must now walk the page tables by itself and load the right
 * translation from the page tables.
 *
 * This is contrasted severely with the fast page table walk that a hardware
 * assisted TLB model provides, where the CPU reads directly to physical RAM. We
 * have to slowly map each frame we need to read into vmem.
 *
 * To be perfectly fair, if writing to and reading from the TLB on an
 * architecture is very fast, then we can provide a very awesome optimization:
 * We could have a loadable TLB state for each process, which is saved on
 * process switch, and restored when a process is loaded onto a CPU. Which is
 * a very awesome optimization if you ask me.
 **/

class vaddrSpaceC;

namespace tlbControl
{
	// Called by walkerPageRangerC.
	void flushSingleEntry(void *vaddr);
	void flushEntryRange(void *vaddr, uarch_t nPages);

	// Clears the local TLB completely of all (even global) entries.
	void purge(void);

	void smpFlushSingleEntry(void *vaddr);
	void smpFlushEntryRange(void *vaddr, uarch_t nPages);

	// Called on by the page-translation-fault code.
	error_t loadTranslation(void *vaddr);

	// Used to load and save a TLB context on the local CPU.
	void loadContext(vaddrSpaceC *vaddrSpace);
	void saveContext(vaddrSpaceC *vaddrSpace);
}

#endif

