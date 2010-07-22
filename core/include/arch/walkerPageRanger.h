#ifndef _WALKER_PAGE_RANGER_H
	#define _WALKER_PAGE_RANGER_H

	#include <arch/paging.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/vaddrSpace.h>
	#include <kernel/common/pageAttributes.h>

// These are the valid return values for walkerPageRanger::lookup() and unmap().
#define WPRANGER_STATUS_BACKED		(0)
#define WPRANGER_STATUS_SWAPPED		(1)
#define WPRANGER_STATUS_FAKEMAPPED	(2)
#define WPRANGER_STATUS_GUARDPAGE	(3)
#define WPRANGER_STATUS_UNMAPPED	(4)

// Generic operations on attributes.
#define WPRANGER_OP_SET			(1)
#define WPRANGER_OP_CLEAR		(2)
// This is only valid for setAttributes(). Not remap*().
#define WPRANGER_OP_ASSIGN		(3)
// More specific, customized operations for special cases, like COW, etc.
#define WPRANGER_OP_SET_PRESENT		(4)
#define WPRANGER_OP_CLEAR_PRESENT	(5)
#define WPRANGER_OP_SET_WRITE		(6)
#define WPRANGER_OP_CLEAR_WRITE		(7)

namespace walkerPageRanger
{
	/**	EXPLANATION:
	 * These mapping wrappers will map any address range given to them as
	 * userspace pages. They expect to be called with a valid physical
	 * address to back them. A page mapped with these functions should only
	 * fault on access if the access was a violation of the page's access
	 * rights.
	 **/
	status_t ioWriteThroughMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t ioUncachedMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t execMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t dataMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t roDataMap(
		vaddrSpaceC *vaddrSpace, 
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t fakeMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, uarch_t nPages, uarch_t flags);

	/**	EXPLANATION:
	 * These mapping wrappers will map any address range passed to them as
	 * privileged pages. Again, these are expected to receive valid physical
	 * memory to back them.
	 **/
	status_t __kioWriteThroughMap(
			vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t __kioUncachedMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t __kexecMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t __kdataMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t __kroDataMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages);

	status_t __kfakeMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, uarch_t nPages, uarch_t flags);

	/* swapMap() will take a range of pages and map them with the relevant
	 * bitfield values to indicate that the page has been swapped out to
	 * disk.
	 **/
	void swapMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, uarch_t nPages);

	/**	EXPLANATION:
	 * This next group of functions are the back-ends for all of the inline
	 * functions declared above.
	 *
	 * They each have their own caveats, so please read up on them before
	 * using them.
	 **/
	status_t mapInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages, uarch_t flags);

	status_t mapNoInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages, uarch_t flags);

	void remapInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages,
		ubit8 op, uarch_t flags);

	void remapNoInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages,
		ubit8 op, uarch_t flags);

	void setAttributes(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, uarch_t nPages,
		ubit8 op, uarch_t flags);

	status_t lookup(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t *paddr, uarch_t *flags);

	status_t unmap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t *paddr, uarch_t nPages, uarch_t *flags);

	// Architecture specific kernel flag converter.
	uarch_t decodeFlags(uarch_t flags);
	uarch_t encodeFlags(uarch_t flags);	
}


/**	Inline Methods
 *************************************************************************/

// User mapping functions.

inline status_t walkerPageRanger::ioWriteThroughMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_WRITE | PAGEATTRIB_PRESENT |
		PAGEATTRIB_CACHE_WRITE_THROUGH);
}

inline status_t walkerPageRanger::ioUncachedMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE |
		PAGEATTRIB_CACHE_DISABLED);
}

inline status_t walkerPageRanger::execMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_EXECUTABLE);
}

inline status_t walkerPageRanger::dataMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE);
}

inline status_t walkerPageRanger::roDataMap(
	vaddrSpaceC *vaddrSpace, 
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_PRESENT);
}

inline status_t walkerPageRanger::fakeMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, uarch_t nPages, uarch_t flags
	)
{
	return mapNoInc(
		vaddrSpace, vaddr, PAGING_LEAF_FAKEMAPPED, nPages, flags);
}

// Kernel mapping functions.

inline status_t walkerPageRanger::__kioWriteThroughMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_SUPERVISOR | PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE |
		PAGEATTRIB_CACHE_WRITE_THROUGH);
}

inline status_t walkerPageRanger::__kioUncachedMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_SUPERVISOR | PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE |
		PAGEATTRIB_CACHE_DISABLED);
}

inline status_t walkerPageRanger::__kexecMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_SUPERVISOR | PAGEATTRIB_PRESENT |
		PAGEATTRIB_EXECUTABLE);
}

inline status_t walkerPageRanger::__kdataMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_SUPERVISOR | PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE);
}

inline status_t walkerPageRanger::__kroDataMap(
	vaddrSpaceC *vaddrSpace, 
	void *vaddr, paddr_t paddr, uarch_t nPages
	)
{
	return mapInc(
		vaddrSpace, vaddr, paddr, nPages,
		PAGEATTRIB_SUPERVISOR | PAGEATTRIB_PRESENT);
}

inline status_t walkerPageRanger::__kfakeMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, uarch_t nPages, uarch_t flags
	)
{
	return mapNoInc(
		vaddrSpace, vaddr, PAGING_LEAF_FAKEMAPPED, nPages,
		flags | PAGEATTRIB_SUPERVISOR);
}

// Other mapping functions.

inline void walkerPageRanger::swapMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, uarch_t nPages
	)
{
	// Map all pages as swapped and clear the present attributes.
	remapNoInc(
		vaddrSpace, vaddr, PAGING_LEAF_SWAPPED, nPages,
		WPRANGER_OP_CLEAR_PRESENT, 0);
}

#endif

